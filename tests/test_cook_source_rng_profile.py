#!/usr/bin/env python3
"""Authored cooker guards; these fixtures are not measured live actor traces."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import unittest

SOURCE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('source_rng_cooker', SOURCE / 'tools/cook_source_rng_profile.py')
cooker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cooker)


def rows(seed=0):
    out = [dict(kind='observer_start', bios_sha256=cooker.BIOS_SHA, source_exe_sha256=cooker.EXE_SHA,
                overlay_sha256=cooker.OVERLAY_SHA, scope='CURRENT_INSTALLATION', seed32=seed,
                max_rows=32768, game_boot_parity=False, retail_parity=False),
           dict(kind='game_entry', vsync=1062, game_entry_count=1, rand_ordinal=0, seed32=seed, source_identity_matches=True)]
    for ordinal in range(1, 5):
        after = (seed * 1103515245 + 12345) & 0xffffffff
        call = dict(kind='bios_rng_enter', vsync=1062+ordinal, rand_ordinal=ordinal, game_entry_count=1,
                    service=47, caller_ra=0x801047ac, seed_before32=seed, a0_argument=0)
        ret = dict(kind='bios_rng_return', vsync=1062+ordinal, rand_ordinal=ordinal, service=47,
                   caller_ra=0x801047ac, seed_before32=seed, seed_after32=after, v0_actual=(after >> 16) & 32767)
        out += [call, ret]
        seed = after
    return out


def blob(r):
    return b''.join((json.dumps(x, separators=(',', ':')) + '\n').encode() for x in r)


def complete(r):
    return r + [dict(kind='observer_budget_complete', rand_ordinal=4, rows_before=len(r), pending_return=False)]


class CookerTests(unittest.TestCase):
    def test_whole_canonical_profile(self):
        b = blob(complete(rows(0x80000000)))
        package, proof = cooker.cook(b)
        self.assertEqual(len(package), 256)
        self.assertEqual(struct.unpack_from('<4sHH', package), (b'L3RG', 1, 32))
        self.assertEqual(struct.unpack_from('<I', package, 200)[0], 0x80000000)
        self.assertEqual(proof['scope'], 'WHOLE_BOUNDED_CAPTURE')
        self.assertEqual(proof['snapshot_sha256'], proof['validated_prefix_sha256'])
        self.assertTrue(proof['whole_trace_accepted'])
        self.assertFalse(proof['retail_parity'])

    def test_uncompleted_capture_requires_explicit_prefix(self):
        b = blob(rows())
        with self.assertRaises(ValueError): cooker.cook(b)
        _, p = cooker.cook(b, True)
        self.assertEqual(p['scope'], 'COMPLETED_CAPTURE_PREFIX')
        self.assertEqual(p['excluded_complete_rows'], 0)
        self.assertFalse(p['whole_trace_accepted'])
        self.assertEqual(p['actual_rand_returns'], 4)

    def test_exact_pending_failure_tail(self):
        r = rows(); seed = r[-1]['seed_after32']
        r += [dict(kind='bios_rng_enter', vsync=1070, rand_ordinal=5, game_entry_count=1,
                   service=47, caller_ra=0x801047ac, seed_before32=seed, a0_argument=0),
              dict(kind='observer_guard_failure', reason='lua_callback_error')]
        with self.assertRaises(ValueError): cooker.cook(blob(r))
        package, p = cooker.cook(blob(r), True)
        self.assertEqual(p['excluded_complete_rows'], 2)
        self.assertEqual(p['validated_prefix_sha256'], cooker.sha(blob(r[:-2])))
        self.assertEqual(p['snapshot_sha256'], cooker.sha(blob(r)))
        self.assertEqual(p['warnings_mask'], 31)
        self.assertEqual(struct.unpack_from('<I', package, 248)[0], 1)
        self.assertTrue(p['unfinished_final_call_excluded'])

    def test_partial_line_has_explicit_scope_and_hash(self):
        b = blob(rows()) + b'{"kind":"bios_rng_enter"'
        with self.assertRaises(ValueError): cooker.cook(b)
        _, p = cooker.cook(b, True)
        self.assertTrue(p['partial_tail_excluded'])
        self.assertEqual(p['validated_prefix_sha256'], cooker.sha(blob(rows())))
        self.assertEqual(p['warnings_mask'], 39)

    def test_full_audit_cannot_hide_bad_rows_before_tail(self):
        for field, value in [('v0_actual', 99999), ('seed_after32', 123), ('rand_ordinal', 99), ('caller_ra', 0)]:
            r = rows(); r[3][field] = value
            r += [dict(kind='observer_guard_failure', reason='lua_callback_error')]
            with self.subTest(field=field), self.assertRaises(ValueError): cooker.cook(blob(r), True)
        for kind in ['unknown_marker', 'observer_guard_failure']:
            r = rows(); r.insert(4, dict(kind=kind, reason='interior_failure'))
            with self.subTest(kind=kind), self.assertRaises(ValueError): cooker.cook(blob(r), True)

    def test_malformed_pending_call_is_validated_before_exclusion(self):
        r = rows(); r.append(dict(kind='bios_rng_enter', vsync=1070, rand_ordinal=999,
                                game_entry_count=1, service=47, caller_ra=0x801047ac,
                                seed_before32=r[-1]['seed_after32'], a0_argument=0))
        with self.assertRaises(ValueError): cooker.cook(blob(r), True)

    def test_origin_and_source_guards(self):
        base = rows()
        changes = [(0, 'bios_sha256', '0'*64), (0, 'source_exe_sha256', '0'*64),
                   (0, 'overlay_sha256', '0'*64), (0, 'retail_parity', True),
                   (0, 'scope', 'MID_SESSION'), (0, 'game_boot_parity', True),
                   (1, 'rand_ordinal', 1), (1, 'game_entry_count', 2),
                   (1, 'seed32', 1), (1, 'source_identity_matches', False),
                   (2, 'game_entry_count', 0), (2, 'vsync', True), (2, 'a0_argument', -1)]
        for index, key, value in changes:
            r = copy.deepcopy(base); r[index][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): cooker.cook(blob(r), True)
        for r in [base[:1]+base[2:], base+[base[1]], base[:2]]:
            with self.assertRaises(ValueError): cooker.cook(blob(r), True)

    def test_zero_seed_is_observed_not_defaulted(self):
        _, p = cooker.cook(blob(complete(rows())))
        self.assertEqual(p['origin']['seed32'], 0)
        r = rows(); del r[1]['seed32']
        with self.assertRaises(ValueError): cooker.cook(blob(r), True)

    def test_state_gap_and_reseed_refused(self):
        r = rows(); r[2]['seed_before32'] = 1
        with self.assertRaises(ValueError): cooker.cook(blob(r), True)
        r = rows()[:2]
        r += [dict(kind='bios_rng_enter', vsync=1063, rand_ordinal=0, game_entry_count=1,
                   service=48, caller_ra=0x801047ac, seed_before32=0, a0_argument=1),
              dict(kind='bios_rng_return', vsync=1063, rand_ordinal=0, service=48,
                   caller_ra=0x801047ac, seed_before32=0, seed_after32=1, v0_actual=0)]
        with self.assertRaises(ValueError): cooker.cook(blob(r), True)

    def test_duplicate_keys_and_scalar_types_rejected(self):
        r = blob(rows()).replace(b'"seed32":0', b'"seed32":0,"seed32":0', 1)
        with self.assertRaises(ValueError): cooker.cook(r, True)
        r = rows(); r[3]['v0_actual'] = False
        with self.assertRaises(ValueError): cooker.cook(blob(r), True)
        with self.assertRaises(ValueError): cooker.cook(b'\n', True)

    def test_completed_budget_marker_must_be_exact_terminal(self):
        r = complete(rows()); r[-1]['rows_before'] -= 1
        with self.assertRaises(ValueError): cooker.cook(blob(r), True)
        r = complete(rows()) + [dict(kind='observer_guard_failure', reason='late_error')]
        # A final failure still makes the whole snapshot unaccepted; the known
        # completed boundary remains usable only as explicitly scoped evidence.
        _, p = cooker.cook(blob(r), True)
        self.assertFalse(p['whole_trace_accepted'])

    def test_input_provenance_never_binds_chronology(self):
        r = [dict(kind='input_observer_start', scope='CURRENT_INSTALLATION', source_exe_sha256=cooker.EXE_SHA,
                  pad_override=False, retail_parity=False),
             dict(kind='source_update_input', update_ordinal=1, reference_vsync=1100, dispatcher_identity_matches=True,
                  complete_native_chronology_matches=False, rng_pending_return=False, rng_rand_ordinal=1, rng_reference_vsync=1100,
                  pad_held_buttons=0, player_work=0x800b51b0, player_flags00=0, player_marker01=0, player_state08=0,
                  player_action09=0, player_phase0A=0, player_position14=[0,0,0], player_words20=[0,0,0],
                  player_velocity44=[0,0,0], player_heading56=0, player_hp6E=80, player_held11C=0, admitted_actor_slots=[],
                  stage_byte800C1B60=0, transition_byte800C356C=0, busy_halfword80098A72=0, event_active00=0,
                  event_id01=0, event_phase04=0, story_flag_ids=[])]
        _, p = cooker.cook(blob(rows()), True, blob(r))
        self.assertEqual(p['input_reference_audit']['snapshot_sha256'], cooker.sha(blob(r)))
        self.assertFalse(p['input_reference_audit']['complete_native_chronology_matches'])
        r[-1]['complete_native_chronology_matches'] = True
        with self.assertRaises(ValueError): cooker.cook(blob(rows()), True, blob(r))


if __name__ == '__main__':
    unittest.main()

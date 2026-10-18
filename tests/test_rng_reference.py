#!/usr/bin/env python3
"""Authored trace/overlay guards; no BIOS, executable, game data or live inputs."""
import copy
import importlib.util
from pathlib import Path
import unittest

SOURCE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    'rng_reference_validator',
    SOURCE / 'docs/research_01420/validate_rng_reference_trace.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def authored_rows():
    return [
        {'kind': 'observer_start', 'bios_sha256': module.BIOS_SHA,
         'source_exe_sha256': module.EXE_SHA, 'overlay_sha256': module.OVERLAY_SHA,
         'seed32': 0, 'retail_parity': False},
        {'kind': 'bios_rng_enter', 'service': 47, 'rand_ordinal': 1,
         'caller_ra': 0x801022F8, 'seed_before32': 0, 'a0_argument': 0},
        {'kind': 'bios_rng_return', 'service': 47, 'rand_ordinal': 1,
         'caller_ra': 0x801022F8, 'seed_before32': 0, 'seed_after32': 12345,
         'v0_actual': 0},
        {'kind': 'class03_overlay_admitted', 'overlay_identity_matches': True,
         'source_overlay_sha256': module.OVERLAY_SHA},
        {'kind': 'class03_sample_store', 'overlay_identity_matches': True,
         'rand_ordinal': 1, 'v0_actual': 0, 'sample32A_to_store': 0,
         'slot': 15, 'work': 0x8009F108 + 15 * 0x3C0,
         'state08': 1, 'action09': 0, 'phase0A': 1, 'stats_index0C': 0,
         'context_high0F': 0, 'position14': [-32768, 32767, 0], 'heading56': 65535},
    ]


class RngReferenceTests(unittest.TestCase):
    def test_authored_complete_return_and_store(self):
        result = module.validate(authored_rows())
        self.assertEqual(result['actual_class03_samples'], 1)
        self.assertTrue(result['continuous_since_installation'])
        self.assertFalse(result['retail_parity'])
        self.assertFalse(result['game_seed_for_retail_bios_proved'])
        self.assertFalse(result['shared_native_consumer_lifecycle_proved'])
        self.assertTrue(result['class03_captured_context_fields_validated'])
        self.assertFalse(result['class03_input_update_chronology_proved'])

    def test_store_precedes_actual_return_rejects(self):
        rows = authored_rows()
        with self.assertRaisesRegex(ValueError, 'precedes its actual BIOS return'):
            module.validate([rows[0], rows[1], rows[3], rows[4], rows[2]])

    def test_store_pc_is_not_original_caller_return_address(self):
        rows = authored_rows()
        rows[1]['caller_ra'] = rows[2]['caller_ra'] = 0x801022FC
        with self.assertRaisesRegex(ValueError, 'actual original BIOS return'):
            module.validate(rows)

    def test_one_random_ordinal_cannot_supply_two_class03_stores(self):
        rows = authored_rows()
        rows.append(copy.deepcopy(rows[-1]))
        with self.assertRaisesRegex(ValueError, 'Repeated class03 store'):
            module.validate(rows)

    def test_captured_actor_context_is_required_not_reconstructed(self):
        for name in ('state08', 'action09', 'phase0A', 'stats_index0C',
                     'context_high0F', 'position14', 'heading56'):
            with self.subTest(field=name):
                rows = authored_rows()
                del rows[-1][name]
                with self.assertRaises(ValueError):
                    module.validate(rows)

    def test_incomplete_tail_is_explicit(self):
        result = module.validate(authored_rows()[:2])
        self.assertTrue(result['pending_return_at_end'])
        self.assertFalse(result['continuous_since_installation'])
        self.assertEqual(result['status'], 'UNKNOWN')

    def test_budget_complete_requires_final_completed_boundary(self):
        rows = authored_rows()
        marker = {'kind': 'observer_budget_complete', 'rand_ordinal': 1,
                  'rows_before': len(rows), 'pending_return': False}
        self.assertTrue(module.validate(rows + [marker])['bounded_observer_completed'])
        for field, value in [('rand_ordinal', 2), ('rows_before', 0),
                             ('pending_return', True)]:
            broken = dict(marker); broken[field] = value
            with self.assertRaises(ValueError):
                module.validate(rows + [broken])
        with self.assertRaises(ValueError):
            module.validate(rows + [marker, marker])
        pending_marker = dict(marker, rows_before=2)
        with self.assertRaises(ValueError):
            module.validate(rows[:2] + [pending_marker])

    def test_observer_identity_failure_rejects(self):
        with self.assertRaises(ValueError):
            module.validate([authored_rows()[0],
                             {'kind': 'observer_guard_failure', 'reason': 'authored'}])

    def test_nonmatching_overlay_never_reads_or_admits_actor(self):
        rows = authored_rows()
        rows.insert(1, {'kind': 'class03_overlay_not_admitted',
                        'overlay_identity_matches': False, 'actor_fields_read': False})
        result = module.validate(rows)
        self.assertEqual(result['not_admitted_overlay_markers'], 1)
        self.assertTrue(result['class03_overlay_admitted'])

    def test_store_before_overlay_admission_rejects(self):
        rows = [row for row in authored_rows() if row['kind'] != 'class03_overlay_admitted']
        with self.assertRaises(ValueError):
            module.validate(rows)

    def test_code_mismatch_after_admission_rejects(self):
        rows = authored_rows()
        rows.append({'kind': 'class03_overlay_not_admitted',
                     'overlay_identity_matches': False, 'actor_fields_read': False})
        with self.assertRaises(ValueError):
            module.validate(rows)

    def test_failed_trace_only_reports_validated_prefix(self):
        rows = authored_rows()[:3] + [{'kind': 'observer_guard_failure',
                                      'reason': 'authored_unknown_overlay'}]
        result = module.validate_failed_prefix(rows)
        self.assertEqual(result['actual_rand_returns'], 1)
        self.assertEqual(result['actual_class03_samples'], 0)
        self.assertFalse(result['whole_trace_accepted'])
        self.assertTrue(result['prefix_only_validation'])
        self.assertFalse(result['retail_parity'])
        with self.assertRaises(ValueError):
            module.validate(rows)


def corruption_test(row, key, value):
    def test(self):
        rows = copy.deepcopy(authored_rows())
        rows[row][key] = value
        with self.assertRaises(ValueError):
            module.validate(rows)
    return test


for name, row, key, value in [
    ('actual_return', 2, 'v0_actual', 1),
    ('stored_state', 2, 'seed_after32', 12346),
    ('actor_store', 4, 'sample32A_to_store', 1),
    ('caller_identity', 2, 'caller_ra', 0x80000000),
    ('pool_slot', 4, 'slot', 16),
    ('work_alignment', 4, 'work', 0),
    ('global_ordinal', 1, 'rand_ordinal', 2),
    ('overlay_identity', 4, 'overlay_identity_matches', False),
    ('unsigned_domain', 1, 'seed_before32', -1),
    ('bios_identity', 0, 'bios_sha256', '0' * 64),
    ('state_byte', 4, 'state08', 256),
    ('context_bool', 4, 'context_high0F', True),
    ('raw_short', 4, 'position14', [0, -32769, 0]),
    ('raw_lane_count', 4, 'position14', [0, 0, 0, 0]),
    ('heading_unsigned', 4, 'heading56', -1),
    ('heading_truncation', 4, 'heading56', 65536),
]:
    setattr(RngReferenceTests, 'test_corrupted_' + name, corruption_test(row, key, value))


if __name__ == '__main__':
    unittest.main()

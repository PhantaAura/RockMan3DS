#!/usr/bin/env python3
"""Synthetic comparator fixtures; none of these rows is a retail measurement."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tools/p2'))
from mml_control_corpus import rows_for, scenarios, write_corpus
from mml_trace_io import assemble_trace
from trace_compare import TraceProfileError, compare, compare_traces, input_sha256, normalize_input, validate_trace


def fixture(kind='synthetic_test'):
    rows = [{'frame': i, 'input': [16, 0, 0, 0], 'position': [i, 0, -i],
             'velocity_work': [16, 0, 0], 'heading': 4095,
             'state': [0, 1, 0], 'grounded': None,
             'contact': {'floor_y': None, 'floor_primitive': None},
             'vertical_velocity': 0, 'animation_request': 1, 'animation_frame': i,
             'camera': {'eye': [0, -188, 512], 'look_at': [0, -150, 0]}}
            for i in range(3)]
    fields = [key for key in rows[0] if key not in ('frame', 'input')]
    meta = {'source_kind': kind, 'game_profile': {'id': 'synthetic-fixture', 'sha256': '1' * 64},
            'producer': {'name': 'test_fixture', 'version': '1', 'artifact_sha256': '2' * 64,
                         'settings_sha256': '3' * 64},
            'scenario': 'FLAT_ACCEL', 'initial_state_id': 'synthetic-flat-pose',
            'timing': {'clock': 'simulation_tick', 'rate_num': 30, 'rate_den': 1,
                       'sample_phase': 'post_update_fixture'},
            'coordinate_profile': 'psx-y-down-integer', 'control_profile': 'USA-profile-0',
            'capture_fields': fields, 'field_units': {key: 'fixture_units' for key in fields}}
    meta['field_units']['heading'] = 'turn_4096'
    return assemble_trace(rows, meta, kind)


class ParityTraceTests(unittest.TestCase):
    def test_first_divergence_and_field_deltas(self):
        ref, candidate = fixture(), fixture()
        candidate['frames'][1]['position'][2] = 10
        candidate['frames'][2]['heading'] = 0
        result = compare_traces(ref, candidate)
        self.assertFalse(result['exact'])
        self.assertEqual(result['first_divergence'], {'frame': 1, 'field': 'position[2]',
                                                     'reference': -1, 'candidate': 10, 'delta': 11})
        self.assertEqual(result['field_errors']['heading']['max_abs_error'], 1)
        self.assertEqual(result['field_errors']['position[2]']['mean_abs_error'], 11 / 3)
        self.assertFalse(result['retail_comparison'])
        self.assertFalse(result['PHYSICS_1TO1'])

    def test_source_label_controls_claim_and_null_coverage(self):
        ref, candidate = fixture('retail_reference'), fixture('native_replay')
        result = compare_traces(ref, candidate)
        self.assertTrue(result['retail_comparison'])
        self.assertEqual(result['status'], 'MATCHES_MEASURED_FIELDS')
        self.assertEqual(result['field_coverage']['grounded']['status'], 'UNKNOWN')
        self.assertEqual(result['field_coverage']['grounded']['unavailable_reference_samples'], 3)
        self.assertEqual(result['field_coverage']['grounded']['observed_samples'], 0)
        self.assertEqual(result['field_coverage']['camera.eye[1]']['observed_samples'], 3)
        self.assertFalse(result['PHYSICS_1TO1'])
        result = compare_traces(fixture('native_replay'), fixture('native_replay'))
        self.assertEqual(result['comparison_kind'], 'native_determinism')
        self.assertEqual(result['status'], 'CANDIDATE')
        self.assertFalse(result['retail_comparison'])
        with self.assertRaises(TraceProfileError):
            compare_traces(candidate, ref)
        with self.assertRaises(TraceProfileError):
            compare_traces(ref, fixture('retail_reference'))

    def test_unknown_reference_never_becomes_matching_measured_state(self):
        ref, candidate = fixture('retail_reference'), fixture('native_replay')
        ref['metadata']['capture_fields'] = ['grounded']
        candidate['frames'][0]['grounded'] = True
        result = compare_traces(ref, candidate)
        self.assertTrue(result['exact'])
        self.assertEqual(result['status'], 'UNKNOWN')
        self.assertEqual(result['observed_scalar_samples'], 0)
        self.assertEqual(result['field_coverage']['grounded']['status'], 'UNKNOWN')

    def test_missing_candidate_and_state_boolean_mismatch(self):
        ref, candidate = fixture(), fixture()
        candidate['metadata']['capture_fields'].remove('animation_frame')
        del candidate['frames'][0]['animation_frame']
        result = compare_traces(ref, candidate)
        self.assertEqual(result['first_divergence']['field'], 'animation_frame')
        self.assertEqual(result['first_divergence']['candidate'], '<missing>')
        self.assertEqual(result['field_coverage']['animation_frame']['unavailable_candidate_samples'], 1)
        ref['frames'][0]['grounded'] = True
        candidate['frames'][0]['grounded'] = 1
        result = compare_traces(ref, candidate)
        self.assertEqual(result['first_divergence']['field'], 'grounded')

    def test_profile_rejects_unlabeled_wrong_game_start_phase_inputs_units(self):
        with self.assertRaises(TraceProfileError):
            compare_traces([{'frame': 0}], [{'frame': 0}])
        for field, value in [('game_profile', {'id': 'other', 'sha256': '4' * 64}),
                             ('initial_state_id', 'different-pose'),
                             ('control_profile', 'USA-profile-1'),
                             ('coordinate_profile', 'y-up'), ('scenario', 'JUMP'),
                             ('timing', {'clock': 'GPU::Vsync', 'rate_num': 60, 'rate_den': 1,
                                         'sample_phase': 'display_capture'})]:
            candidate = fixture()
            candidate['metadata'][field] = value
            with self.subTest(field=field), self.assertRaises(TraceProfileError):
                compare_traces(fixture(), candidate)
        candidate = fixture()
        candidate['metadata']['field_units']['position'] = 'meters'
        with self.assertRaises(TraceProfileError):
            compare_traces(fixture(), candidate)
        candidate = fixture()
        candidate['frames'][1]['input']['held_psx_mask'] = 64
        candidate['metadata']['input_sha256'] = input_sha256(candidate['frames'])
        with self.assertRaises(TraceProfileError):
            compare_traces(fixture(), candidate)

    def test_malformed_trace_rejected(self):
        edits = [lambda t: t['metadata'].pop('producer'),
                 lambda t: t['metadata'].__setitem__('source_kind', 'guess'),
                 lambda t: t['metadata'].__setitem__('capture_fields', ['position', 'position']),
                 lambda t: t['metadata'].__setitem__('capture_fields', [{}]),
                 lambda t: t['metadata'].__setitem__('input_sha256', 'bad'),
                 lambda t: t['frames'][1].__setitem__('frame', 8),
                 lambda t: t['frames'][0].__setitem__('heading', float('nan')),
                 lambda t: t['frames'][0]['input'].__setitem__('fire_pressed', 1),
                 lambda t: t['frames'][0]['input'].__setitem__('held_psx_mask', True),
                 lambda t: t['frames'][0].pop('contact')]
        for edit in edits:
            item = fixture()
            edit(item)
            with self.subTest(edit=edit), self.assertRaises(TraceProfileError):
                validate_trace(item)

    def test_heading_wrap_and_contact_animation_camera_mismatches(self):
        ref, candidate = fixture(), fixture()
        ref['frames'][0]['heading'] = 4096
        candidate['frames'][0]['heading'] = 0
        self.assertTrue(compare_traces(ref, candidate)['exact'])
        for field in ('contact', 'animation_request', 'camera'):
            ref, candidate = fixture(), fixture()
            if field == 'contact':
                ref['frames'][0]['contact']['floor_y'] = 0
                candidate['frames'][0]['contact']['floor_y'] = 10
            elif field == 'camera':
                candidate['frames'][0]['camera']['eye'][1] += 1
            else:
                candidate['frames'][0]['animation_request'] += 1
            report = compare_traces(ref, candidate)
            self.assertTrue(report['first_divergence']['field'].startswith(field))

    def test_legacy_api_never_claims_retail(self):
        report = compare([{'frame': 0, 'position': [0, 0, 0]}], [{'frame': 0, 'position': [0, 0, 0]}])
        self.assertTrue(report['exact'])
        self.assertFalse(report['retail_comparison'])
        self.assertEqual(report['comparison_kind'], 'unlabeled_native_determinism')

    def test_ingest_preserves_actual_measurements_and_refuses_relabel(self):
        source = fixture('native_replay')
        rows = copy.deepcopy(source['frames'])
        rows[0]['input'] = [16, 0, 0, 0]
        result = assemble_trace(rows, source['metadata'], 'native_replay')
        self.assertEqual(result, source)
        self.assertEqual(rows[0]['input'], [16, 0, 0, 0])
        with self.assertRaises(TraceProfileError):
            assemble_trace(rows, source['metadata'], 'retail_reference')
        for malformed in ([{}], [1], []):
            with self.assertRaises((TraceProfileError, KeyError)):
                assemble_trace(malformed, source['metadata'], 'native_replay')

    def test_control_corpus_reproducibility_and_no_fictional_outputs(self):
        records = scenarios()
        self.assertEqual(len(records), 21)
        self.assertEqual(len({r['id'] for r in records}), 21)
        for record in records:
            self.assertEqual(record['status'], 'UNKNOWN')
            self.assertFalse(record['retail_reference_available'])
            self.assertNotIn('expected', record)
            rows = rows_for(record).splitlines()
            self.assertEqual(len(rows), sum(s['ticks'] for s in record['segments']))
            for i, row in enumerate(rows):
                fields = [int(v) for v in row.split()]
                self.assertEqual(fields[0], i)
                self.assertEqual(len(fields), 5)
                self.assertGreaterEqual(fields[1], 0)
                self.assertLessEqual(fields[1], 65535)
        with tempfile.TemporaryDirectory() as first, tempfile.TemporaryDirectory() as second:
            a, b = write_corpus(first), write_corpus(second)
            self.assertEqual(a, b)
            for path in Path(first).iterdir():
                self.assertEqual(path.read_bytes(), (Path(second) / path.name).read_bytes())
            with self.assertRaises(FileExistsError):
                write_corpus(first)

    def test_cli_reports_rejection_separately_and_never_overwrites_capture(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)
            (path / 'legacy.json').write_text(json.dumps([{'frame': 0, 'input': [0, 0, 0, 0]}]))
            cmd = [sys.executable, str(ROOT / 'tools/p2/trace_compare.py'), str(path / 'legacy.json'), str(path / 'legacy.json')]
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn('unlabeled', result.stderr)
            result = subprocess.run(cmd + ['--legacy-native'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0)
            self.assertFalse(json.loads(result.stdout)['retail_comparison'])
            source = fixture('native_replay')
            (path / 'rows.json').write_text(json.dumps(source['frames']))
            (path / 'meta.json').write_text(json.dumps(source['metadata']))
            ingest = [sys.executable, str(ROOT / 'tools/mml_trace_io.py'), 'wrap-native',
                      str(path / 'rows.json'), str(path / 'meta.json'), str(path / 'capture.json')]
            result = subprocess.run(ingest, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            original = (path / 'capture.json').read_bytes()
            result = subprocess.run(ingest, capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual((path / 'capture.json').read_bytes(), original)


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
"""Bounded actor contact proof against hash-gated, read-only owner inputs.

Runs the original actor wall wrapper on authored scalar core-result premises,
and full original wall/floor queries on materialized ST00 stage premises.
Exports only authored scalar cases, resource hashes and aggregate results.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01417'), str(SOURCE / 'docs/research_01415')]
from class20_03_callback_source_probe import authored_store_hook
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

WORK = 0x1000


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def signed32(value):
    value &= 0xffffffff
    return value - 0x100000000 if value & 0x80000000 else value


def fields(cpu):
    return {
        'raw': [cpu.get(WORK + 0x14 + i * 2, 2, True) for i in range(4)],
        'fixed': [cpu.get(WORK + 0x1c + i * 4, 4, True) for i in range(4)],
        'velocity': [cpu.get(WORK + 0x44 + i * 2, 2, True) for i in range(3)],
        'state74': cpu.get(WORK + 0x74, 1),
    }


def flatten_output(output):
    return output['raw'] + output['fixed'] + output['velocity'] + [output['state74']]


def fixture_line(case, prefix):
    values = prefix + [case['class'], case['input_state74']] + case['input_raw'] + case['input_fixed'] + \
             case['prior_raw'] + case['prior_fixed'] + case['input_velocity'] + \
             [case['result']] + flatten_output(case['output'])
    return '\t'.join(map(str, values)) + '\n'


def synthetic_stage(cpu, packed, code=1, flags=0, idx_raw6=0):
    # Authored area, cells, IDX, descriptor and one terminated contact group.
    # Original gather, bank selection, dispatch and response run without hooks.
    for address, width, value in [(0x1f800040, 1, 62), (0x1f800041, 1, 62),
                                  (0x1f800044, 2, 4), (0x1f800046, 2, 4),
                                  (0x1f800048, 4, 0x3000), (0x1f80004c, 4, 0x4000)]:
        cpu.put(address, width, value)
    for index in range(16):
        cpu.put(0x3000 + index * 2, 2, packed)
    if packed < 0:
        return
    cpu.put(0x4012, 2, idx_raw6)
    cpu.put(0x4014, 4, 0x5000)
    cpu.put(0x5004, 4, 0x6000)
    cpu.put(0x5008, 4, 0x6000)
    for index, value in enumerate((-100, 100, -100, 100)):
        cpu.put(0x6000 + index * 2, 2, value)
    cpu.put(0x6008, 4, 0x7000)
    cpu.put(0x600c, 4, 1)
    for index, value in enumerate((-128, 128, code, flags)):
        cpu.put(0x7000 + index * 2, 2, value)


def authored_work(cpu, object_class, state, raw, fixed, previous_raw, previous_fixed):
    cpu.r[4], cpu.r[29] = WORK, 0x700000
    cpu.put(WORK + 1, 1, object_class)
    cpu.put(WORK + 0x74, 1, state)
    cpu.put(WORK + 0x78, 4, 0x2000)
    for i, value in enumerate((-60, 60, -60, 60, -128, 0)):
        cpu.put(0x2000 + i * 2, 2, value)
    for offset, values, width in [(0x14, raw, 2), (0x1c, fixed, 4),
                                  (0x2c, previous_raw, 2), (0x34, previous_fixed, 4),
                                  (0x44, [321, -654, 987], 2)]:
        for i, value in enumerate(values):
            cpu.put(WORK + offset + i * width, width, value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dat', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists() or any(args.out.with_suffix(suffix).exists() for suffix in
                                ('.wrapper.tsv', '.synthetic.tsv', '.stage.tsv')):
        raise SystemExit('Refusing to overwrite actor contact evidence')
    image = args.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    resources = list(iter_resources(args.dat / 'ST00.BIN'))
    overlay = next(r['payload'] for r in resources if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    wrapper_cases = []
    for object_class, state, result in itertools.product(
            (0, 0x10, 0x20, 0xa3), (0, 1, 2, 255),
            (-2147483648, -1, 0, 1, 0x100, 0x200, 0x400, 0x800,
             0x1000, 0x2000, 0x3f01)):
        cpu = OverlayProbe(image, base, b'')
        raw, prior_raw = [101, -202, 303, 444], [-11, 22, -33, -44]
        fixed = [v * 65536 + 12345 for v in raw]
        prior_fixed = [v * 65536 + 23456 for v in prior_raw]
        authored_work(cpu, object_class, state, raw, fixed, prior_raw, prior_fixed)
        scratch = [-123, 234, -345, 7]
        # The generic stub uses r8 for constants, so write scratch before the
        # declared return-only core hook instead of reusing r8 as its base.
        for i, value in enumerate(scratch[:3]):
            cpu.put(0x1f8000c0 + i * 2, 2, value)
        cpu.put(0x1f8000f6, 1, scratch[3])
        authored_store_hook(cpu, 0x8004b6ac, [], result)
        cpu.run(0x8004b414)
        wrapper_cases.append({'class': object_class, 'input_state74': state,
                              'core_result': result, 'scratch': scratch,
                              'input_raw': raw, 'input_fixed': fixed,
                              'prior_raw': prior_raw, 'prior_fixed': prior_fixed,
                              'input_velocity': [321, -654, 987],
                              'output': fields(cpu), 'result': signed32(cpu.r[2]),
                              'instructions_executed': cpu.steps})
    synthetic_cases = []
    movements = [((-180, -60, 0), (-184, -60, 0)), ((180, -60, 0), (184, -60, 0)),
                 ((-160, -60, 0), (-164, -60, 0)), ((160, -60, 0), (164, -60, 0)),
                 ((-120, -60, 0), (-200, -60, 0)), ((120, -60, 0), (200, -60, 0)),
                 ((0, -60, -120), (0, -60, -200)), ((0, -60, 120), (0, -60, 200)),
                 ((-120, -60, -120), (-200, -60, -200)), ((120, -60, 120), (200, -60, 200)),
                 ((0, -60, 0), (0, -200, 0)), ((0, 200, 0), (0, 400, 0)),
                 ((0, -127, 0), (0, -127, 0)), ((0, -60, 0), (0, -60, 0)),
                 ((0, -128, 0), (0, -128, 0)), ((0, 256, 0), (0, 256, 0))]
    for code, flags, object_class, state, idx_raw6, movement in itertools.product(
            (1, 3, 15, 8, 0x21, 0x301), (0, 0x8000), (0, 0x20), (0, 1, 2), (0, 1), movements):
        cpu = OverlayProbe(image, base, b'')
        raw, prior_raw = list(movement[0]) + [1234], list(movement[1]) + [4321]
        fixed = [value * 65536 + 12345 for value in raw]
        prior_fixed = [value * 65536 + 23456 for value in prior_raw]
        authored_work(cpu, object_class, state, raw, fixed, prior_raw, prior_fixed)
        synthetic_stage(cpu, 1, code, flags, idx_raw6)
        cpu.run(0x8004b414)
        wall_result, output = signed32(cpu.r[2]), fields(cpu)
        wall_steps = cpu.steps
        cpu.r[4], cpu.r[31] = WORK, 0
        cpu.run(0x80048e6c)
        synthetic_cases.append({'class': object_class, 'input_state74': state,
                                'code': code, 'flags': flags, 'packed': 1, 'idx_raw6': idx_raw6,
                                'input_raw': raw, 'input_fixed': fixed, 'prior_raw': prior_raw,
                                'prior_fixed': prior_fixed, 'input_velocity': [321, -654, 987],
                                'result': wall_result, 'output': output, 'floor_result': signed32(cpu.r[2]),
                                'instructions_executed': cpu.steps, 'wall_instructions_executed': wall_steps})
    for level, bands, state, dy in itertools.product((0, 1, 17, 31), (0, 1, 2, 3), (0, 1, 2), (-32, 0, 32)):
        packed = (0x8000 | (level << 8) | (bands << 13)) - 65536
        floor = -level * 128 - 1
        for y in (floor - 32, floor, floor + 32):
            cpu = OverlayProbe(image, base, b'')
            raw, prior_raw = [0, y, 0, 1234], [-4, y - dy, 0, 4321]
            fixed, prior_fixed = [v * 65536 + 12345 for v in raw], [v * 65536 + 23456 for v in prior_raw]
            authored_work(cpu, 0x20, state, raw, fixed, prior_raw, prior_fixed)
            synthetic_stage(cpu, packed)
            cpu.run(0x8004b414)
            result, output = signed32(cpu.r[2]), fields(cpu)
            wall_steps = cpu.steps
            cpu.r[4], cpu.r[31] = WORK, 0
            cpu.run(0x80048e6c)
            synthetic_cases.append({'class': 0x20, 'input_state74': state, 'code': 0, 'flags': 0,
                                    'packed': packed, 'idx_raw6': 0, 'input_raw': raw, 'input_fixed': fixed,
                                    'prior_raw': prior_raw, 'prior_fixed': prior_fixed, 'input_velocity': [321, -654, 987],
                                    'result': result, 'output': output, 'floor_result': signed32(cpu.r[2]),
                                    'instructions_executed': cpu.steps, 'wall_instructions_executed': wall_steps})
    selected = {}
    for name, load in [('idx', 0x8015c000), ('mdt', 0x80164000),
                       ('stg', 0x80194000), ('hed', 0x8013a000)]:
        found = [r for r in resources if r['type'] == 0 and r['load'] == load]
        assert len(found) == 1
        selected[name] = found[0]
    idx, mdt, stg, hed = [selected[n]['payload'] for n in ('idx', 'mdt', 'stg', 'hed')]
    pointers = []
    for slot in range(512):
        pointer = struct.unpack_from('<I', hed, slot * 8 + 4)[0]
        if not pointer:
            break
        pointers.append(pointer & 0xffffff)
    admitted = []
    for identity in range(len(idx) // 12):
        descriptor = struct.unpack_from('<I', idx, identity * 12 + 8)[0]
        if not descriptor or descriptor & 0xffffff not in pointers:
            continue
        header = struct.unpack_from('<I', stg, descriptor - 0x80194000 + 24)[0]
        seed = struct.unpack_from('<H', stg, header - 0x80194000 + 2)[0]
        admitted.append((identity, seed))
    assert len(admitted) == 994
    sx, sz, width, height, cell_pointer = struct.unpack_from('<4BI', mdt, 12)
    stage_cases = []
    for record in (0x8011a274, 0x8011a28c, 0x8011a2a4):
        origin = list(struct.unpack_from('<3h', overlay, record - 0x80100000 + 12))
        for delta in ((0, 0, 0), (4, 0, 0), (-4, 0, 0), (0, 0, 4), (0, 0, -4), (4, 0, 4)):
            cpu = OverlayProbe(image, base, overlay)
            for resource in selected.values():
                for offset, value in enumerate(resource['payload']):
                    cpu.put(resource['load'] + offset, 1, value)
                    cpu.put((resource['load'] & 0xffffff) + offset, 1, value)
            for identity, seed in admitted:
                cpu.put(0x8015c006 + identity * 12, 2, seed)
                cpu.put(0x15c006 + identity * 12, 2, seed)
            raw = [origin[i] + delta[i] for i in range(3)] + [1234]
            fixed = [v * 65536 + (8192 if i != 1 else 0) for i, v in enumerate(raw)]
            previous = origin + [1234]
            previous_fixed = [v * 65536 for v in previous]
            authored_work(cpu, 0x20, 0, raw, fixed, previous, previous_fixed)
            for address, size, value in [(0x1f800040, 1, sx), (0x1f800041, 1, sz),
                                          (0x1f800044, 2, width), (0x1f800046, 2, height),
                                          (0x1f800048, 4, cell_pointer), (0x1f80004c, 4, 0x8015c000)]:
                cpu.put(address, size, value)
            cpu.run(0x8004b414)
            wall = signed32(cpu.r[2])
            wall_fields = fields(cpu)
            wall_steps = cpu.steps
            cpu.r[4], cpu.r[31] = WORK, 0
            cpu.run(0x80048e6c)
            stage_cases.append({'record_address': hex(record), 'origin': origin,
                                'delta': list(delta), 'input_raw': raw, 'input_fixed': fixed,
                                'prior_raw': previous, 'prior_fixed': previous_fixed,
                                'wall_result': wall, 'wall_output': wall_fields,
                                'floor_result': signed32(cpu.r[2]),
                                'wall_instructions_executed': wall_steps,
                                'floor_instructions_executed': cpu.steps - wall_steps,
                                'hooks': cpu.return_only_hooks})
    evidence = {'schema': 'actor-contact-source/1', 'status': 'SOURCE_BACKED_PARTIAL',
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
                'resources': {n: sha(r['payload']) for n, r in selected.items()},
                'materialized_identity_count': len(admitted),
                'bounds': [-60, 60, -60, 60, -128, 0], 'area': 1,
                'wrapper_external_premise': 'Only 8004B6AC return and scratch externally authored; all original wrapper instructions execute.',
                'wrapper_cases': wrapper_cases, 'stage_cases': stage_cases,
                'synthetic_cases': synthetic_cases,
                'limits': ['Synthetic loaded-RAM premises; native scheduling and live encounter parity excluded.',
                           'No owner bytes, geometry arrays, table payloads or instruction listings exported.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(evidence, indent=2) + '\n')
    args.out.with_suffix('.wrapper.tsv').write_text('# class state result scratch4 input_raw4 input_fixed4 prior_raw4 prior_fixed4 velocity3 returned_result output_raw4 output_fixed4 output_velocity3 state\n' +
        ''.join(fixture_line(case, [case['core_result']] + case['scratch']) for case in wrapper_cases))
    args.out.with_suffix('.synthetic.tsv').write_text('# packed code flags raw6 class state input_raw4 input_fixed4 prior_raw4 prior_fixed4 velocity3 result output_raw4 output_fixed4 output_velocity3 state floor_result\n' +
        ''.join(fixture_line(case, [case['packed'], case['code'], case['flags'], case['idx_raw6']]).rstrip('\n') + '\t' + str(case['floor_result']) + '\n' for case in synthetic_cases))
    args.out.with_suffix('.stage.tsv').write_text('# class state input_raw4 input_fixed4 prior_raw4 prior_fixed4 velocity3 result output_raw4 output_fixed4 output_velocity3 state floor_result\n' +
        ''.join(fixture_line({'class': 0x20, 'input_state74': 0, 'input_velocity': [321, -654, 987],
                            'result': case['wall_result'], 'output': case['wall_output'], **case}, []).rstrip('\n') + '\t' + str(case['floor_result']) + '\n' for case in stage_cases))
    print(json.dumps({'wrapper_cases': len(wrapper_cases),
                      'synthetic_cases': len(synthetic_cases),
                      'stage_cases': [(s['origin'], s['delta'], s['wall_result'], s['floor_result']) for s in stage_cases],
                      'instructions_executed': sum(s['instructions_executed'] for s in wrapper_cases) +
                          sum(s['instructions_executed'] for s in synthetic_cases) +
                          sum(s['wall_instructions_executed'] + s['floor_instructions_executed'] for s in stage_cases),
                      'evidence_sha256': sha(args.out.read_bytes())}, indent=2))


if __name__ == '__main__':
    main()

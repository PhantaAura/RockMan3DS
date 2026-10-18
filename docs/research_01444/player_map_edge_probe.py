#!/usr/bin/env python3
"""Bounded, no-hook original map-edge calls for the native 01443 west escape.

Read the supported owner EXE and already-extracted ST00.BIN in place. Stage
resources are selected and materialized only in probe memory. Export hashes,
addresses, aggregate admissions and scalar query observations; never export
original instruction, table, geometry-array or asset payloads. These authored
poses are not a complete original player update or a live reference trace.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01418'),
               str(SOURCE / 'docs/research_01415')]
from contact_source_probe import (OverlayProbe, SUPPORTED_EXE, authored_work,
                                  fields, iter_resources, signed32)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def region(image, base, start, end):
    return {'start': hex(start), 'end_exclusive': hex(end),
            'sha256': sha(image[start - base + 2048:end - base + 2048])}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dat', type=Path, required=True,
                        help='Existing extracted CDDATA/DAT directory')
    parser.add_argument('--pack', type=Path, required=True,
                        help='Existing pack; metadata read only, no recook')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists():
        raise ValueError('Choose a new proof output; earlier evidence is preserved')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise ValueError('Unsupported owner executable identity')
    base = struct.unpack_from('<I', image, 24)[0]
    resources = list(iter_resources(args.dat / 'ST00.BIN'))
    selected = {}
    for name, load in [('idx', 0x8015C000), ('mdt', 0x80164000),
                       ('stg', 0x80194000), ('hed', 0x8013A000)]:
        matches = [r for r in resources if r['type'] == 0 and r['load'] == load]
        if len(matches) != 1:
            raise ValueError('Owner resource selection is not unique: ' + name)
        selected[name] = matches[0]
    idx, mdt, stg, hed = [selected[n]['payload'] for n in ('idx', 'mdt', 'stg', 'hed')]
    pointers = []
    for slot in range(512):
        pointer = struct.unpack_from('<I', hed, slot * 8 + 4)[0]
        if not pointer:
            break
        pointers.append(pointer & 0xFFFFFF)
    seeds = {}
    for identity in range(len(idx) // 12):
        descriptor = struct.unpack_from('<I', idx, identity * 12 + 8)[0]
        if not descriptor or descriptor & 0xFFFFFF not in pointers:
            continue
        header = struct.unpack_from('<I', stg, descriptor - 0x80194000 + 24)[0]
        seeds[identity] = struct.unpack_from('<H', stg, header - 0x80194000 + 2)[0]
    if len(seeds) != 994:
        raise ValueError('Source startup materialization identity count differs')
    # The actual source initializer and prior 01418 differential prove these
    # render-header seeds; this probe does not invent a raw6 selector override.
    sx, sz, width, height, cell_pointer = struct.unpack_from('<4BI', mdt, 4)
    pack_map = (args.pack / 'st00.l3map').read_bytes()
    pack_init = (args.pack / 'st00.l3init').read_bytes()
    if pack_map[:4] != b'L3MP' or pack_init[:4] != b'L3MI':
        raise ValueError('Unsupported existing pack metadata')
    offsets = struct.unpack_from('<7I', pack_map, 48)
    pack_seeds = {struct.unpack_from('<H', pack_init, p)[0]:
                  struct.unpack_from('<H', pack_init, p + 8)[0]
                  for p in range(48, len(pack_init), 16)}
    if seeds != pack_seeds:
        raise ValueError('Existing pack startup materialization differs from owner')
    original14 = struct.unpack_from('<4hI', idx, 14 * 12)
    cooked14 = struct.unpack_from('<4h2H', pack_map, offsets[2] + 14 * 12)
    if original14[:4] != cooked14[:4]:
        raise ValueError('IDX14 owner/cooked scalar binding differs')
    descriptor = original14[4] - 0x80194000
    alt, default = struct.unpack_from('<2I', stg, descriptor + 4)
    cooked_desc = struct.unpack_from('<I2H', pack_map, offsets[3] + cooked14[4] * 44)
    if alt != default or cooked_desc[1] != cooked_desc[2]:
        raise ValueError('Expected IDX14 identical alternate/default admission differs')
    li = cooked_desc[1]
    group_first, group_count = struct.unpack_from('<2I', pack_map, offsets[4] + li * 8)
    kinds = Counter()
    for gi in range(group_first, group_first + group_count):
        primitive_first, primitive_count = struct.unpack_from('<2I', pack_map, offsets[5] + gi * 16 + 8)
        for pi in range(primitive_first, primitive_first + primitive_count):
            kinds[hex(struct.unpack_from('<H', pack_map, offsets[6] + pi * 8 + 4)[0])] += 1
    if group_count != 1 or kinds != Counter({'0x3': 1}):
        raise ValueError('Expected IDX14 floor-only admission differs')

    def candidates(x, z):
        nx = 0x7E00 if (x & 511) < 257 else 0x8200
        nz = 0x7E00 if (z & 511) < 257 else 0x8200
        found = []
        for ox, oz in [(0x8000, 0x8000), (nx, 0x8000),
                       (0x8000, nz), (nx, nz)]:
            gx, gz = (x + ox) // 512, (z + oz) // 512
            lx, lz = gx - sx, gz - sz
            if not (0 <= lx < width and 0 <= lz < height):
                continue
            local = lz * width + lx
            packed = struct.unpack_from('<h', mdt, cell_pointer - 0x80164000 + local * 2)[0]
            cooked = struct.unpack_from('<h', pack_map, offsets[1] + local * 2)[0]
            if packed != cooked:
                raise ValueError('Owner/cooked cell admission differs')
            found.append({'cell_index': local, 'packed_cell': packed})
        return found

    bounds = (-32, 32, -32, 32, -160, 0)
    if struct.unpack_from('<6h', image, 0x80097D90 - base + 2048) != bounds:
        raise ValueError('PL00 owner bounds differ')
    cases = []
    premises = [
        ('west_inside', (-390, -1, 1761), (-365, -1, 1761), 0, -1),
        ('west_trace_escape', (-449, -1, 1761), (-424, -1, 1761), 0, 32767),
        ('west_next_sample', (-479, -1, 1746), (-449, -1, 1761), 0, 32767),
        ('west_outside_below_floor', (-449, 2, 1761), (-424, -1, 1761), 0, 32767),
        ('east_inside', (390, -1, 1761), (365, -1, 1761), 0, -1),
        ('east_outside', (449, -1, 1761), (424, -1, 1761), 0, 32767),
    ]
    for name, position, previous, expected_wall, expected_floor in premises:
        cpu = OverlayProbe(image, base, b'')
        for resource in selected.values():
            for offset, value in enumerate(resource['payload']):
                cpu.put(resource['load'] + offset, 1, value)
                cpu.put((resource['load'] & 0xFFFFFF) + offset, 1, value)
        for identity, seed in seeds.items():
            cpu.put(0x8015C006 + identity * 12, 2, seed)
            cpu.put(0x15C006 + identity * 12, 2, seed)
        raw, prior = list(position) + [0], list(previous) + [0]
        authored_work(cpu, 0, 0, raw, [v * 65536 for v in raw],
                      prior, [v * 65536 for v in prior])
        for i, value in enumerate(bounds):
            cpu.put(0x2000 + i * 2, 2, value)
        for address, size, value in [
            (0x1F800040, 1, sx), (0x1F800041, 1, sz),
            (0x1F800044, 2, width), (0x1F800046, 2, height),
            (0x1F800048, 4, cell_pointer), (0x1F80004C, 4, 0x8015C000),
        ]:
            cpu.put(address, size, value)
        cpu.run(0x8004B414)
        wall_result, wall_after = signed32(cpu.r[2]), fields(cpu)
        wall_steps, wall_calls = cpu.steps, list(cpu.calls)
        cpu.r[4], cpu.r[31] = 0x1000, 0
        cpu.run(0x80048E6C)
        floor_result = signed32(cpu.r[2])
        if (wall_result != expected_wall or floor_result != expected_floor or
                wall_after['raw'] != raw or wall_after['fixed'] != [v * 65536 for v in raw]):
            raise ValueError('Original map-edge scalar result differs: ' + name)
        if cpu.return_only_hooks:
            raise ValueError('No-hook premise violated')
        found = candidates(position[0], position[2])
        cases.append({'id': name, 'input_xyz': position, 'prior_xyz': previous,
                      'input_state74': 0, 'object_class': 0,
                      'wall_result': wall_result, 'wall_output': wall_after,
                      'floor_result': floor_result, 'gather_before_positive_dedup': found,
                      'positive_identities_after_dedup': sorted({c['packed_cell'] for c in found if c['packed_cell'] > 0}),
                      'wall_original_instructions': wall_steps,
                      'floor_original_instructions': cpu.steps - wall_steps,
                      'wall_direct_calls': wall_calls,
                      'helper_hooks': 0, 'instruction_patches': 0})
    proof = {'schema': 'mml-player-map-edge/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'PHYSICS_1TO1': False, 'exe_sha256': sha(image),
             'resources_sha256': {n: sha(r['payload']) for n, r in selected.items()},
             'existing_pack_sha256': {'st00.l3map': sha(pack_map), 'st00.l3init': sha(pack_init)},
             'area_index': 0, 'bounds': bounds,
             'materialized_identity_count': len(seeds),
             'startup_seeds_match_existing_pack': True,
             'idx14_owner_binding': {'descriptor_index': cooked14[4],
                                     'source_alternate_equals_default': True,
                                     'cooked_contact_list_index': li,
                                     'materialized_raw6': seeds[14],
                                     'group_count': group_count,
                                     'primitive_code_counts': dict(kinds)},
             'source_regions': [region(image, base, 0x8004B414, 0x8004B5CC),
                                region(image, base, 0x8004B6AC, 0x8004BC34),
                                region(image, base, 0x80048E6C, 0x8004920C)],
             'cases': cases,
             'finding': 'Original map wrapper supplies no horizontal correction at the sampled west escape; outside-floor query returns32767.',
             'fall_kill_recovery_caller': 'UNKNOWN',
             'limits': ['Authored bounded query poses, not a live original controller trace.',
                        'Complete player movement, adhesion ordering, physical registry pairs, stage transitions and recovery are not executed.',
                        'This proves no map wall at these query poses; it does not prove that original full gameplay permits this escape.',
                        'Separate floor-order repair does not establish a wall or recovery fix.',
                        'No asset, source table, geometry array or instruction payload is exported; no existing pack is modified.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps({'cases_passed': len(cases), 'helper_hooks': 0,
                      'instruction_patches': 0, 'proof_sha256': sha(args.out.read_bytes())}))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Read-only80049C54 differential under authored contexts and materializedstage.

Exports scalar fixtures, counts/hashes only; actualstage/ownerbounds fixtures
stay owner-local. No original instruction listings, tables or payloadbytes.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01415'), str(SOURCE / 'docs/research_01418')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
from contact_source_probe import synthetic_stage, signed32

WORK, END, BOUNDS = 0x1000, 0x1100, 0x2000
AUTHORED_BOUNDS = [-25, 25, -35, 35, -70, 10]


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def invoke(c, start, end, bounds, source_class):
    for address, values in [(WORK, start), (END, end), (BOUNDS, bounds)]:
        for i, value in enumerate(values):
            c.put(address + i * 2, 2, value)
    c.r[4:8] = [WORK, END, BOUNDS, source_class]
    c.r[29], c.r[31] = 0x700000, 0
    c.run(0x80049c54)
    result = signed32(c.r[2])
    # Independent original gather invocation provides the exact count only
    # when the original full query actually reached the gather call.
    count = 0
    if any(call['target'] == '0x8004b1e0' for call in c.calls):
        c.r[4:6] = [WORK, 1]
        c.r[31] = 0
        c.run(0x8004b1e0)
        count = c.r[2]
    return result, count


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    args = p.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite segment proof')
    args.out.mkdir(parents=True)
    image = args.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    resources = list(iter_resources(args.dat / 'ST00.BIN'))
    overlay = next(r['payload'] for r in resources if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    rows, stage_rows, instructions = [], [], 0
    counts = {'authored_primitive': 0, 'packed_negative_y': 0, 'overflow_empty_boundary': 0}
    points = [(0, -60, 0), (-126, -60, 0), (-125, -60, 0), (-124, -60, 0),
              (124, -60, 0), (125, -60, 0), (0, -60, -136), (0, -60, -135),
              (0, -60, -134), (0, -139, 0), (0, -138, 0), (0, -137, 0),
              (0, 197, 0), (0, 198, 0), (0, 199, 0)]
    for code, flags, source_class, raw6, point in itertools.product(
            (1, 3, 15, 8, 0x21, 0x301), (0, 5, 0x8000, 0x8009),
            (0, 0x20, 0xffff), (0, 1), points):
        c = OverlayProbe(image, base, b'')
        synthetic_stage(c, 1, code, flags, raw6)
        start = list(point) + [1234]
        end = [point[0] + 16, point[1] - 16, point[2] - 16, -1234]
        result, count = invoke(c, start, end, AUTHORED_BOUNDS, source_class)
        rows.append([1, code, flags, raw6, source_class] + start + end + AUTHORED_BOUNDS + [result, count])
        counts['authored_primitive'] += 1
        instructions += c.steps
    for level, bands, delta, source_class in itertools.product((0, 1, 17, 31), (0, 1, 2, 3),
                                                               (-33, -1, 0, 1, 33), (0, 0xffff)):
        packed = (0x8000 | (level << 8) | (bands << 13)) - 65536
        floor = -level * 128 - 1 - AUTHORED_BOUNDS[5]
        for y in [floor + delta, -level * 128 - bands * 384 + 1 - AUTHORED_BOUNDS[4] + delta]:
            c = OverlayProbe(image, base, b''); synthetic_stage(c, packed)
            start, end = [0, y, 0, 1234], [32, y + 32, -32, -1234]
            result, count = invoke(c, start, end, AUTHORED_BOUNDS, source_class)
            rows.append([packed, 0, 0, 0, source_class] + start + end + AUTHORED_BOUNDS + [result, count])
            counts['packed_negative_y'] += 1; instructions += c.steps
    boundary_points = [(x, -60, z) for x, z in [(-1024, 0), (-1023, 0), (1023, 0), (1024, 0),
                      (0, -1024), (0, -1023), (0, 1023), (0, 1024), (-1023, -1023)]]
    overflow_points = [(x, -60, z) for x, z in [(16384, 0), (16385, 0), (-16384, 0), (-16385, 0),
                       (0, 16384), (0, 16385), (0, -16384), (0, -16385), (32767, 0), (-32768, 0)]]
    for packed, point in itertools.product((0, 1, -32768), boundary_points + overflow_points):
        c = OverlayProbe(image, base, b''); synthetic_stage(c, packed)
        start = list(point) + [1234]
        end = [-1 if point[0] >= 0 else 1, -60, -1 if point[2] >= 0 else 1, -1234]
        result, count = invoke(c, start, end, AUTHORED_BOUNDS, 0xffff)
        rows.append([packed, 1, 0, 0, 0xffff] + start + end + AUTHORED_BOUNDS + [result, count])
        counts['overflow_empty_boundary'] += 1; instructions += c.steps

    selected = {}
    for name, load in [('idx', 0x8015c000), ('mdt', 0x80164000), ('stg', 0x80194000), ('hed', 0x8013a000)]:
        found = [r for r in resources if r['type'] == 0 and r['load'] == load]
        assert len(found) == 1
        selected[name] = found[0]
    idx, mdt, stg, hed = [selected[name]['payload'] for name in ('idx', 'mdt', 'stg', 'hed')]
    pointers = []
    for slot in range(512):
        pointer = struct.unpack_from('<I', hed, slot * 8 + 4)[0]
        if not pointer: break
        pointers.append(pointer & 0xffffff)
    admitted = []
    for identity in range(len(idx) // 12):
        descriptor = struct.unpack_from('<I', idx, identity * 12 + 8)[0]
        if not descriptor or descriptor & 0xffffff not in pointers: continue
        header = struct.unpack_from('<I', stg, descriptor - 0x80194000 + 24)[0]
        admitted.append((identity, struct.unpack_from('<H', stg, header - 0x80194000 + 2)[0]))
    assert len(admitted) == 994
    sx, sz, width, height, cells = struct.unpack_from('<4BI', mdt, 12)
    original_bounds = list(struct.unpack_from('<6h', image, 0x80097d0c - base + 2048))
    for record in (0x8011a274, 0x8011a28c, 0x8011a2a4):
        origin = list(struct.unpack_from('<3h', overlay, record - 0x80100000 + 12))
        for delta in [(0, 0, 0), (0, -60, 0), (256, 0, 0), (-256, 0, 0),
                      (0, 0, 256), (0, 0, -256), (512, 0, 512)]:
            c = OverlayProbe(image, base, overlay)
            for resource in selected.values():
                for off, value in enumerate(resource['payload']):
                    c.put(resource['load'] + off, 1, value)
                    c.put((resource['load'] & 0xffffff) + off, 1, value)
            for identity, seed in admitted:
                c.put(0x8015c006 + identity * 12, 2, seed); c.put(0x15c006 + identity * 12, 2, seed)
            for address, size, value in [(0x1f800040, 1, sx), (0x1f800041, 1, sz),
                                        (0x1f800044, 2, width), (0x1f800046, 2, height),
                                        (0x1f800048, 4, cells), (0x1f80004c, 4, 0x8015c000)]:
                c.put(address, size, value)
            start = [origin[i] + delta[i] for i in range(3)] + [1234]
            end = [start[0] + 32, start[1], start[2] - 32, -1234]
            result, count = invoke(c, start, end, original_bounds, 0xffff)
            stage_rows.append([0xffff] + start + end + original_bounds + [result, count])
            instructions += c.steps
    fixture = args.out / 'actor-detection-segment-authored.tsv'
    fixture.write_text('packed code flags raw6 class start4 end4 bounds6 result candidatecount\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    # This private scalar fixture contains actual ownerbounds/positions; it is
    # never copied to source. The test accepts it only with owner-local packages.
    private = args.out / 'actor-detection-segment-owner-stage.tsv'
    private.write_text('class start4 end4 ownerbounds6 result candidatecount\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in stage_rows))
    evidence = {'schema': 'actor-detection-segment-source-evidence/1', 'status': 'SOURCE_BACKED_PARTIAL',
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay), 'entry': '0x80049c54',
                'authored_cases': len(rows), 'authored_case_categories': counts,
                'authored_fixture_sha256': sha(fixture.read_bytes()),
                'materialized_stage_cases': len(stage_rows), 'owner_stage_fixture_sha256': sha(private.read_bytes()),
                'materialized_identities': len(admitted), 'resources': {n: sha(r['payload']) for n, r in selected.items()},
                'instructions_executed': instructions, 'return_only_hooks': [],
                'bounds_contract': {'source': '0x80097d0c', 'bytes': 12, 'payload_exported': False},
                'limits': ['Authored helper contexts and materialized stage mirror; not live retail encounter or hardware.',
                           'Native only admits proved supported families; excludes unknown intersecting geometry.',
                           'Source-safe authored fixtures do not contain original bounds or payload.',
                           'Private stage scalar fixture must stay owner-local.']}
    (args.out / 'actor-detection-segment-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'authored_cases': len(rows), 'stage_cases': len(stage_rows),
                      'interpreted_instructions': instructions, 'fixture_sha256': evidence['authored_fixture_sha256']}, indent=2))


if __name__ == '__main__':
    main()

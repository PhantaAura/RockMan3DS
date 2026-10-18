#!/usr/bin/env python3
"""Original floor wrapper at three owner spawn positions after IDX initialization.

Reads hash-gated owner input and executes the original floor/gather/classifier
instructions without call hooks. Output is bounded scalar/hash evidence only.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    a = p.parse_args()
    if a.out.exists(): raise SystemExit('Refusing to overwrite floor evidence')
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    resources = list(iter_resources(a.dat / 'ST00.BIN'))
    overlay = next(r['payload'] for r in resources if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    selected = {}
    for name, load in [('idx', 0x8015c000), ('mdt', 0x80164000), ('stg', 0x80194000), ('hed', 0x8013a000)]:
        found = [r for r in resources if r['type'] == 0 and r['load'] == load]
        assert len(found) == 1
        selected[name] = found[0]
    idx, mdt, stg, hed = [selected[n]['payload'] for n in ('idx', 'mdt', 'stg', 'hed')]
    # Reuse the independently original-instruction-proved materialization
    # rule here to create a fresh synthetic loaded-RAM premise.
    admitted = []
    pointers = []
    for slot in range(512):
        pointer = struct.unpack_from('<I', hed, slot * 8 + 4)[0]
        if not pointer: break
        pointers.append(pointer & 0xffffff)
    for i in range(len(idx) // 12):
        descriptor = struct.unpack_from('<I', idx, i * 12 + 8)[0]
        if not descriptor or descriptor & 0xffffff not in pointers: continue
        header = struct.unpack_from('<I', stg, descriptor - 0x80194000 + 24)[0]
        seed = struct.unpack_from('<H', stg, header - 0x80194000 + 2)[0]
        admitted.append((i, seed))
    assert len(admitted) == 994
    sx, sz, width, height, cell_ptr = struct.unpack_from('<4BI', mdt, 12)
    samples = []
    for record in (0x8011a274, 0x8011a28c, 0x8011a2a4):
        pos = list(struct.unpack_from('<3h', overlay, record - 0x80100000 + 12))
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[29] = 0x1000, 0x700000
        for r in selected.values():
            for offset, value in enumerate(r['payload']):
                # Explicit PS1 physical/KSEG0 aliases for descriptor mask reads.
                c.put(r['load'] + offset, 1, value)
                c.put((r['load'] & 0xffffff) + offset, 1, value)
        for identity, seed in admitted:
            c.put(0x8015c006 + identity * 12, 2, seed)
            c.put(0x15c006 + identity * 12, 2, seed)
        c.put(0x1001, 1, 0x20)
        c.put(0x1078, 4, 0x8011bed4)
        for i, value in enumerate(pos):
            c.put(0x1014 + i * 2, 2, value)
            c.put(0x102c + i * 2, 2, value)
        for address, size, value in [(0x1f800040, 1, sx), (0x1f800041, 1, sz),
                                      (0x1f800044, 2, width), (0x1f800046, 2, height),
                                      (0x1f800048, 4, cell_ptr), (0x1f80004c, 4, 0x8015c000)]:
            c.put(address, size, value)
        c.run(0x80048e6c)
        floor = c.r[2] if c.r[2] < 0x80000000 else c.r[2] - 0x100000000
        assert floor == -1
        assert [c.get(0x1014 + i * 2, 2, True) for i in range(3)] == pos
        samples.append({'record_address': hex(record), 'origin': pos, 'floor': floor,
                        'position_preserved': True, 'instructions_executed': c.steps,
                        'direct_calls': c.calls, 'indirect_calls': c.indirect,
                        'hooks': c.return_only_hooks})
    authored = []
    for radius in (0, 60):
        edge = 100 + radius
        points = [(v, 0) for v in (-edge - 1, -edge, edge, edge + 1)] + \
                 [(0, v) for v in (-edge - 1, -edge, edge, edge + 1)]
        for code in (0x21, 0x25, 0x31, 0x35, 0x41, 0x45, 0x51, 0x55):
            # Original function pointer remains owner data, read in memory;
            # only the authored code/query and scalar result are serialized.
            entry = struct.unpack_from('<I', image, 2048 + 0x8008aa24 - base + (code >> 4) * 4)[0]
            for x, z in points:
                c = OverlayProbe(image, base, b'')
                c.r[4], c.r[5], c.r[29] = 0x1000, code, 0x700000
                for i, value in enumerate((-100, 100, -100, 100, -1, 513)):
                    c.put(0x1000 + i * 2, 2, value)
                for i, value in enumerate((-radius, radius, -radius, radius, -128, 0)):
                    c.put(0x1f800124 + i * 2, 2, value)
                c.put(0x1f8000c0, 2, x); c.put(0x1f8000c4, 2, z)
                c.run(entry)
                assert c.r[2] == 0
                authored.append({'code': code, 'bounds_radius': radius, 'query': [x, z],
                                 'result': 0, 'instructions_executed': c.steps})
    evidence = {'schema': 'class20-03-floor-initialization-source/1',
                'status': 'SOURCE_BACKED_PARTIAL', 'exe_sha256': sha(image),
                'overlay_sha256': sha(overlay),
                'resources': {n: sha(r['payload']) for n, r in selected.items()},
                'materialized_identity_count': len(admitted),
                'bounds': [-60, 60, -60, 60, -128, 0], 'area': 1,
                'samples': samples, 'hooks': [], 'authored_outside_classifier_cases': authored,
                'limits': ['Source floor wrapper on authored fresh loaded-RAM premises, not live retail allocation timing.',
                           'IDX materialization rule has separate original-instruction proof.',
                           'No owner bytes, table payloads, geometry arrays or original instructions exported.']}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'samples': [(s['origin'], s['floor'], s['instructions_executed']) for s in samples],
                      'authored_case_count': len(authored),
                      'authored_instructions_executed': sum(c['instructions_executed'] for c in authored),
                      'hooks': [], 'evidence_sha256': sha(a.out.read_bytes())}, indent=2))


if __name__ == '__main__': main()

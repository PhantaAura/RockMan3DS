#!/usr/bin/env python3
"""Original bounded sound-request producers with authored raw byte slots.

No sound data, sample bank, original tables, code, assets or saves are emitted.
This proves queue fields/full drops, not audible playback or consumer dispatch.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path
SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE
BASE, END, POINTER = 0x800be4f8, 0x800be6d8, 0x80098938


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args(); image = a.exe.read_bytes(); assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    if a.out.exists(): raise SystemExit('Refusing to overwrite audio queue evidence')
    a.out.mkdir(parents=True)
    rows, steps = [], 0
    for kind, count, request, raw0, raw1 in itertools.product((0, 1), (0, 1, 22, 23, 24),
            (0, 156, 176, 180, 65535, 0x12345678), (0, 0xffffffff, 0x80000000), (0, 0x1234ffff)):
        c = OverlayProbe(image, base, b''); c.r[29] = 0x700000
        for i in range(480): c.put(BASE + i, 1, (i * 17 + 3) & 255)
        c.put(POINTER, 4, BASE + count * 20)
        c.r[4], c.r[5], c.r[6] = request, raw0, raw1
        if kind == 1:
            c.r[5] = 0x1000; c.put(0x1000, 4, raw0); c.put(0x1004, 4, raw1)
        c.run(0x8001a008 if kind == 0 else 0x8001a050)
        after_count = (c.get(POINTER, 4) - BASE) // 20
        assert after_count == (count if count == 24 else count + 1)
        slot = [c.get(BASE + min(count, 23) * 20 + i, 1) for i in range(20)]
        expected = [((min(count, 23) * 20 + i) * 17 + 3) & 255 for i in range(20)]
        if count < 24:
            expected[0] = kind
            for i in range(2): expected[2 + i] = request >> (8 * i) & 255
            for i in range(4): expected[4 + i] = raw0 >> (8 * i) & 255; expected[8 + i] = raw1 >> (8 * i) & 255
        assert slot == expected
        rows.append([kind, count, request, raw0, raw1, after_count] + slot)
        steps += c.steps
    c = OverlayProbe(image, base, b''); c.run(0x80019e14, 0x80019e28)
    assert c.get(POINTER, 4) == BASE; steps += c.steps
    resets = []
    for count in range(25):
        c = OverlayProbe(image, base, b''); c.put(POINTER, 4, BASE + count * 20)
        c.run(0x8001a1d0, 0x8001a1e0)
        assert c.get(POINTER, 4) == BASE; steps += c.steps; resets.append(count)
    fixture = a.out / 'authored-audio-queue.tsv'
    fixture.write_text(''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    metadata = {'schema': 'source-audio-queue-proof/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'exe_sha256': SUPPORTED_EXE, 'producer_cases': len(rows), 'reset_cases': len(resets),
        'initializer_pointer_verified': True, 'source_capacity': 24, 'source_record_bytes': 20,
        'instructions_executed': steps, 'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
        'retail_parity': False, 'audible_playback_proved': False, 'no_original_payload_exported': True,
        'source': {'initialization': '0x80019E14', 'scalar_producer': '0x8001A008',
                   'pointed_producer': '0x8001A050', 'reset': '0x8001A1D0'},
        'limits': ['Authored raw slot bytes and scalar/pointer premises.',
                   'Only source-used bytes0/2..11 are written; bytes1/12..19 retained.',
                   'Consumer table/sample lookup/driver and actualsound playback unported.',
                   'Clear occurs after consumer dispatch in source; native diagnostic draining must be labelled.']}
    (a.out / 'source-evidence.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps(metadata, indent=2))
if __name__ == '__main__': main()

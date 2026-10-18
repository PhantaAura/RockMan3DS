#!/usr/bin/env python3
"""Hash-gated80048E6C wrapper/full floor-query authored scalar proof.

Owner EXE/overlay are read-only. Output contains authored geometry, scalar
arguments/results and hashes only; never source instructions or payloads.
"""
import argparse
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01422'), str(SOURCE / 'docs/research_01418')]
from class60_06_source_probe import FastProbe, sha, SUPPORTED_EXE, OVERLAY_SHA, iter_resources, WORK, SIZE, store
from contact_source_probe import synthetic_stage, signed32


class FloorProbe(FastProbe):
    def get(self, address, width, signed=False):
        if address == 0x80048ea4 and width == 4:
            self.arguments = [self.r[4], self.r[5], signed32(self.r[6]), self.r[7]]
        return super().get(address, width, signed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dat', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose a new private output directory')
    image = args.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    initialization = FloorProbe(image, base, overlay)
    initialization.put(0x80098798, 4, 0x13572468)
    initialization.put(0x800988e0, 4, 63)
    initialization.r[31] = 0
    initialization.run(0x80033ebc)
    assert initialization.get(0x80098798, 4) == 255
    assert initialization.get(0x800988e0, 4) == 0
    rows = []
    steps = 0
    for packed, code, flags, object_class, position, prior_y in itertools.product(
            (-32768, 1), (1, 3, 15, 8), (0, 0x8000), (0x60, 0x7f),
            ((0, -127, 0), (0, 129, 0), (101, -60, 0)), (-32768, -200, 300)):
        cpu = FloorProbe(image, base, overlay)
        synthetic_stage(cpu, packed, code, flags, 0)
        for i in range(SIZE):
            cpu.put(WORK + i, 1, (i * 13 + 7) & 255)
        cpu.put(WORK + 1, 1, object_class)
        cpu.put(WORK + 0x78, 4, 0)
        store(cpu, 0x14, 2, list(position) + [1234])
        store(cpu, 0x2c, 2, [-19, prior_y, 202, -444])
        before = cpu.blob(WORK, SIZE)
        cpu.r[4], cpu.r[29], cpu.r[31] = WORK, 0x700000, 0
        cpu.run(0x80048e6c)
        assert cpu.arguments == [WORK + 0x14, 0, position[1] - prior_y, object_class]
        assert cpu.blob(WORK, SIZE) == before
        rows.append([packed, code, flags, object_class, *position, prior_y,
                     signed32(cpu.r[2]), *cpu.arguments])
        steps += cpu.steps
    args.out.mkdir(parents=True)
    (args.out / 'shadow-floor-authored.tsv').write_text(
        '# authored packed/code/flags/class/x/y/z/previousY/sourceFloor/sourceArgs4\n' +
        ''.join(' '.join(map(str, row)) + '\n' for row in rows))
    result = {
        'schema': 'class60-shadow-floor-source/1',
        'status': 'SOURCE_BACKED_PARTIAL',
        'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
        'wrapper': '80048E6C', 'floor_query': '80048EA4',
        'authored_cases': len(rows), 'original_instructions': steps,
        'all_arguments_match': True, 'all_work_bytes_unchanged': True,
        'proxy_global_initializer': '80033EBC',
        'proxy_startup_mask_80098798': 255, 'proxy_startup_count_800988e0': 0,
        'proxy_initialization_instructions': initialization.steps,
        'limits': ['Native null bounds only; nonzero bounds tokens held.',
                   'Native80048EA4 still holds unproved slope/unknown visits and Y deltas outside its admitted range.',
                   'No shadow rasterizer, GTE/fog or hardware parity claim.']}
    (args.out / 'source-evidence.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()

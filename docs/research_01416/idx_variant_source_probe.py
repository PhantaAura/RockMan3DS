#!/usr/bin/env python3
"""Bounded original IDX variant initialization on explicit synthetic premises.

Only scalar/hash observations are written. This is not the live raw6=1 writer.
The owner executable is read only; no asset, instruction or source table bytes
are saved. Full loader state and the packed selector's owner are not inferred.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research_01414'))
from player_contact_source_probe import CPU, SUPPORTED_EXE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to replace earlier source evidence')
    image = args.exe.read_bytes()
    assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    load = struct.unpack_from('<I', image, 24)[0]
    idx_base, stack, packed_base = 0x8015c000, 0x700000, 0x800bd3b8
    rows = []
    instructions = 0
    for stage in (0, 4, 5, 6, 7):
        for target in (1, 15, 16, 17, 31, 32, 63):
            for selector in range(4):
                for special in (False, True):
                    for lookup_slot in range(4):
                        cpu = CPU(image, load)
                        cpu.r[29] = stack
                        cpu.put(stack + 80, 4, 0)  # explicit packed-group premise
                        cpu.put(stack + 88, 4, stage)
                        for bank in range(4):
                            for slot in range(4):
                                cpu.put(stack + 16 + bank * 16 + slot * 4, 4,
                                        0x70000 + bank * 0x100 + slot * 4)
                        for page in range(target // 16 + 1):
                            cpu.put(packed_base + page * 4, 4, 0)
                        cpu.put(packed_base + (target // 16) * 4, 4,
                                selector << (2 * (target % 16)))
                        for index in range(target + 1):
                            descriptor = 0x10000 + index * 64
                            packet = 0x20000 + index * 4
                            cpu.put(idx_base + index * 12 + 6, 2, 77)
                            cpu.put(idx_base + index * 12 + 8, 4, descriptor)
                            cpu.put(descriptor, 4, 0x81000000 if special else 0)
                            cpu.put(descriptor + 24, 4, packet)
                            cpu.put(packet, 4, lookup_slot << 14)
                        sentinel = idx_base + (target + 1) * 12
                        cpu.put(sentinel + 6, 2, 91)
                        cpu.put(sentinel + 8, 4, 0)
                        cpu.run(0x8003aec0, 0x8003b10c)
                        raw6 = cpu.get(idx_base + target * 12 + 6, 2)
                        pointer = cpu.get(idx_base + target * 12 + 8, 4)
                        descriptor = 0x10000 + target * 64
                        expected_raw6 = (0, 100, 200, 77)[selector]
                        if selector in (1, 2):
                            bank = selector - 1 if special and stage in (5, 6) else selector + 1
                            expected_pointer = 0x70000 + bank * 0x100 + lookup_slot * 4
                        else:
                            expected_pointer = descriptor
                        assert (raw6, pointer) == (expected_raw6, expected_pointer)
                        assert cpu.get(idx_base + 6, 2) == 77, 'record0 must remain untouched'
                        assert cpu.get(sentinel + 6, 2) == 91, 'sentinel must remain untouched'
                        for index in range(1, target):
                            assert cpu.get(idx_base + index * 12 + 6, 2) == 0
                        instructions += cpu.steps
                        rows.append({'stage_premise': stage, 'target_index': target,
                                     'packed_selector': selector, 'special_descriptor_premise': special,
                                     'synthetic_lookup_slot': lookup_slot, 'raw6_after': raw6,
                                     'descriptor_after': hex(pointer)})
    region = image[2048 + 0x8003aec0 - load:2048 + 0x8003b10c - load]
    result = {'schema': 'idx-variant-source-probe/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'owner_exe_sha256': SUPPORTED_EXE,
              'routine': '0x8003AD14', 'executed_region': ['0x8003AEC0', '0x8003B10C'],
              'executed_region_sha256': hashlib.sha256(region).hexdigest(),
              'case_count': len(rows), 'instructions_executed': instructions,
              'all_scalar_checks_passed': True, 'cases': rows,
              'limits': ['Synthetic packed-group, lookup and descriptor premises; not live replay.',
                         'This routine writes0/100/200, not the captured gameplay value1.',
                         'The full routine derives stack lookup values through prior helpers.',
                         'Stage-group selector ownership and full descriptor substitution semantics remain unbound.',
                         'No native selector override or collision fallback is implemented.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ('case_count', 'instructions_executed',
                                                 'all_scalar_checks_passed')}))


if __name__ == '__main__':
    main()

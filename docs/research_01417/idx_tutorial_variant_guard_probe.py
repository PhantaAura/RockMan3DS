#!/usr/bin/env python3
"""Original tutorial variant guard replay; source-safe scalar/hash evidence only."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research_01414'))
from player_contact_source_probe import CPU, SUPPORTED_EXE


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    a = p.parse_args()
    if a.out.exists(): raise SystemExit('refusing to replace prior guard evidence')
    image = a.exe.read_bytes()
    assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    source_byte = struct.unpack_from('<b', image, 2048 + 0x80089F04 - base)[0]
    assert source_byte == -1
    rows = []
    for flag in (0, 1, 256, 65535):
        c = CPU(image, base)
        c.r[29] = 0x00700000
        c.put(0x800C1B60, 1, 0)
        c.put(0x800C1B74, 2, flag)
        for index in (78, 94, 123):
            c.put(0x8015C000 + 12 * index + 6, 2, 1)
        c.run(0x8003AD14, 0x8003B10C)
        observed = [c.get(0x8015C000 + 12 * index + 6, 2) for index in (78, 94, 123)]
        assert observed == [1, 1, 1]
        rows.append({'raw_flag_800C1B74': flag, 'raw_stage_800C1B60': 0,
                     'source_group_signed': source_byte, 'raw6_after': observed,
                     'instructions_executed': c.steps})
    regions = []
    for start, end in ((0x8003AD14, 0x8003AD7C), (0x8003B5B4, 0x8003B5F4),
                       (0x8003B888, 0x8003B8B0), (0x80089F04, 0x80089F05)):
        regions.append({'start': hex(start), 'end_exclusive': hex(end),
                        'sha256': hashlib.sha256(image[2048 + start - base:2048 + end - base]).hexdigest()})
    result = {'schema': 'idx-tutorial-variant-guard/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'source_exe_sha256': SUPPORTED_EXE, 'source_ranges': regions,
              'cases': rows, 'all_scalar_checks_passed': True,
              'conclusion': 'Original stage0 guard returns before0/100/200 IDX variant tail; optional prior helper also returns for stage0.',
              'limits': ['Synthetic stage/flag premises in original integer source, not original live load timing.',
                         'Stages5/6 and later mutation routes remain separate from stage0 initialization.']}
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'all_scalar_checks_passed': True, 'cases': rows}))


if __name__ == '__main__': main()

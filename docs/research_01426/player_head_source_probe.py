#!/usr/bin/env python3
"""Bounded unchanged owner instructions for the original fresh head selection.

Reads one exact owner EXE and selected existing head container. Outputs scalar
metadata only. Stops before external asset-loading calls; no game payload write.
"""
import argparse
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415'),
               str(SOURCE / 'docs/research_01414')]
from cook_actor_combat import OwnerExe
from source_player_head import select_fresh_player_head, HEAD_TABLE
from tutorial_actor_identity_probe import OverlayProbe


def prove(image, dat):
    owner = OwnerExe(image)
    selection = select_fresh_player_head(image, dat)
    base = struct.unpack_from('<I', image, 24)[0]
    instructions = 0
    for initial in range(256):
        cpu = OverlayProbe(image, base, b'')
        cpu.r[16], cpu.r[17], cpu.r[18] = 0x800B51B0, 0x800B5DB0, 0
        cpu.put(0x800B531A, 1, initial)
        cpu.run(0x8003BF38, 0x8003BF7C)
        if cpu.get(0x800B531A, 1) != selection['selector']:
            raise ValueError('Original fresh selector differential failed')
        instructions += cpu.steps
    loaded = []
    for selector in (0, 1):
        expected = struct.unpack('<h', owner.read(HEAD_TABLE + selector * 2, 2))[0]
        for cached in (-1, 0, 27, 28):
            cpu = OverlayProbe(image, base, b'')
            cpu.r[29] = 0x7000
            cpu.put(0x800B531A, 1, selector)
            cpu.put(0x800C1B80, 2, cached)
            cpu.run(0x8001F5E4, 0x8001F624 if cached == expected else 0x8001D440)
            called = any(c['target'] == '0x8001d440' for c in cpu.calls)
            if cpu.r[4] != expected or called != (cached != expected) or cpu.get(0x800C1B80, 2, True) != expected:
                raise ValueError('Original component-loader differential failed')
            loaded.append({'selector': selector, 'cached': cached,
                           'selected_file_id': cpu.r[4], 'requests_load': called})
            instructions += cpu.steps
    return {'format': 'fresh-player-head-source-proof/1', 'status': 'SOURCE_BACKED_PARTIAL',
            'selection': selection, 'fresh_initial_values_checked': 256,
            'loader_cases': loaded, 'original_instructions_executed': instructions,
            'original_instructions_replaced': False, 'external_asset_call_executed': False,
            'retail_payload_written': False, 'resumed_gear_runtime_implemented': False,
            'hardware_or_full_visual_parity': 'UNKNOWN'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dat', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(prove(args.exe.read_bytes(), args.dat), indent=2))


if __name__ == '__main__':
    main()

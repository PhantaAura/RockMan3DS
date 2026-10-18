#!/usr/bin/env python3
"""Bounded owner-instruction type1 RECT/sector proof; scalar output only.

No owner instruction/data payload is written. Authored image headers/work state
exercise unchanged source instructions. LoadImage is an explicit observation
boundary, then execution resumes at its existing return site; no GPU claim.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415'),
               str(SOURCE / 'docs/research_01414')]
from cook_actor_combat import OwnerExe
from tutorial_actor_identity_probe import OverlayProbe


def prove(image):
    owner = OwnerExe(image)
    base = struct.unpack_from('<I', image, 24)[0]
    dispatch = struct.unpack('<I', owner.read(0x8001025C + 2 * 4, 4))[0]
    header_dispatch = struct.unpack('<I', owner.read(0x80010284 + 4, 4))[0]
    assert dispatch == 0x8001C350 and header_dispatch == 0x8001BD98
    cpu = OverlayProbe(image, base, b'')
    header, metadata, rect = 0x70000000, 0x800C560C, 0x8009BE48
    sector_base, sector_counter, loader_stage = 0x800B5DB0, 0x800987A8, 0x800989C8
    total_steps = 0
    cases = []
    for width, height in ((32, 32), (64, 32), (32, 64), (64, 64), (64, 256), (128, 64)):
        cpu.r = [0] * 32
        cpu.r[18], cpu.r[19], cpu.r[22] = metadata, 0x800C5604, 1
        cpu.r[16] = cpu.r[6] = header
        # -1 is the original explicit no-CLUT-copy branch; the source image
        # rectangle/header/counter path is still executed without hooks.
        for offset, value in ((12, 0xFFFFFFFF), (16, 240), (20, 16), (24, 1),
                              (28, 320), (32, 0), (36, width), (40, height)):
            cpu.put(header + offset, 4, value)
        cpu.put(0x80098A54, 4, 1)  # Source resource type, copied before header dispatch.
        before = cpu.steps
        cpu.run(header_dispatch, 0x8001C740)
        assert cpu.get(loader_stage, 4) == 2
        assert cpu.get(rect + 4, 2) == 32 and cpu.get(rect + 6, 2) == 32
        total_steps += cpu.steps - before
        tiles = []
        count = (width // 32) * (height // 32)
        for sector in range(count):
            cpu.put(sector_counter, 4, sector + 1)  # Header sector precedes image sectors.
            cpu.r[21] = sector_base
            before = cpu.steps
            cpu.run(dispatch, 0x8007D2F0)
            values = [cpu.get(cpu.r[4] + i * 2, 2) for i in range(4)]
            expected = [320 + (sector % (width // 32)) * 32,
                        (sector // (width // 32)) * 32, 32, 32]
            assert values == expected
            assert cpu.r[5] == sector_base + (sector + 1) * 2048
            tiles.append({'sector': sector + 1, 'destination_rect_words': values,
                          'source_buffer_offset': (sector + 1) * 2048})
            # LoadImage itself is outside this CPU/GPU proof. No instruction is
            # overwritten; observe call arguments and resume after its return.
            cpu.run(0x8001C3AC, 0x8001C740)
            total_steps += cpu.steps - before
        cases.append({'width_words': width, 'height': height, 'uploads': tiles})
    return {'format': 'mml-type1-sector-source-proof/1',
            'status': 'SOURCE_BACKED_PARTIAL',
            'owner_executable_sha256': hashlib.sha256(image).hexdigest(),
            'resource_header_dispatch': hex(header_dispatch),
            'image_stage_dispatch': hex(dispatch), 'load_image_boundary': '0x8007D2F0',
            'resume_site': '0x8001C3AC', 'instructions_replaced': 0,
            'helper_hooks': cpu.return_only_hooks, 'authored_cases': len(cases),
            'original_instructions_executed': total_steps, 'cases': cases,
            'payload_layout': '2048B sectors;32VRAMwords*32rows;X tiles beforeY tiles',
            'limits': ['Authored loader work/header/sector counters, not live retail chronology.',
                       'Original no-CLUT-copy branch isolates image layout; palette behavior is not retested.',
                       'GPU LoadImage boundary is observed, not executed or emulated.',
                       'No owner instruction/asset bytes, retail save data or cooked payload are written.',
                       'No full renderer/material/animation/hardware parity claim.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(prove(args.exe.read_bytes()), indent=2))


if __name__ == '__main__':
    main()

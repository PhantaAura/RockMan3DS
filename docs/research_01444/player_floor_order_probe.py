#!/usr/bin/env python3
"""Verify the original player floor-adhesion block on authored scalar inputs.

Reads the owner's supported executable in place. Outputs only identities,
scalar observations and source addresses; no binary or instruction payloads.
The floor-query return is an explicit authored boundary, not a live trace.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research_01414'))
from player_contact_source_probe import CPU, SUPPORTED_EXE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    image = args.exe.read_bytes()
    digest = hashlib.sha256(image).hexdigest()
    if digest != SUPPORTED_EXE:
        raise ValueError('Unsupported owner executable identity')
    base = struct.unpack_from('<I', image, 24)[0]
    # Y-down original fields: y, explicit floor return, resulting y, contact,
    # action and phase. No renderer or original instruction is replaced.
    cases = ((-8, -512, -8, 0, 0, 0), (-8, 0, 0, 0, 0, 0),
             (-40, 0, -40, 1, 7, 4), (-32, 0, 0, 0, 0, 0),
             (-16, 0, 0, 0, 0, 0), (0, 0, 0, 0, 0, 0),
             (-1, 32767, -1, 1, 7, 4))
    observations = []
    for y, floor, expected_y, contact, action, phase in cases:
        cpu = CPU(image, base)
        work, bounds = 0x1000, 0x2000
        cpu.r[29], cpu.r[17] = 0x4000, work
        cpu.r[16], cpu.r[2] = y & 0xFFFFFFFF, floor & 0xFFFFFFFF
        cpu.put(work + 0x16, 2, y)
        cpu.put(work + 0x78, 4, bounds)
        cpu.put(bounds + 10, 2, 0)
        cpu.run(0x8003C61C, 0x8003C6FC)
        actual = (cpu.get(work + 0x16, 2, True), cpu.get(work + 0x74, 1),
                  cpu.get(work + 9, 1), cpu.get(work + 10, 1))
        if actual != (expected_y, contact, action, phase):
            raise ValueError(f'Original scalar response mismatch: {y}, {floor}, {actual}')
        observations.append(dict(input_y=y, floor_return=floor, output_y=actual[0],
                                 contact=actual[1], action=actual[2], phase=actual[3],
                                 original_instructions=cpu.steps))
    # The immediately following original instruction is JAL 8004B414. Retain
    # only its decoded destination and boolean check, never the instruction.
    call = struct.unpack_from('<I', image, 0x8003C6FC - base + 2048)[0]
    destination = 0x80000000 | ((call & 0x03FFFFFF) << 2)
    if call >> 26 != 3 or destination != 0x8004B414:
        raise ValueError('Original post-adhesion contact call identity differs')
    proof = dict(format='mml-player-floor-order/1', status='SOURCE_BACKED_PARTIAL',
                 PHYSICS_1TO1=False, exe_sha256=digest,
                 source_block='0x8003C61C..0x8003C6FC',
                 block_sha256=hashlib.sha256(image[0x8003C61C-base+2048:
                                                 0x8003C6FC-base+2048]).hexdigest(),
                 subsequent_contact_call='0x8003C6FC -> 0x8004B414',
                 floor_return_explicit_hook=True, bounds_y1=0,
                 cases=observations,
                 limits=['Authored scalar floor returns; not live game chronology.',
                         'No full movement, collision, slope or hardware parity claim.',
                         'No hidden barrier or retail recovery behavior inferred.'])
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(proof, indent=2) + '\n')
    print(f'PASS: {len(cases)} original floor-adhesion scalar cases; contact call follows adhesion')


if __name__ == '__main__':
    main()

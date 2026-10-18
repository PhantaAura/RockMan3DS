#!/usr/bin/env python3
"""Run unchanged 80015EE8 on authored scalar owners; export no game payloads.

A0 is the player owner, A1 is the transition structure at 800C3558, and A2 is
the original caller's argument6. The fixture contains only authored player and
transition values and resulting scalar stores. No helper or instruction hooks.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01422')]
from cook_actor_combat import OwnerExe, OWNER_SHA
from class60_06_source_probe import FastProbe

PLAYER, TRANSITION, SIZE = 0x800b51b0, 0x800c3558, 1184
PREMISES = (
    ((-32768, -1, 32767), 0), ((0, 1, -1), 1),
    ((32767, -32768, 0), 0x7fff), ((1, 2, 3), 0x8000),
    ((-1, -2, -3), 0xffff), ((2822, -1, -4833), 0xfff),
    ((-32767, 32766, -16384), 0x1000), ((12345, -23456, 30000), 0xdead),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def authored(seed, size):
    return bytes((seed * 37 + i * 17 + 11) & 255 for i in range(size))


def fill(cpu, address, data):
    for i, value in enumerate(data):
        cpu.put(address + i, 1, value)


def blob(cpu, address, size):
    return bytes(cpu.get(address + i, 1) for i in range(size))


class SpawnProbe(FastProbe):
    def put(self, address, size, value):
        if getattr(self, 'record_stores', False):
            if not PLAYER <= address or address + size > PLAYER + SIZE:
                raise ValueError('Spawn wrote outside the authored player owner')
            self.stores.append((address - PLAYER, size))
        return super().put(address, size, value)


def expected_player(before, raw, argument6):
    result = bytearray(before)
    for axis in range(3):
        bits = struct.unpack_from('<H', raw, axis * 2)[0]
        for offset in (0x14 + axis * 2, 0x2c + axis * 2):
            struct.pack_into('<H', result, offset, bits)
        for offset in (0x1c + axis * 4, 0x34 + axis * 4):
            struct.pack_into('<I', result, offset, bits << 16)
    for offset in (0x56, 0x5e):
        result[offset:offset + 2] = raw[10:12]
    if argument6:
        result[0x74:0x76] = b'\0\0'
    if result[8] == 3:
        for target, source, size in ((0x438, 0x14, 8), (0x428, 0x1c, 16), (0x440, 0x54, 8)):
            result[target:target + size] = result[source:source + size]
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'out'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    if output.exists() or output == SOURCE or SOURCE in output.parents:
        raise ValueError('Choose a new private output outside source')
    image = args.exe.read_bytes()
    owner = OwnerExe(image)
    rows, instructions, stores, touched = [], 0, 0, set()
    for case, (state, argument6, premise) in enumerate(itertools.product((0, 1, 3, 255), (0, 1), PREMISES)):
        cpu = SpawnProbe(image, owner.base, b'')
        before = bytearray(authored(case, SIZE))
        before[8] = state
        raw = bytearray(authored(case + 19, 18))
        xyz, heading = premise
        struct.pack_into('<hhh', raw, 0, *xyz)
        struct.pack_into('<H', raw, 10, heading)
        fill(cpu, PLAYER, before)
        fill(cpu, TRANSITION + 8, raw)
        cpu.r[4], cpu.r[5], cpu.r[6] = PLAYER, TRANSITION, argument6
        cpu.r[29], cpu.r[31] = 0x700000, 0
        cpu.stores, cpu.record_stores = [], True
        cpu.run(0x80015ee8)
        if cpu.return_only_hooks or cpu.calls or cpu.indirect:
            raise ValueError('Spawn leaf unexpectedly called a helper')
        after = blob(cpu, PLAYER, SIZE)
        if after != expected_player(before, raw, argument6):
            raise ValueError('Original instruction result differs from bounded spawn stores')
        if after[8] != state:
            raise ValueError('Spawn changed the retained player state')
        instructions += cpu.steps
        stores += len(cpu.stores)
        for offset, size in cpu.stores:
            touched.update(range(offset, offset + size))
        rows.append(f'S {case} {argument6} {state} {case} {bytes(raw).hex()} {after.hex()}')
    fixture = ('\n'.join(rows) + '\n').encode()
    proof = {
        'schema': 'mml-source-handoff-spawn/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'source_helper': '80015EE8', 'source_exe_sha256': OWNER_SHA,
        'authored_cases': len(rows), 'original_instructions': instructions,
        'original_store_instructions': stores, 'distinct_player_bytes_written': len(touched),
        'fixture_sha256': sha(fixture), 'helper_hooks': 0, 'instruction_patches': 0,
        'retained_player_state_preserved': True,
        'xyz_lanes': [0, 2, 4], 'heading_lane': 10,
        'argument6_clears_player_offsets': [0x74, 0x75],
        'state3_copy_ranges': [[0x438, 0x14, 8], [0x428, 0x1c, 16], [0x440, 0x54, 8]],
        'asset_payloads_written': False, 'retail_parity': False, 'physical_hardware_tested': False,
        'limits': [
            'Authored inputs prove the complete bounded player-store leaf, not live source chronology.',
            'New-area callers pass argument6=1; same-area 80015ACC passes argument6=0.',
            'Area ownership, CD callback execution and transport completion remain separate consumers.',
            'The helper preserves player08 and does not establish a runnable player state.']}
    output.mkdir(parents=True)
    (output / 'handoff-spawn-authored.tsv').write_bytes(fixture)
    (output / 'source-handoff-spawn-proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps(proof, indent=2))


if __name__ == '__main__':
    main()

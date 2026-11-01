#!/usr/bin/env python3
"""Cook a stage's background-layer descriptors into neutral L3BG.

Reads a hash-guarded owner overlay: R3_ST01 (layer table 0x80100D5C[area])
or r3_st07 (0x80105658[area]); each table holds 11 pointers, the 0x80022FD4
input of the stage init. Each present descriptor
keeps its 11 init halfwords (p+0..p+20) and its width*height tile words
(from p+32). Also prints the (CLUT, tpage) material words the tiles use,
one hex word per line, for l3cook_intro_materials. Output is owner-local
data, never part of the source release.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zlib

OVERLAY_SHA = '96bb58fb3adbd0ecc3d2e75e42d2672a71be378a2b0b03e2e38db29d38aa8fac'
# Stage -> (overlay SHA-256, area-indexed table of layer tables).
STAGES = {'st01': (OVERLAY_SHA, 0x80100D5C),
          'st07': ('10cccd074b1e941b348fe152df05d8272e280ecbc3ed8fb02aeab08017bfa565', 0x80105658)}
BASE = 0x80100000
LAYERS = 11
MAX_CELLS = 512


def cook(overlay, stage='st01', area=0):
    sha, tables = STAGES[stage]
    if hashlib.sha256(overlay).hexdigest() != sha:
        raise ValueError(f'Unsupported owner {stage.upper()} overlay identity')
    def word(a): return struct.unpack_from('<I', overlay, a - BASE)[0]
    def half(a): return struct.unpack_from('<H', overlay, a - BASE)[0]
    table = word(tables + 4 * area)
    payload = bytes.fromhex(sha)
    words = set()
    for i in range(LAYERS):
        p = word(table + 4 * i)
        if not p:
            payload += struct.pack('<B3x', 0) + bytes(22 + 4 * MAX_CELLS) + bytes(2)
            continue
        init = [half(p + 2 * k) for k in range(11)]
        cells = init[1] * init[2]
        if not 0 < cells <= MAX_CELLS: raise ValueError('Background layer map size unsupported')
        tiles = [word(p + 32 + 4 * k) for k in range(cells)]
        for t in tiles:   # 0x80023554: tile word + (+0x28 << 14); CLUT is its high half
            words.add(((((t + (init[4] << 14)) & 0xFFFFFFFF) >> 16) << 16) | init[3])
        payload += struct.pack('<B3x', 1) + struct.pack('<11H', *init) + b''.join(struct.pack('<I', t) for t in tiles)
        payload += bytes(4 * (MAX_CELLS - cells)) + bytes(2)
    package = struct.pack('<4sHHII', b'L3BG', 2, 16, 16 + len(payload), zlib.crc32(payload) & 0xFFFFFFFF) + payload
    return package, sorted(words)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--overlay', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--words', required=True, type=Path)
    parser.add_argument('--stage', choices=sorted(STAGES), default='st01')
    parser.add_argument('--area', type=int, default=0)
    args = parser.parse_args()
    package, words = cook(args.overlay.read_bytes(), args.stage, args.area)
    args.out.write_bytes(package)
    args.words.write_text(''.join(f'{w:08x}\n' for w in words))


if __name__ == '__main__':
    main()

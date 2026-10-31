#!/usr/bin/env python3
"""Cook the ST00 family-C0 palette tables (subtypes 3/4) and the E0 variant-4
UV frame table (0x8011CA94) into neutral L3C0.

Reads the hash-guarded owner overlay: subtype-3 timelines 0x8011CF28[v] and
destination/mode pairs 0x8011CF38[v]; subtype-4 timelines 0x8011D10C[v] and
{destination, count, mode} records 0x8011D178[v]. Each timeline keeps its
4-byte entries up to and including the first negative control byte. Output
is owner-local data, never part of the source release.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zlib

OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
BASE = 0x80100000
CAPACITY = 16


def cook(overlay):
    if hashlib.sha256(overlay).hexdigest() != OVERLAY_SHA:
        raise ValueError('Unsupported owner ST00 overlay identity')
    def word(a): return struct.unpack_from('<I', overlay, a - BASE)[0]
    def half(a): return struct.unpack_from('<H', overlay, a - BASE)[0]
    def timeline(pointer):
        entries = []
        for k in range(CAPACITY):
            e = overlay[pointer - BASE + 4 * k:pointer - BASE + 4 * k + 4]
            if len(e) != 4: raise ValueError('Truncated palette timeline')
            entries.append(e)
            if struct.unpack('b', e[3:4])[0] < 0: break
        else:
            raise ValueError('Palette timeline exceeds capacity')
        body = b''.join(entries).ljust(4 * CAPACITY, b'\0')
        return struct.pack('<IB3x', pointer, len(entries)) + body
    payload = bytes.fromhex(OVERLAY_SHA)
    payload += b''.join(timeline(word(0x8011CF28 + 4 * v)) for v in range(4))
    payload += b''.join(timeline(word(0x8011D10C + 4 * v)) for v in range(6))
    payload += b''.join(struct.pack('<HH', half(0x8011CF38 + 4 * v), half(0x8011CF3A + 4 * v)) for v in range(4))
    payload += b''.join(struct.pack('<HHH', half(0x8011D178 + 6 * v), half(0x8011D17A + 6 * v), half(0x8011D17C + 6 * v)) for v in range(6))
    # 0x8011CA94: six (u, v) frame bases of the E0 variant-4 UV animation 0x8011381C.
    payload += overlay[0x8011CA94 - BASE:0x8011CA94 - BASE + 12]
    assert len(payload) == 32 + 10 * (8 + 4 * CAPACITY) + 16 + 36 + 12
    return b'L3C0' + struct.pack('<HHII', 1, 16, 16 + len(payload), zlib.crc32(payload) & 0xffffffff) + payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--overlay', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite cooked palette profile')
    args.out.write_bytes(cook(args.overlay.read_bytes()))


if __name__ == '__main__':
    main()

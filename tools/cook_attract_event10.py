#!/usr/bin/env python3
"""Cook the ST07 event-10 (title attract) tables into neutral L3AT.

Reads the hash-guarded owner overlay r3_st07: the phase-0 and main timelines
(0x80105E1C, 0x801060D4; 8-byte records {u16 event+8, u16 ticks, callback}
up to the 0xFF terminator), the two camera programs (0x80105DD0,
0x80105E2C; words up to 0xFFFFFFFF) and the eight background-layer rows the
phase-0 setup writes (0x80106124; {u8 flags, 3 pad, s16 vx, s16 vy}).
Output is owner-local data, never part of the source release.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zlib

OVERLAY_SHA = '10cccd074b1e941b348fe152df05d8272e280ecbc3ed8fb02aeab08017bfa565'
BASE = 0x80100000
TIMELINES = (0x80105E1C, 0x801060D4)
CAMERAS = (0x80105DD0, 0x80105E2C)
BG_ROWS = 0x80106124
MAX_RECORDS, MAX_WORDS = 16, 256


def cook(overlay):
    if hashlib.sha256(overlay).hexdigest() != OVERLAY_SHA:
        raise ValueError('Unsupported owner ST07 overlay identity')
    def word(a): return struct.unpack_from('<I', overlay, a - BASE)[0]
    payload = bytes.fromhex(OVERLAY_SHA)
    for t in TIMELINES:
        records = []
        for k in range(MAX_RECORDS):
            header, callback = word(t + 8 * k), word(t + 8 * k + 4)
            records.append((header, callback))
            if header & 0xFF == 0xFF: break
        else:
            raise ValueError('Timeline exceeds capacity')
        payload += struct.pack('<II', t, len(records))
        payload += b''.join(struct.pack('<II', h, c) for h, c in records) + bytes(8 * (MAX_RECORDS - len(records)))
    for c in CAMERAS:
        words = []
        for k in range(MAX_WORDS):
            words.append(word(c + 4 * k))
            if words[-1] == 0xFFFFFFFF: break
        else:
            raise ValueError('Camera program exceeds capacity')
        payload += struct.pack('<II', c, len(words)) + b''.join(struct.pack('<I', w) for w in words)
        payload += bytes(4 * (MAX_WORDS - len(words)))
    payload += overlay[BG_ROWS - BASE:BG_ROWS - BASE + 64]
    return struct.pack('<4sHHII', b'L3AT', 1, 16, 16 + len(payload), zlib.crc32(payload) & 0xFFFFFFFF) + payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--overlay', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    args.out.write_bytes(cook(args.overlay.read_bytes()))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Cook the ST00 E0/06 tutorial trigger lists (0x8011CC9C[area]) into neutral L3TR/1.

Reads the owner's ST00 overlay (R3_ST00.BIN, loaded at 0x80100000). Each
16-byte record keeps only its type, message block, cell corner and size;
runtime state bytes (+8..) are not copied.
"""
import argparse, hashlib, struct, zlib
from pathlib import Path

OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
BASE = 0x80100000
LISTS = 0x8011CC9C
AREAS = 5
MAX_RECORDS = 32

def cook(overlay: bytes) -> bytes:
    def u32(address):
        offset = address - BASE
        if offset < 0 or offset + 4 > len(overlay): raise ValueError('address outside overlay')
        return struct.unpack_from('<I', overlay, offset)[0]
    payload = bytearray(); total = 0
    for area in range(AREAS):
        pointer = u32(LISTS + area * 4)
        records = []
        if BASE <= pointer < BASE + len(overlay):
            at = pointer - BASE
            while overlay[at] != 0xFF:
                if len(records) >= MAX_RECORDS: raise ValueError('trigger list too long')
                t, msg, cx, cz = overlay[at], overlay[at + 1], overlay[at + 2], overlay[at + 3]
                w, d = struct.unpack_from('<HH', overlay, at + 4)
                records.append(struct.pack('<BBBBHH', t, msg, cx, cz, w, d)); at += 16
        payload += struct.pack('<H', len(records)) + b''.join(records); total += len(records)
    header = bytearray(32); header[:4] = b'L3TR'
    struct.pack_into('<HHIIII', header, 4, 1, 32, 32 + len(payload), AREAS, total, zlib.crc32(payload) & 0xffffffff)
    return bytes(header + payload)

def main():
    p = argparse.ArgumentParser(); p.add_argument('--overlay', required=True, type=Path); p.add_argument('--out', required=True, type=Path)
    a = p.parse_args(); data = a.overlay.read_bytes()
    if hashlib.sha256(data).hexdigest() != OVERLAY_SHA: raise ValueError('unsupported ST00 overlay')
    if a.out.exists(): raise FileExistsError('refusing to overwrite cooked tutorial triggers')
    out = cook(data); a.out.parent.mkdir(parents=True, exist_ok=True); a.out.write_bytes(out)
    print('schema=L3TR/1 bytes=%d source=%08x' % (len(out), LISTS))

if __name__ == '__main__': main()

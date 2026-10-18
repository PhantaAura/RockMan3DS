#!/usr/bin/env python3
"""Decode phase-two command scalars and measured LOAD snapshots locally.

Never emit executable instructions, samples, media or retail saves. The output
is owner-derived metadata and must stay outside the code-only source release.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
REQUEST = 0x20000003


def cook(image):
    if hashlib.sha256(image).hexdigest() != EXE_SHA:
        raise ValueError('Unsupported owner executable identity')
    if image[:8] != b'PS-X EXE':
        raise ValueError('Owner executable header rejected')
    base, size = struct.unpack_from('<II', image, 0x18)
    if len(image) < 2048 + size:
        raise ValueError('Truncated executable LOAD image')

    def read(address, count):
        if address < base or address + count > base + size:
            raise ValueError('Source pointer leaves the measured LOAD image')
        offset = 2048 + address - base
        return image[offset:offset + count]

    def u32(address):
        return struct.unpack('<I', read(address, 4))[0]

    bank, entry, index = (REQUEST >> 28) & 7, (REQUEST >> 16) & 255, REQUEST & 65535
    id_table = u32(0x80088F9C + bank * 4)
    pair_table = u32(0x80088EF8 + bank * 4)
    command_id = struct.unpack('<H', read(id_table + entry * 2, 2))[0]
    pair_pointer = u32(pair_table + entry * 4)
    words = struct.unpack('<II', read(pair_pointer + index * 8, 8))
    descriptors = read(0x80098AF8, 32)
    command_records = read(0x800A3A40, 256)
    if any(descriptors) or any(command_records):
        raise ValueError('Unsupported retained cold-boot snapshot')
    payload = struct.pack('<IIII', REQUEST, command_id, *words) + descriptors + command_records
    header = struct.pack('<4sHH6I32s', b'L3EQ', 1, 64, 1, 0, len(payload),
                         64 + len(payload), zlib.crc32(payload), 0, bytes.fromhex(EXE_SHA))
    proof = {'schema': 'mml-event-phase2-cook/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'source_sha256': EXE_SHA, 'request': hex(REQUEST),
             'pointer_chain': [hex(id_table), hex(pair_table), hex(pair_pointer)],
             'cold_boot_snapshot_addresses': ['0x80098AF8:32', '0x800A3A40:256'],
             'cold_boot_data_measured_in_load_image': True,
             'scene_reset_is_not_cold_boot': True,
             'cooked_bytes': len(header + payload),
             'owner_payload_release_permitted': False, 'retail_parity': False}
    return header + payload, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite cooked phase2 profile')
    blob, proof = cook(args.exe.read_bytes())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(blob)
    proof['cooked_sha256'] = hashlib.sha256(blob).hexdigest()
    print(json.dumps(proof, indent=2))


if __name__ == '__main__':
    main()

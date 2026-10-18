#!/usr/bin/env python3
"""Retain bounded source lift offset windows from an existing owner overlay.

This does not unpack a disc or extract graphics/audio. Its output is owner-local
game data, never part of the source release. Unchanged cooked assets are reused.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zlib

EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
EBD_SHA = 'ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15'
PAYLOAD_SHA = 'cb7f79c1f39c21a17b69dd55f71a149dd2f7429acd5e833e64fb15d1c2f96a54'
METADATA = (0x8011c9fc, 0x8011c958, 0x8011c998, 0x8011c9d8,
            0x80113280, 442, 2, 0)


def validate_package(data):
    if (len(data) != 384 or struct.unpack_from('<4sHH6I', data) !=
            (b'L3PL', 1, 160, 1, 3, 64, 384,
             zlib.crc32(data[32:]) & 0xffffffff, 0)):
        raise ValueError('L3PL header/CRC rejected')
    if data[32:128] != bytes.fromhex(EXE_SHA + OVERLAY_SHA + EBD_SHA):
        raise ValueError('L3PL source identities rejected')
    if struct.unpack_from('<8I', data, 128) != METADATA:
        raise ValueError('L3PL source owner metadata rejected')
    if any(data[352:]):
        raise ValueError('L3PL reserved bytes rejected')
    if hashlib.sha256(data[160:352]).hexdigest() != PAYLOAD_SHA:
        raise ValueError('L3PL scalar payload identity rejected')
    return {'format': 1, 'bytes': len(data),
            'sha256': hashlib.sha256(data).hexdigest(),
            'payload_sha256': PAYLOAD_SHA,
            'scope': 'Source lift offset windows; not live RNG or rendering'}


def cook(overlay):
    if hashlib.sha256(overlay).hexdigest() != OVERLAY_SHA:
        raise ValueError('Existing owner overlay identity rejected')
    data = bytearray(384)
    struct.pack_into('<4sHH6I', data, 0, b'L3PL', 1, 160, 1, 3, 64, 384, 0, 0)
    data[32:128] = bytes.fromhex(EXE_SHA + OVERLAY_SHA + EBD_SHA)
    struct.pack_into('<8I', data, 128, *METADATA)
    pointers = struct.unpack_from('<3I', overlay, 0x1c9fc)
    if pointers != METADATA[1:4]:
        raise ValueError('Source lift pointer table rejected')
    # Explicit caller-indexed windows; no next-pointer extent inference.
    for destination, source, count in ((160 + i * 64, token, 64)
                                       for i, token in enumerate(pointers)):
        at = source - 0x80100000
        if at < 0 or at + count > len(overlay):
            raise ValueError('Source scalar range outside overlay')
        data[destination:destination + count] = overlay[at:at + count]
    struct.pack_into('<I', data, 24, zlib.crc32(data[32:]) & 0xffffffff)
    validate_package(data)
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--overlay', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists() or args.out.is_symlink():
        raise ValueError('Choose a new owner-local output path')
    data = cook(args.overlay.read_bytes())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open('xb') as stream:
        stream.write(data)
    print('384-byte L3PL lift profile validated; owner-local output only')


if __name__ == '__main__':
    main()

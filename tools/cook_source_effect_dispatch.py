#!/usr/bin/env python3
"""Retain bounded scalar dispatch/control words from an existing owner overlay.

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
PAYLOAD_SHA = '6015208c30d7c82fa3ce30147d74697b65842be15a220278178565aea2295ee3'
METADATA = (0x8011a604, 0x8011cd74, 0x8011cda8, 0x8011cdcc,
            0x1f800064, 0x800172f4, 72, 64)


def validate_package(data):
    if (len(data) != 384 or struct.unpack_from('<4sHH6I', data) !=
            (b'L3E2', 1, 160, 1, 16, 8, 384,
             zlib.crc32(data[32:]) & 0xffffffff, 0)):
        raise ValueError('L3E2 header/CRC rejected')
    if data[32:128] != bytes.fromhex(EXE_SHA + OVERLAY_SHA + EBD_SHA):
        raise ValueError('L3E2 source identities rejected')
    if struct.unpack_from('<8I', data, 128) != METADATA:
        raise ValueError('L3E2 source owner metadata rejected')
    if any(data[278:280]) or any(data[300:]):
        raise ValueError('L3E2 reserved bytes rejected')
    if hashlib.sha256(data[160:300]).hexdigest() != PAYLOAD_SHA:
        raise ValueError('L3E2 scalar payload identity rejected')
    return {'format': 1, 'bytes': len(data),
            'sha256': hashlib.sha256(data).hexdigest(),
            'payload_sha256': PAYLOAD_SHA,
            'scope': 'Scalar source dispatch/control; not RNG chronology or rendering'}


def cook(overlay):
    if hashlib.sha256(overlay).hexdigest() != OVERLAY_SHA:
        raise ValueError('Existing owner overlay identity rejected')
    data = bytearray(384)
    struct.pack_into('<4sHH6I', data, 0, b'L3E2', 1, 160, 1, 16, 8, 384, 0, 0)
    data[32:128] = bytes.fromhex(EXE_SHA + OVERLAY_SHA + EBD_SHA)
    struct.pack_into('<8I', data, 128, *METADATA)
    for destination, source, count in [(160, 0x8011a604, 64),
                                       (224, 0x8011cd74, 32),
                                       (256, 0x8011cda8, 22),
                                       (280, 0x8011cdcc, 20)]:
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
    print('384-byte L3E2 scalar profile validated; owner-local output only')


if __name__ == '__main__':
    main()

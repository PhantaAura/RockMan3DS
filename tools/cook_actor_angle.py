#!/usr/bin/env python3
"""Cook optional actor angle reference from an already extracted owner executable.

No original constants are embedded in this tool. The owner-derived output must
remain outside the source tree/source release. Earlier outputs are never replaced.
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

OWNER_EXE_SHA256 = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OWNER_TABLE_SHA256 = '81270be1526005e5347e7dc972ec5ed38c19ad1fa914885dadcf6a0eca4f9447'
OWNER_TABLE_ADDRESS = 0x80092b64
SOURCE = Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def cook(image):
    if sha(image) != OWNER_EXE_SHA256:
        raise ValueError('Unsupported owner executable profile')
    if image[:8] != b'PS-X EXE' or struct.unpack_from('<I', image, 24)[0] != 0x80010000:
        raise ValueError('Unsupported executable load address')
    offset = OWNER_TABLE_ADDRESS - 0x80010000 + 0x800
    payload = image[offset:offset + 48]
    if len(payload) != 48 or sha(payload) != OWNER_TABLE_SHA256:
        raise ValueError('Owner angle table profile mismatch')
    header = struct.pack('<4sHHIIIIII', b'L3AH', 1, 32, 80, 12,
                         zlib.crc32(payload) & 0xffffffff, 0, 0, 0)
    return header + payload


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--evidence', type=Path)
    a = ap.parse_args()
    if a.out.resolve().is_relative_to(SOURCE.resolve()):
        raise SystemExit('Owner-derived angle payload must remain outside source tree')
    if a.out.exists() or (a.evidence is not None and a.evidence.exists()):
        raise SystemExit('Refusing to replace an earlier output')
    package = cook(a.exe.read_bytes())
    a.out.parent.mkdir(parents=True, exist_ok=True)
    with a.out.open('xb') as file:
        file.write(package)
    result = {'schema': 'actor-angle-cook/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'owner_exe_sha256': OWNER_EXE_SHA256, 'owner_table_address': hex(OWNER_TABLE_ADDRESS),
              'owner_table_sha256': OWNER_TABLE_SHA256, 'package_sha256': sha(package),
              'package_bytes': len(package), 'output': str(a.out.resolve()),
              'original_table_values_embedded_in_source': False,
              'limits': ['Optional native input, not full AI or retail gameplay parity.',
                         'Owner payload must be excluded from code-only archives.']}
    if a.evidence is not None:
        a.evidence.parent.mkdir(parents=True, exist_ok=True)
        with a.evidence.open('x') as file:
            file.write(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()

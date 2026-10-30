#!/usr/bin/env python3
"""Cook the title Servbot parade scripts from the owner's ROCK_NEO.EXE (USA).

The front end's spawner 0x80015428 gives each of up to 16 class-20 actors a
script pointer from the wave table 0x800820E0 (9 waves x 16 entries); the
scripts are halfword streams read by 0x80014BCC. Both live in the EXE data
block 0x80080960..0x80082104, which this tool copies verbatim:

  L3TS0001 | u32 base | u32 size | u32 crc32(data) | data

Output holds owner-derived data and MUST stay outside the source release.
"""
import argparse
import hashlib
import struct
import zlib
from pathlib import Path

EXE_SHA = "10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
BASE, END = 0x80080960, 0x80082104


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXE_SHA:
        raise SystemExit("unsupported ROCK_NEO.EXE (USA digest expected)")
    text = struct.unpack_from("<I", image, 0x18)[0]
    data = image[0x800 + BASE - text:0x800 + END - text]
    if args.out.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise SystemExit("owner-derived output must be outside the source tree")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(b"L3TS0001" + struct.pack("<III", BASE, len(data), zlib.crc32(data)) + data)
    print(f"base={BASE:#x} bytes={len(data)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

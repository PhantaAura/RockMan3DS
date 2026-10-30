#!/usr/bin/env python3
"""Cook the title Servbot parade scripts from the owner's ROCK_NEO.EXE (USA).

The front end's spawner 0x80015428 gives each of up to 16 class-20 actors a
script pointer from the wave table 0x800820E0 (9 waves x 16 entries); the
scripts are halfword streams read by 0x80014BCC. Both live in the EXE data
block 0x80080960..0x80082104, which this tool copies verbatim:

  L3TS0002 | u32 base | u32 size | u32 crc32(data + camera) | data | camera

The camera block is 0x80038370 evaluated on the title's two constant points
(0x800137D4 passes a0 = 0x800979DC, the look-at target, and a1 = 0x800979EC):
i32 target x/y/z, then the distance, yaw and pitch arguments it hands to
0x80037FB4. Its two table helpers, SquareRoot0 0x8007A578 (table 0x80092BA4)
and ratan2 0x8007BFA8 (table 0x80096EC4), run here on the EXE's own tables,
branch for branch.

Output holds owner-derived data and MUST stay outside the source release.
"""
import argparse
import hashlib
import struct
import zlib
from pathlib import Path

EXE_SHA = "10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
BASE, END = 0x80080960, 0x80082104


def s32(v: int) -> int:
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def div(a: int, b: int) -> int:
    """MIPS div: truncate toward zero."""
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b >= 0) else -q


def make_reader(image: bytes, text: int):
    def h(addr: int) -> int:
        return struct.unpack_from("<h", image, 0x800 + addr - text)[0]
    def w(addr: int) -> int:
        return struct.unpack_from("<i", image, 0x800 + addr - text)[0]
    return h, w


def square_root0(h, a0: int) -> int:
    # 0x8007A578: LZCS/LZCR, then table 0x80092BA4.
    lz = 32 - (a0 & 0xFFFFFFFF).bit_length() if a0 >= 0 else 32 - ((~a0) & 0xFFFFFFFF).bit_length()
    if lz == 32:
        return 0
    t2 = lz & ~1
    t1 = (31 - t2) >> 1
    t4 = (a0 << (t2 - 24)) if t2 - 24 >= 0 else (a0 >> (24 - t2))
    t5 = h(0x80092BA4 + (t4 - 0x40) * 2)
    return ((t5 << t1) & 0xFFFFFFFF) >> 12


def ratan2(h, a0: int, a1: int) -> int:
    # 0x8007BFA8 (a0 = y, a1 = x), table 0x80096EC4.
    neg_x = a1 < 0
    neg_y = a0 < 0
    if neg_x:
        a1 = -a1
    if neg_y:
        a0 = -a0
    if a1 == 0 and a0 == 0:
        return 0
    if a0 < a1:
        q = div(a0, a1 >> 10) if a0 & 0x7FE00000 else div(a0 << 10, a1)
        v = h(0x80096EC4 + q * 2)
    else:
        q = div(a1, a0 >> 10) if a1 & 0x7FE00000 else div(a1 << 10, a0)
        v = 0x400 - h(0x80096EC4 + q * 2)
    if neg_x:
        v = 0x800 - v
    return -v if neg_y else v


def camera_80038370(h, w, a0: int, a1: int):
    target = [w(a0), w(a0 + 4), w(a0 + 8)]
    other = [w(a1), w(a1 + 4), w(a1 + 8)]
    s0 = s32((target[0] - other[0]) ** 2 + (target[2] - other[2]) ** 2)
    s1 = square_root0(h, s0)
    if s1:
        s1 = (div(s0, s1) + s1) >> 1
    yaw = ratan2(h, other[0] - target[0], other[2] - target[2]) + 0x800
    pitch = -ratan2(h, other[1] - target[1], s1)
    return target, s1, yaw, pitch


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
    h, w = make_reader(image, text)
    target, distance, yaw, pitch = camera_80038370(h, w, 0x800979DC, 0x800979EC)
    camera = struct.pack("<6i", *target, distance, yaw, pitch)
    args.out.write_bytes(b"L3TS0002" + struct.pack("<III", BASE, len(data), zlib.crc32(data + camera)) + data + camera)
    print(f"base={BASE:#x} bytes={len(data)} camera target={target} distance={distance} yaw={yaw:#x} pitch={pitch}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

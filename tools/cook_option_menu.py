#!/usr/bin/env python3
"""Cook the OPTION menu's (and in-game sub-screen's) message data from the owner's ROCK_NEO.EXE (USA).

The front end's OPTION state (0x80013E0C) opens message window 0 with the
data at 0x8008B69C (0x8005382C(0, 0x8008B69C, -1)). That message opens the
"Options" title (id 0x25 via 0x99) and, per row, a help message (ids
0x27..0x31 via the 0xAD row handler 0x80055CFC), all through the message
pointer table 0x8008CACC. The interpreter also reads the glyph widths
0x8008AE7C, the CLUT table 0x80097F30, the window colours 0x80097F6C..,
the decor parameter table 0x8008AF18 and the setting pointer table
0x8008D0D4. This tool copies those ranges verbatim:

  "L3EX0001" | u32 count | count x (u32 addr, u32 bytes, data) | u32 crc32

Output holds owner-derived data and MUST stay outside the source release.
"""
import argparse
import hashlib
import struct
import zlib
from pathlib import Path

EXE_SHA = "10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
TEXT = 0x80010000
RANGES = [
    (0x8008AE7C, 0x8008AF00),   # glyph widths (u8 per glyph byte 0x00..0x83)
    (0x8008AF18, 0x8008AF24),   # decor -> frame parameter
    (0x8008B67C, 0x8008BA7C),   # messages 0x25..0x32 (title, OPTION, help, sub-screen menu)
    (0x8008C79C, 0x8008C8E4),   # messages 0xDD..0xE2 (sub-screen menu help)
    (0x8008CACC, 0x8008CACC + 4 * 0xE4),   # message pointer table
    (0x8008E09C, 0x8008E120),   # sub-screen status frame sprite list (layout 6)
    (0x8008D0D4, 0x8008D0D4 + 4 * 10),     # setting pointer table
    (0x80097F30, 0x80097F90),   # CLUTs, window colours, edge greys
]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("exe", type=Path)
    ap.add_argument("out", type=Path)
    args = ap.parse_args()
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXE_SHA:
        raise SystemExit("cook_option_menu: not the USA ROCK_NEO.EXE this port was traced on")
    out = bytearray(b"L3EX0001") + struct.pack("<I", len(RANGES))
    for lo, hi in RANGES:
        off = 0x800 + lo - TEXT
        out += struct.pack("<II", lo, hi - lo) + image[off:off + hi - lo]
    # The table entries this menu reaches must stay inside the copied block.
    for i in list(range(0x25, 0x33)) + list(range(0xDD, 0xE3)):
        ptr = struct.unpack_from("<I", image, 0x800 + 0x8008CACC + 4 * i - TEXT)[0]
        if not (0x8008B67C <= ptr < 0x8008BA7C or 0x8008C79C <= ptr < 0x8008C8E4):
            raise SystemExit(f"cook_option_menu: message 0x{i:02X} at 0x{ptr:08X} outside the excerpt")
    out += struct.pack("<I", zlib.crc32(out) & 0xFFFFFFFF)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(out)
    print(f"cook_option_menu: {len(RANGES)} ranges, {len(out)} bytes -> {args.out}")


if __name__ == "__main__":
    main()

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
    (0x80089B2C, 0x80089EEC),   # weapon level tables (8 x 8 bytes per weapon) + pointers 0x80089EAC (item 65)
    (0x8008AE7C, 0x8008AF00),   # glyph widths (u8 per glyph byte 0x00..0x83)
    (0x8008AF18, 0x8008AF24),   # decor -> frame parameter
    (0x8008AF24, 0x8008AF48),   # number printer 0x8005BF10: six divisors, pad glyphs
    (0x8008B05C, 0x8008B67C),   # save/load screen messages (pointers 0x8008CAEC..0x8008CB50) and list strings
    (0x8008B67C, 0x8008C8E4),   # messages 0x25..0x32 (title, OPTION, help, sub-screen menu), part,
                                # item and weapon names, sub-screen menu help (0xDD..0xE2)
    (0x8008CACC, 0x8008CACC + 4 * 0xE4),   # message pointer table (also the name tables)
    (0x8008D0D4, 0x8008D0D4 + 4 * 32),     # setting pointer table (15 scroll, 26 max scroll, 27/17/30 cursors)
    (0x8008D7E8, 0x8008DB24),   # special weapon stat bytes + class index 0x8008DB08
    (0x8008DE1C, 0x8008E060),   # special item icon records (layout 2, handler 1)
    (0x8008E09C, 0x8008ECDC),   # sprite lists of the other layouts (6 frame, 7/8/9.. headers, 22/23/26/27/28)
    (0x8008ECDC, 0x8008ECDC + 8 * 30),     # layout table {handler, tpage, list}
    (0x8008FE64, 0x8008FE64 + 8 * 0x34),   # buster part stats (attack, energy, range, rapid)
    (0x80097F30, 0x80097F90),   # CLUTs, window colours, edge greys
    (0x8008D028, 0x8008D034),   # save block: digit -> message glyph (0x800679B8)
    (0x8008CE94, 0x8008D028),   # save screen: stage names, 13 bytes each (0x80066A40)
    (0x8008D168, 0x8008D1A0),   # save screen: list string pointers (names 0x8008D168, times 0x8008D184)
    (0x80097FAC, 0x80098004),   # save screen: "No data!", broken-data and empty-time strings
    (0x8008F9BC, 0x8008FAA4),   # save block: icon, CLUT, title (0x80064CB8 / 0x80067D10)
    (0x80098124, 0x8009813A),   # save block: Shift-JIS digits (0x80067D10)
]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("exe", type=Path)
    ap.add_argument("out", type=Path)
    ap.add_argument("--sub-key", type=Path, help="SUB_KEY.BIN: adds STFF_FE.MSG at 0x801F2000 (sub-screen item pages)")
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
        if not 0x8008B67C <= ptr < 0x8008C8E4:
            raise SystemExit(f"cook_option_menu: message 0x{i:02X} at 0x{ptr:08X} outside the excerpt")
    if args.sub_key:
        # SUB_KEY.BIN: SUB01.TIM, then "..\MESS\STFF_FE.MSG" (type 0, load
        # 0x801F2000, 0x38C7 bytes) whose payload follows its 0x800 header.
        sub = args.sub_key.read_bytes()
        hdr = sub.find(b"..\\MESS\\STFF_FE.MSG") - 0x40
        typ, size, _, load = struct.unpack_from("<IIII", sub, hdr)
        if hdr < 0 or typ != 0 or load != 0x801F2000:
            raise SystemExit("cook_option_menu: SUB_KEY.BIN has no STFF_FE.MSG at 0x801F2000")
        out[8:12] = struct.pack("<I", len(RANGES) + 1)
        out += struct.pack("<II", load, size) + sub[hdr + 0x800:hdr + 0x800 + size]
    out += struct.pack("<I", zlib.crc32(out) & 0xFFFFFFFF)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(out)
    print(f"cook_option_menu: {len(RANGES)} ranges, {len(out)} bytes -> {args.out}")


if __name__ == "__main__":
    main()

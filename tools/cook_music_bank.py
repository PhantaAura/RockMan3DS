#!/usr/bin/env python3
"""Cook the owner-local tutorial music (0x8008241C handlers 0/1) to L3MU.

Source path, ROCK_NEO.EXE (USA):
- area music: 0x80019D18 reads u16 0x80082320[stage][area] and queues
  0x8001A008(id, 0, 0); 0xFFFF stops the sequences (0x8001B33C).
- request id r: 0x8008241C + r*4 = [handler<<6 | VAB tag, loop<<7 | SEP tag,
  slot<<4 | priority, sequence]. Handler 1 (0x8001AC58) opens the SEP with
  SsSepOpen (0x8006B664) and plays it with SsSepPlay (0x8006E338).
- the SEP is DAT resource type 8 ([type, size, sectors, slot, tag, sequence
  count]); the VAB is type 5 (VB from the next 0x800 sector after the VH).
- libsnd pitch table 0x80090C80: 12 semitones x 16 fine steps (u16).
- stage reverb: 0x8001F23C calls 0x80019F94(0x800896D4[stage]), which points
  0x800AD15C at 0x800821F8 + n*4 = [type, RVol, depth, 0]; 0x8001AEF0 then sets
  SsUtSetReverbType(type). libspu's preset for the type is 0x80090988 +
  type*0x44 (mask word, 32 SPU reverb registers from 0x1F801DC0) and its work
  area starts at 0x80090938[type] (8-byte units).

Output holds the raw SEP, the raw VH and PCM16 decoded from the VB at the
SPU's pitch-0x1000 rate (44100 Hz). It is owner-derived and MUST stay outside
the source release.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import struct
import sys
import tempfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cook_sfx_bank import Exe, decode_vag, load_vab, vag_offsets  # noqa: E402

TABLE = 0x8008241C
AREA_MUSIC = 0x80082320
PITCH_TABLE = 0x80090C80
STAGE_REVERB = 0x800896D4
REVERB_ENTRIES = 0x800821F8
REVERB_PRESETS, REVERB_BASES = 0x80090988, 0x80090938
STAGE = 0
STAGE_AREAS = 6           # 0x80082200..0x8008220B; stage 3's row starts at 0x8008220C
MAGIC = b"L3MU001\0"
HEADER_FORMAT = "<8sIIIIIIIII"  # magic, version, areas, requests, seps, vabs, samples, rate, bytes, crc
REQUEST_FORMAT = "<H4sH"        # id, raw row, reserved
SEP_FORMAT = "<BBHII"           # tag, sequences, reserved, bytes, offset
VAB_FORMAT = "<BBHIIHH"         # tag, reserved, reserved, VH bytes, VH offset, first sample, samples
SAMPLE_FORMAT = "<IIII"         # frames, loop start (0xFFFFFFFF = none), pcm offset, pcm crc
# Tutorial data: tag -> (container, resource header offset).
TUTORIAL_SEPS = {0: ("ST00.BIN", 0xF5000)}
TUTORIAL_VABS = {20: ("ST00.BIN", 0xC2800)}


def load_sep(container: Path, header_offset: int) -> tuple[bytes, int, int]:
    data = container.read_bytes()
    kind, size, _sectors, _slot, tag, sequences = struct.unpack_from("<6I", data, header_offset)
    sep = data[header_offset + 0x800:header_offset + 0x800 + size]
    if kind != 8 or len(sep) != size or sep[:4] != b"pQES" or sep[4:6] != b"\0\0":
        raise ValueError(f"{container.name}:{header_offset:#x} is not a SEP resource")
    return sep, tag, sequences


def pad4(blob: bytes) -> bytes:
    return blob + b"\0" * (-len(blob) & 3)


def cook(exe: Exe, dat: Path, output: Path) -> dict:
    if output.exists():
        raise ValueError("Refusing to overwrite cooked music bank")
    if output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise ValueError("Owner-derived audio output must be outside source tree")
    row = exe.word(AREA_MUSIC + STAGE * 4)
    areas = [struct.unpack("<H", exe.read(row + i * 2, 2))[0] for i in range(STAGE_AREAS)]
    seps = {}
    for tag, (name, offset) in TUTORIAL_SEPS.items():
        sep, found, sequences = load_sep(dat / name, offset)
        if found != tag:
            raise ValueError(f"{name}: SEP tag {found} != {tag}")
        seps[tag] = (sep, sequences)
    vabs = {}
    for tag, (name, offset) in TUTORIAL_VABS.items():
        vh, body, found = load_vab(dat / name, offset)
        if found != tag:
            raise ValueError(f"{name}: VAB tag {found} != {tag}")
        vabs[tag] = (vh, body)
    requests = []
    for rid in sorted({a for a in areas if a != 0xFFFF}):
        raw = exe.read(TABLE + rid * 4, 4)
        if raw[0] >> 6 not in (0, 1) or (raw[0] & 0x3F) not in vabs or (raw[1] & 0x7F) not in seps:
            raise ValueError(f"Area music request {rid:#x} is not a cooked sequence")
        if raw[0] >> 6 == 1 and raw[3] >= seps[raw[1] & 0x7F][1]:
            raise ValueError(f"Request {rid:#x} sequence outside the SEP")
        requests.append((rid, raw))
    samples, vab_rows = [], []
    for tag, (vh, body) in vabs.items():
        offsets = vag_offsets(vh)
        count = struct.unpack_from("<H", vh, 0x16)[0]
        vab_rows.append((tag, vh, len(samples), count))
        for vag in range(1, count + 1):
            samples.append(decode_vag(body, *offsets[vag]))
    pitch = exe.read(PITCH_TABLE, 192 * 2)
    entry = exe.read(REVERB_ENTRIES + struct.unpack("<H", exe.read(STAGE_REVERB + STAGE * 2, 2))[0] * 4, 4)
    if entry[0] > 9:
        raise ValueError("Stage reverb type outside the libspu presets")
    base = exe.word(REVERB_BASES + entry[0] * 4)
    if not 0 < base <= 0xFFFE:
        raise ValueError("Reverb work area outside SPU RAM")
    table = bytearray(pitch + entry + struct.pack("<I", base) + exe.read(REVERB_PRESETS + entry[0] * 0x44 + 4, 64))
    for value in areas:
        table += struct.pack("<H", value)
    table += b"\0" * (-len(table) & 3)
    for rid, raw in requests:
        table += struct.pack(REQUEST_FORMAT, rid, raw, 0)
    header_bytes = struct.calcsize(HEADER_FORMAT)
    blob_start = (header_bytes + len(table) + struct.calcsize(SEP_FORMAT) * len(seps) +
                  struct.calcsize(VAB_FORMAT) * len(vab_rows) + struct.calcsize(SAMPLE_FORMAT) * len(samples))
    blobs, offset = [], blob_start
    for tag, (sep, sequences) in seps.items():
        table += struct.pack(SEP_FORMAT, tag, sequences, 0, len(sep), offset)
        blobs.append(pad4(sep))
        offset += len(blobs[-1])
    for tag, vh, first, count in vab_rows:
        table += struct.pack(VAB_FORMAT, tag, 0, 0, len(vh), offset, first, count)
        blobs.append(pad4(vh))
        offset += len(blobs[-1])
    for pcm, loop in samples:
        blob = struct.pack(f"<{len(pcm)}h", *pcm)
        table += struct.pack(SAMPLE_FORMAT, len(pcm), 0xFFFFFFFF if loop is None else loop,
                             offset, zlib.crc32(blob))
        blobs.append(blob)
        offset += len(blob)
    if header_bytes + len(table) != blob_start:
        raise AssertionError("L3MU table size")
    header = struct.pack(HEADER_FORMAT, MAGIC, 1, len(areas), len(requests), len(seps), len(vab_rows),
                         len(samples), 44100, offset, zlib.crc32(table))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("wb", dir=output.parent, prefix=".l3mu-", delete=False) as handle:
        handle.write(header + table)
        for blob in blobs:
            handle.write(blob)
        temporary = Path(handle.name)
    os.link(temporary, output)
    temporary.unlink()
    return {"areas": ",".join(f"{a:#x}" for a in areas), "reverb": entry[:3].hex(), "requests": len(requests),
            "seps": len(seps), "vabs": len(vab_rows), "samples": len(samples),
            "looped_samples": sum(1 for _p, loop in samples if loop is not None), "bytes": offset}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--dat", required=True, type=Path, help="owner CDDATA/DAT folder")
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    report = cook(Exe(args.exe.read_bytes()), args.dat, args.out)
    print(" ".join(f"{k}={v}" for k, v in report.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

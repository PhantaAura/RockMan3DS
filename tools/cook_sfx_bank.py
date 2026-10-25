#!/usr/bin/env python3
"""Cook the owner-local sound-effect requests (0x8001A968 handler) to L3SB.

Source path, ROCK_NEO.EXE (USA):
- request id r: 0x8008241C + r*4 = [handler<<6 | bank tag, program, tone<<4 |
  priority, (tone count-1)<<5 | first SPU voice]. Handler 2 (0x8001A968) keys
  one SPU voice per tone.
- the bank tag selects a loaded VAB (DAT resource type 5: slot, tag, SPU
  address, VH bytes). The tone record is VH + 0x820 + (program*16 + tone)*0x20.
- 0x8001A744: volume = tone vol (+2) * 0x81 (then 0x8001A274 pan/distance),
  start = VAG (+0x16) address, pitch = 0x8001A6DC(min (+6) - centre (+4),
  fine (+5)) with the note table 0x80082C70/0x80082CA0, ADSR1/2 (+0x10/+0x12).

Samples are decoded from the VAB body (SPU ADPCM) to PCM16 at the SPU's
pitch-0x1000 rate (44100 Hz). Output holds owner-derived audio and MUST stay
outside the source release.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import struct
import tempfile
import zlib

EXE_SHA = "10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
TABLE, TABLE_END = 0x8008241C, 0x80082C64
NOTE_UP, NOTE_DOWN = 0x80082C70, 0x80082CA0
FILTERS = ((0, 0), (60, 0), (115, -52), (98, -55), (122, -60))
MAGIC = b"L3SB001\0"
HEADER_FORMAT = "<8sIIIIIII"   # magic, version, requests, tones, samples, sample rate, file bytes, table crc
REQUEST_FORMAT = "<HBBBBH"     # id, first voice, tone count, priority, tag, first tone index
TONE_FORMAT = "<HHHBBH"        # pitch, adsr1, adsr2, volume, reserved, sample index
SAMPLE_FORMAT = "<IIII"        # frames, loop start (0xFFFFFFFF = none), pcm offset, pcm crc
MAX_SAMPLE_FRAMES = 1 << 20
# Tutorial banks: tag -> (container, resource header offset). common.vab is
# loaded before ST00 (slot 0); DEMO_DAT and EXIT_SUP carry the same bank name.
TUTORIAL_BANKS = {0: ("DEMO_DAT.BIN", 0x4E000), 1: ("ST00.BIN", 0x9E000),
                  2: ("ST00.BIN", 0xB2000), 20: ("ST00.BIN", 0xC2800)}


class Exe:
    def __init__(self, image: bytes):
        if len(image) != 825344 or hashlib.sha256(image).hexdigest() != EXE_SHA:
            raise ValueError("Unsupported owner executable identity")
        self.image = image
        self.base, self.size = struct.unpack_from("<II", image, 0x18)

    def read(self, address: int, count: int) -> bytes:
        if address < self.base or address + count > self.base + self.size:
            raise ValueError("Read leaves the executable LOAD")
        return self.image[2048 + address - self.base:2048 + address - self.base + count]

    def word(self, address: int) -> int:
        return struct.unpack("<I", self.read(address, 4))[0]


def note_pitch(exe: Exe, note: int) -> int:
    """0x8001A63C: 0x1000 shifted by octaves times the 12-entry 16.16 table."""
    if note >= 0:
        octave, step = divmod(note, 12)
        return ((4096 << octave) * exe.word(NOTE_UP + step * 4)) >> 16
    octave, step = divmod(-note, 12)
    return ((4096 >> octave) * exe.word(NOTE_DOWN + step * 4)) >> 16


def tone_pitch(exe: Exe, note: int, fine: int) -> int:
    """0x8001A6DC: linear between note and note+1 by fine/128 (C division)."""
    low, high = note_pitch(exe, note), note_pitch(exe, note + 1)
    step = (high - low) * fine
    return low + (int(step / 128) if step < 0 else step >> 7)


def load_vab(container: Path, header_offset: int) -> tuple[bytes, bytes, int]:
    data = container.read_bytes()
    kind, size, _sectors, slot, tag, _spu, vh_bytes = struct.unpack_from("<7I", data, header_offset)
    if kind != 5 or size <= vh_bytes:
        raise ValueError(f"{container.name}:{header_offset:#x} is not a VAB resource")
    # The type-5 loader (0x8001BB54 case 5) reads the VH, then the VB from the
    # next 0x800 sector: the VH is padded to a sector boundary in the container.
    vh_start = header_offset + 0x800
    vb_start = vh_start + ((vh_bytes + 0x7FF) & ~0x7FF)
    vh, vb = data[vh_start:vh_start + vh_bytes], data[vb_start:vb_start + size - vh_bytes]
    if len(vh) != vh_bytes or len(vb) != size - vh_bytes or vh[:4] != b"pBAV":
        raise ValueError(f"{container.name}:{header_offset:#x} VAB payload missing")
    return vh, vb, tag


def vag_offsets(vh: bytes) -> list[tuple[int, int]]:
    programs = struct.unpack_from("<H", vh, 0x12)[0]
    table = 0x820 + programs * 0x200
    sizes = struct.unpack_from("<256H", vh, table)
    out, offset = [(0, 0)], 0
    for size in sizes[1:]:
        out.append((offset, size << 3))
        offset += size << 3
    return out


def decode_vag(body: bytes, offset: int, size: int) -> tuple[list[int], int | None]:
    samples: list[int] = []
    loop_start = None
    older = newer = 0
    for block in range(offset, offset + size, 16):
        chunk = body[block:block + 16]
        if len(chunk) != 16:
            raise ValueError("Truncated VAG block")
        shift, filt, flags = chunk[0] & 15, chunk[0] >> 4, chunk[1]
        if filt > 4 or shift > 12:
            shift, filt = 12, 0     # SPU treats shift 13..15 as 9; none observed in tutorial banks
        gain0, gain1 = FILTERS[filt]
        if flags & 4:
            loop_start = len(samples)
        for i in range(28):
            nibble = (chunk[2 + i // 2] >> (4 * (i & 1))) & 15
            signed = nibble - 16 if nibble >= 8 else nibble
            value = (signed << 12) >> shift
            value += (newer * gain0 + older * gain1 + 32) >> 6
            value = max(-32768, min(32767, value))
            samples.append(value)
            older, newer = newer, value
        if flags & 1:
            return samples, loop_start if flags & 2 else None
        if len(samples) > MAX_SAMPLE_FRAMES:
            raise ValueError("Unbounded VAG")
    return samples, None


# Title (front end): common.vab is tag 0 and ee_title.vab tag 2, both from
# DEMO_DAT.BIN; the 0x8001456C confirms (166..168) use tag 2.
TITLE_BANKS = {0: ("DEMO_DAT.BIN", 0x4E000), 2: ("DEMO_DAT.BIN", 0x73800)}


def cook(exe: Exe, dat: Path, output: Path, bank_map: dict = TUTORIAL_BANKS) -> dict:
    if output.exists():
        raise ValueError("Refusing to overwrite cooked sound bank")
    if output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise ValueError("Owner-derived audio output must be outside source tree")
    banks = {}
    for tag, (name, offset) in bank_map.items():
        vh, body, found = load_vab(dat / name, offset)
        if found != tag:
            raise ValueError(f"{name}: bank tag {found} != {tag}")
        banks[tag] = (vh, body, vag_offsets(vh))
    requests, tones, samples, sample_keys = [], [], [], {}
    for rid in range((TABLE_END - TABLE) // 4):
        b0, program, tone_byte, voice_byte = exe.read(TABLE + rid * 4, 4)
        if b0 >> 6 != 2 or (b0 & 0x3F) not in banks:
            continue
        tag = b0 & 0x3F
        vh, body, vags = banks[tag]
        if (program & 0x7F) >= struct.unpack_from("<H", vh, 0x12)[0]:
            continue                # program not in this stage's bank (another stage's data)
        first_tone = len(tones)
        count = ((voice_byte >> 5) & 3) + 1
        for k in range(count):
            record = 0x820 + ((program & 0x7F) * 16 + (tone_byte >> 4) + k) * 0x20
            if record + 0x20 > len(vh):
                raise ValueError(f"Request {rid:#x} tone outside VH")
            vol, centre, fine, low = vh[record + 2], vh[record + 4], vh[record + 5], vh[record + 6]
            adsr1, adsr2, vag = struct.unpack_from("<HH2xH", vh, record + 0x10)
            if vag >= len(vags):
                raise ValueError(f"Request {rid:#x} VAG index {vag}")
            if vag == 0:            # empty tone slot: the voice keys silence
                tones.append((0, adsr1, adsr2, vol, 0xFFFF))
                continue
            key = (tag, vag)
            if key not in sample_keys:
                pcm, loop = decode_vag(body, *vags[vag])
                sample_keys[key] = len(samples)
                samples.append((pcm, loop))
            tones.append((tone_pitch(exe, low - centre, fine) & 0x3FFF, adsr1, adsr2, vol, sample_keys[key]))
        requests.append((rid, voice_byte & 0x1F, count, tone_byte & 15, tag, first_tone))
    table = bytearray()
    for r in requests:
        table += struct.pack(REQUEST_FORMAT, *r)
    for pitch, a1, a2, vol, index in tones:
        table += struct.pack(TONE_FORMAT, pitch, a1, a2, vol, 0, index)
    header_bytes = struct.calcsize(HEADER_FORMAT)
    offset = header_bytes + len(table) + struct.calcsize(SAMPLE_FORMAT) * len(samples)
    sample_table, blobs = bytearray(), []
    for pcm, loop in samples:
        blob = struct.pack(f"<{len(pcm)}h", *pcm)
        sample_table += struct.pack(SAMPLE_FORMAT, len(pcm), 0xFFFFFFFF if loop is None else loop,
                                    offset, zlib.crc32(blob))
        blobs.append(blob)
        offset += len(blob)
    table += sample_table
    header = struct.pack(HEADER_FORMAT, MAGIC, 1, len(requests), len(tones), len(samples), 44100,
                         offset, zlib.crc32(table))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("wb", dir=output.parent, prefix=".l3sb-", delete=False) as handle:
        handle.write(header + table)
        for blob in blobs:
            handle.write(blob)
        temporary = Path(handle.name)
    os.link(temporary, output)
    temporary.unlink()
    return {"requests": len(requests), "tones": len(tones), "samples": len(samples),
            "looped_samples": sum(1 for _p, loop in samples if loop is not None), "bytes": offset}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--dat", required=True, type=Path, help="owner CDDATA/DAT folder")
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--title", action="store_true", help="title-screen banks (DEMO_DAT tags 0 and 2)")
    args = parser.parse_args()
    report = cook(Exe(args.exe.read_bytes()), args.dat, args.out, TITLE_BANKS if args.title else TUTORIAL_BANKS)
    print(" ".join(f"{k}={v}" for k, v in report.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Cook the owner-local message voice clips (XA mono 18900 Hz) to an L3VB bank.

The message AUDIO opcode 0x8F (0x800548C4) calls 0x8001D58C(id): bank
(id>>28)&3, entry (id>>16)&0xFFF, index id&0xFFFF through 0x80088F9C /
0x80088EF8, then the file record at 0x80082CD0 + stream*12. The pair gives the
start offset (low 24 bits) with the XA channel in bits 24..28, and the end
offset. Tutorial clips are bank 0 entry 0 (stream 209): file 1, coding 0x04
(mono, 18900 Hz, 4-bit), 32 channels interleaved.

Input is the hash-guarded MODE2/2352 CUE/BIN (see cook_xa_stream.py). Output
holds owner-derived audio and MUST stay outside the source release.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import struct
import sys
import tempfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
import cook_xa_stream as xa  # noqa: E402

MAGIC = b"L3VB001\0"
HEADER_FORMAT = "<8sIIIIII"          # magic, version, count, rate, entry bytes, file bytes, table crc
HEADER_BYTES = struct.calcsize(HEADER_FORMAT)
ENTRY_FORMAT = "<IIII"               # request, frames, pcm offset, pcm crc32
ENTRY_BYTES = struct.calcsize(ENTRY_FORMAT)
SAMPLE_RATE = 18900
STRIDE = 32
SAMPLES_PER_SECTOR = 4032
MAX_CLIP_SECTORS = 256


def lookup(image: bytes, request: int) -> tuple[int, int, int, int]:
    """(stream id, file lba, start lba, end lba, channel) for one 0x8001D58C id."""
    if len(image) != 825344 or hashlib.sha256(image).hexdigest() != xa.EXE_SHA:
        raise ValueError("Unsupported owner executable identity")
    base, size = struct.unpack_from("<II", image, 0x18)

    def read(address: int, count: int) -> bytes:
        if address < base or address + count > base + size:
            raise ValueError("Source lookup leaves measured LOAD")
        return image[2048 + address - base:2048 + address - base + count]

    def word(address: int) -> int:
        return struct.unpack("<I", read(address, 4))[0]

    bank, entry, index = (request >> 28) & 3, (request >> 16) & 0xFFF, request & 0xFFFF
    stream = struct.unpack("<H", read(word(0x80088F9C + bank * 4) + entry * 2, 2))[0]
    pair = word(word(0x80088EF8 + bank * 4) + entry * 4)
    offset_word, end_word = struct.unpack("<II", read(pair + index * 8, 8))
    file_lba, file_bytes, _tag = struct.unpack("<III", read(0x80082CD0 + stream * 12, 12))
    if file_bytes % 2336:
        raise ValueError("Source file size is not a full Form2 sector count")
    start, end = file_lba + (offset_word & 0xFFFFFF), file_lba + (end_word & 0xFFFFFF)
    channel = (offset_word >> 24) & 31
    if not file_lba <= start < end <= file_lba + file_bytes // 2336:
        raise ValueError(f"Request {request:#x} leaves its source file")
    return stream, start, end, channel


def inspect_voice_sector(raw: bytes, lba: int, allow_eof_variant: bool) -> tuple[int, int, int, int]:
    if len(raw) != xa.SECTOR_BYTES:
        raise ValueError(f"Short full sector at LBA {lba}")
    if raw[:12] != xa.SYNC or raw[12:15] != xa.expected_msf(lba) or raw[15] != 2:
        raise ValueError(f"Invalid sync/MSF/Mode2 header at LBA {lba}")
    first, second = raw[16:20], raw[20:24]
    if first != second:
        variant = bytes((first[0], first[1] | 0x80, first[2] & 0x7F, first[3]))
        if not allow_eof_variant or first[2] != 0xE4 or second != variant:
            raise ValueError(f"Duplicate XA subheaders differ at LBA {lba}")
    return tuple(first)


def decode_mono_sector(raw: bytes, history: list[int]) -> list[int]:
    """Level B mono: units 0..7 play in order, unit u nibble (u&1) of byte u>>1."""
    out = []
    trial = history[:]
    for g in range(xa.GROUP_COUNT):
        group = raw[24 + g * xa.GROUP_BYTES:24 + (g + 1) * xa.GROUP_BYTES]
        parameters = xa.validate_group(group)
        for unit in range(8):
            parameter = parameters[unit]
            scale = 1 << (12 - (parameter & 15))
            newer_gain, older_gain = xa.COEFFICIENTS[parameter >> 4]
            newer, older = trial
            for row in range(28):
                nibble = (group[16 + row * 4 + (unit >> 1)] >> (4 * (unit & 1))) & 15
                signed = nibble if nibble < 8 else nibble - 16
                prediction = (newer_gain * newer + older_gain * older + 32) // 64
                value = max(-32768, min(32767, signed * scale + prediction))
                out.append(value)
                older, newer = newer, value
            trial = [newer, older]
    history[:] = trial
    return out


def cook_clip(source, request: int, spec: tuple[int, int, int, int], allow_eof: bool) -> bytes:
    _stream, start, end, channel = spec
    history = [0, 0]
    samples: list[int] = []
    previous = None
    for lba in range(start, end):
        source.seek(lba * xa.SECTOR_BYTES)
        raw = source.read(xa.SECTOR_BYTES)
        file, chan, submode, coding = inspect_voice_sector(raw, lba, allow_eof)
        if file != 1 or chan != channel or not submode & 4:
            continue
        if coding != 0x04 or submode not in (0x64, 0xE4):
            raise ValueError(f"Request {request:#x}: unsupported coding/submode at LBA {lba}")
        if previous is not None and lba - previous != STRIDE:
            raise ValueError(f"Request {request:#x}: channel interleave is not {STRIDE}")
        previous = lba
        samples.extend(decode_mono_sector(raw, history))
        if len(samples) > MAX_CLIP_SECTORS * SAMPLES_PER_SECTOR:
            raise ValueError("Clip exceeds bounded size")
        if submode & 0x80:
            break
    if not samples:
        raise ValueError(f"Request {request:#x}: no audio sectors")
    return struct.pack(f"<{len(samples)}h", *samples)


def cook_bank(binary: Path, image: bytes, requests: list[int], output: Path, allow_eof: bool) -> dict:
    if output.exists():
        raise ValueError("Refusing to overwrite cooked voice bank")
    if output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise ValueError("Owner-derived audio output must be outside source tree")
    clips = []
    with binary.open("rb") as source:
        for request in requests:
            spec = lookup(image, request)
            clips.append((request, spec, cook_clip(source, request, spec, allow_eof)))
    table = bytearray()
    offset = HEADER_BYTES + ENTRY_BYTES * len(clips)
    for request, _spec, pcm in clips:
        table += struct.pack(ENTRY_FORMAT, request, len(pcm) // 2, offset, zlib.crc32(pcm))
        offset += len(pcm)
    header = struct.pack(HEADER_FORMAT, MAGIC, 1, len(clips), SAMPLE_RATE, ENTRY_BYTES,
                         offset, zlib.crc32(table))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("wb", dir=output.parent, prefix=".l3vb-", delete=False) as handle:
        handle.write(header + table)
        for _r, _s, pcm in clips:
            handle.write(pcm)
        temporary = Path(handle.name)
    os.link(temporary, output)
    temporary.unlink()
    return {"schema": "mml-xa-voice/1", "status": "SOURCE_BACKED_PARTIAL", "clips": [
        {"request": f"{r:#x}", "stream": s[0], "start_lba": s[1], "end_lba": s[2], "channel": s[3],
         "frames": len(p) // 2, "seconds": round(len(p) / 2 / SAMPLE_RATE, 3)} for r, s, p in clips],
        "sample_rate": SAMPLE_RATE, "decoder_rounding": "(+32)//64, cold-zero history per clip",
        "retail_cd_decoder_parity": False, "bytes": offset}


def parse_requests(text: str) -> list[int]:
    out: list[int] = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = (int(x, 0) for x in part.split("-"))
            out.extend(range(lo, hi + 1))
        elif part:
            out.append(int(part, 0))
    if not out or len(out) > 512 or len(set(out)) != len(out):
        raise ValueError("Bad request list")
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cue", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--requests", required=True, help="e.g. 0x0-0x2d")
    args = parser.parse_args()
    binary, _meta = xa.verify_disc(args.cue)
    image = args.exe.read_bytes()
    report = cook_bank(binary, image, parse_requests(args.requests), args.out, allow_eof=True)
    total = sum(c["seconds"] for c in report["clips"])
    print(f"voice_clips={len(report['clips'])} seconds={total:.1f} bytes={report['bytes']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

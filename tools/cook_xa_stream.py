#!/usr/bin/env python3
"""Cook the measured owner-local command stream to bounded stereo PCM16.

Input is a hash-guarded MODE2/2352 CUE/BIN and the supported owner executable.
Output contains owner-derived audio and MUST remain outside the source release.
No third-party decoder implementation is used. See docs/research_01425/XA_IMPORT.md.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import tempfile
from typing import NamedTuple
import zlib

EXE_SHA = "10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
BIN_SHA = "b544b9f09f8d2fca435c433069da8726ec06eca5f131bd1599598f0d6f1ed908"
BIN_BYTES = 524601840
REQUEST = 0x20000003
SECTOR_BYTES = 2352
GROUP_BYTES = 128
GROUP_COUNT = 18
FRAMES_PER_SECTOR = 2016
SAMPLE_RATE = 37800
MAX_SELECTED_SECTORS = 4096
MAGIC = b"L3XA001\0"
HEADER_BYTES = 256
HEADER_FORMAT = "<8s12I5Q8I32s32s32s32s"
INDEX_FORMAT = "<IIQII"
INDEX_BYTES = struct.calcsize(INDEX_FORMAT)
COEFFICIENTS = ((0, 0), (60, 0), (115, -52), (98, -55))
SYNC = b"\0" + b"\xff" * 10 + b"\0"
PRIMARY_REFERENCE = "https://www.icdia.co.uk/docs/cdi_may94_r2.pdf"


class StreamSpec(NamedTuple):
    stream_id: int
    file_lba: int
    file_sectors: int
    start_lba: int
    end_lba: int
    filter_file: int
    filter_channel: int
    stride: int = 8


class SectorInfo(NamedTuple):
    file: int
    channel: int
    submode: int
    coding: int
    eof_variant: bool


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_cue(cue: Path) -> tuple[Path, int]:
    """Accept only the measured single-file, two-track owner profile."""
    if cue.stat().st_size > 4096:
        raise ValueError("Oversized CUE profile")
    text = cue.read_text(encoding="utf-8-sig")
    pattern = (r'\s*FILE\s+"([^"\r\n]+)"\s+BINARY\s+'
               r'TRACK\s+01\s+MODE2/2352\s+INDEX\s+01\s+00:00:00\s+'
               r'TRACK\s+02\s+AUDIO\s+INDEX\s+00\s+46:01:70\s+'
               r'INDEX\s+01\s+46:03:70\s*')
    match = re.fullmatch(pattern, text, re.IGNORECASE)
    if not match:
        raise ValueError("Unsupported CUE: require measured MODE2/2352/index-zero profile")
    filename = Path(match.group(1))
    if filename.is_absolute() or len(filename.parts) != 1 or filename.name in (".", ".."):
        raise ValueError("CUE binary must be a sibling basename")
    binary = (cue.parent / filename).resolve()
    if binary.parent != cue.parent.resolve() or not binary.is_file():
        raise ValueError("CUE binary is missing or leaves owner directory")
    return binary, (46 * 60 + 1) * 75 + 70


def verify_disc(cue: Path) -> tuple[Path, dict]:
    binary, track_end = parse_cue(cue)
    if binary.stat().st_size != BIN_BYTES or BIN_BYTES % SECTOR_BYTES:
        raise ValueError("Unsupported full-sector owner BIN size")
    digest = sha256_file(binary)
    if digest != BIN_SHA:
        raise ValueError("Unsupported full-sector owner BIN SHA-256")
    return binary, {"bin_bytes": BIN_BYTES, "bin_sha256": digest,
                    "cue_sha256": sha256_file(cue), "track1_end_lba": track_end,
                    "sector_bytes": SECTOR_BYTES, "index1_frames": 0}


def source_spec(image: bytes) -> StreamSpec:
    """Read this request's packet and file bounds from the authenticated LOAD."""
    if len(image) != 825344 or hashlib.sha256(image).hexdigest() != EXE_SHA:
        raise ValueError("Unsupported owner executable identity")
    if image[:8] != b"PS-X EXE":
        raise ValueError("Unsupported executable header")
    base, size = struct.unpack_from("<II", image, 0x18)
    if size > len(image) - 2048:
        raise ValueError("Truncated executable LOAD")

    def read(address: int, count: int) -> bytes:
        if address < base or count < 0 or address + count > base + size:
            raise ValueError("Source lookup leaves measured LOAD")
        offset = 2048 + address - base
        return image[offset:offset + count]

    def word(address: int) -> int:
        return struct.unpack("<I", read(address, 4))[0]

    bank, entry, index = (REQUEST >> 28) & 7, (REQUEST >> 16) & 255, REQUEST & 65535
    id_table = word(0x80088F9C + bank * 4)
    pair_table = word(0x80088EF8 + bank * 4)
    stream_id = struct.unpack("<H", read(id_table + entry * 2, 2))[0]
    pair_pointer = word(pair_table + entry * 4)
    offset_word, end_word = struct.unpack("<II", read(pair_pointer + index * 8, 8))
    file_lba, file_bytes, _tag = struct.unpack("<III", read(0x80082CD0 + stream_id * 12, 12))
    if file_bytes % 2336:
        raise ValueError("Source file size is not a full Form2 sector count")
    spec = StreamSpec(stream_id, file_lba, file_bytes // 2336,
                      file_lba + (offset_word & 0xFFFFFF),
                      file_lba + (end_word & 0xFFFFFF), 1, (offset_word >> 24) & 31)
    if (spec.stream_id, spec.start_lba, spec.end_lba, spec.filter_channel) != (245, 91731, 107315, 3):
        raise ValueError("Unsupported measured command span")
    validate_bounds(spec, BIN_BYTES // SECTOR_BYTES)
    return spec


def validate_bounds(spec: StreamSpec, disc_sectors: int) -> None:
    if not (0 <= spec.file_lba <= spec.start_lba < spec.end_lba <=
            spec.file_lba + spec.file_sectors <= disc_sectors):
        raise ValueError("Source stream span leaves file/disc bounds")
    if spec.stride != 8 or not (1 <= spec.filter_file <= 255 and 0 <= spec.filter_channel < 8):
        raise ValueError("Unsupported source filter/interleave")
    if spec.end_lba - spec.start_lba > MAX_SELECTED_SECTORS * spec.stride:
        raise ValueError("Source stream exceeds bounded package capacity")


def bcd(value: int) -> int:
    if not 0 <= value < 100:
        raise ValueError("Sector MSF minute cannot fit BCD")
    return (value // 10 << 4) | value % 10


def expected_msf(lba: int) -> bytes:
    if lba < 0:
        raise ValueError("Negative sector")
    minute, rest = divmod(lba + 150, 75 * 60)
    second, frame = divmod(rest, 75)
    return bytes((bcd(minute), bcd(second), bcd(frame)))


def inspect_sector(raw: bytes, lba: int, *, allow_eof_variant: bool = False) -> SectorInfo:
    if len(raw) != SECTOR_BYTES:
        raise ValueError(f"Short full sector at LBA {lba}")
    if raw[:12] != SYNC or raw[12:15] != expected_msf(lba) or raw[15] != 2:
        raise ValueError(f"Invalid sync/MSF/Mode2 header at LBA {lba}")
    first, second = raw[16:20], raw[20:24]
    eof_variant = False
    if first != second:
        # This exact owner dump's terminal mark modifies the redundant bytes.
        # It is an explicit profile exception, not generic corrupt-header repair.
        variant = bytes((first[0], first[1] | 0x80, first[2] & 0x7F, first[3]))
        if (not allow_eof_variant or first[1] >= 8 or first[2] != 0xE4 or
                first[3] != 1 or second != variant):
            raise ValueError(f"Duplicate XA subheaders differ at LBA {lba}")
        eof_variant = True
    file, channel, submode, coding = first
    if submode & 4:
        if not (channel < 8 and submode in (0x64, 0xE4) and coding == 1):
            raise ValueError(f"Unsupported XA audio submode/coding at LBA {lba}")
        if any(raw[2328:2348]):
            raise ValueError(f"Nonzero XA audio padding at LBA {lba}")
        # The supported dump has zero (omitted) EDC for audio. Do not label that
        # a verified sector error-detection code or accept unverified nonzero EDC.
        if any(raw[2348:2352]):
            raise ValueError(f"Unsupported nonzero XA audio EDC at LBA {lba}")
    elif not (file == 1 and channel == 0 and submode == 0 and coding == 0):
        raise ValueError(f"Unsupported nonaudio filler at LBA {lba}")
    return SectorInfo(file, channel, submode, coding, eof_variant)


def validate_group(group: bytes) -> tuple[int, ...]:
    if len(group) != GROUP_BYTES:
        raise ValueError("Short XA sound group")
    if group[:4] != group[4:8] or group[8:12] != group[12:16]:
        raise ValueError("Duplicate XA sound parameters differ")
    parameters = tuple(group[:4] + group[8:12])
    if any((parameter >> 4) > 3 or (parameter & 15) > 12 for parameter in parameters):
        raise ValueError("Unsupported XA predictor filter/range")
    return parameters


def decode_group(group: bytes, history: list[list[int]]) -> list[int]:
    """Decode Level B stereo; history stores clipped newer/older PCM per side.

    Philips Green Book IV.3.6/IV.4.3/IV.5.3 defines layout and rational gains.
    Quantization here is explicit nearest with ties toward positive infinity:
    (prediction_numerator + 32) // 64. It is not a PS1 hardware parity claim.
    """
    parameters = validate_group(group)
    if len(history) != 2 or any(len(side) != 2 for side in history):
        raise ValueError("Stereo predictor history requires two pairs")
    output = []
    for pair in range(4):
        units = []
        for channel in range(2):
            unit = pair * 2 + channel
            parameter = parameters[unit]
            scale = 1 << (12 - (parameter & 15))
            newer_gain, older_gain = COEFFICIENTS[parameter >> 4]
            newer, older = history[channel]
            samples = []
            for row in range(28):
                packed = group[16 + row * 4 + pair]
                nibble = (packed >> (4 * channel)) & 15
                signed = nibble if nibble < 8 else nibble - 16
                prediction = (newer_gain * newer + older_gain * older + 32) // 64
                value = max(-32768, min(32767, signed * scale + prediction))
                samples.append(value)
                older, newer = newer, value
            history[channel][:] = newer, older
            units.append(samples)
        for left, right in zip(*units):
            output.extend((left, right))
    return output


def decode_sector(raw: bytes, history: list[list[int]]) -> bytes:
    if len(raw) != SECTOR_BYTES:
        raise ValueError("Short XA sector")
    groups = [raw[24 + i * GROUP_BYTES:24 + (i + 1) * GROUP_BYTES] for i in range(GROUP_COUNT)]
    for group in groups:
        validate_group(group)
    trial = [side[:] for side in history]
    samples = []
    for group in groups:
        samples.extend(decode_group(group, trial))
    pcm = struct.pack("<4032h", *samples)
    history[:] = trial
    return pcm


def encode_header(spec: StreamSpec, count: int, pcm_crc: int, index_crc: int,
                  raw_crc: int, raw_sha: bytes, disc_sha: bytes, pcm_sha: bytes,
                  exe_sha: bytes) -> bytes:
    pcm_offset = HEADER_BYTES + count * INDEX_BYTES
    pcm_bytes = count * FRAMES_PER_SECTOR * 4
    header = bytearray(struct.pack(
        HEADER_FORMAT, MAGIC, HEADER_BYTES, 1, 1, spec.stream_id,
        spec.start_lba, spec.end_lba, SAMPLE_RATE, 2, 16, 1, count, INDEX_BYTES,
        count * FRAMES_PER_SECTOR, HEADER_BYTES, pcm_offset, pcm_bytes,
        pcm_offset + pcm_bytes, 0, index_crc, pcm_crc, raw_crc,
        spec.filter_file, spec.filter_channel, spec.stride, 0,
        raw_sha, disc_sha, pcm_sha, exe_sha))
    assert len(header) == HEADER_BYTES
    struct.pack_into("<I", header, 96, zlib.crc32(header))
    return bytes(header)


def cook_stream(binary: Path, output: Path, spec: StreamSpec, *,
                disc_sha: str, exe_sha: str, allow_eof_variant: bool = False) -> dict:
    """One-sector source/PCM working set; caller must authenticate owner input."""
    if binary.stat().st_size % SECTOR_BYTES:
        raise ValueError("BIN is not full-sector aligned")
    validate_bounds(spec, binary.stat().st_size // SECTOR_BYTES)
    if output.exists():
        raise ValueError("Refusing to overwrite cooked XA package")
    if output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise ValueError("Owner-derived audio output must be outside source tree")
    if any(parent.name == "sources" for parent in output.resolve().parents):
        raise ValueError("Synced reference sources are read-only")
    if len(bytes.fromhex(disc_sha)) != 32 or len(bytes.fromhex(exe_sha)) != 32:
        raise ValueError("Provenance hashes must be SHA-256")
    if allow_eof_variant and disc_sha != BIN_SHA:
        raise ValueError("EOF duplicate-subheader exception requires exact owner BIN SHA-256")
    count_expected = (spec.end_lba - spec.start_lba) // spec.stride
    if (spec.end_lba - spec.start_lba) % spec.stride or count_expected == 0:
        raise ValueError("Measured span must contain complete eight-sector interleaves")
    pcm_offset = HEADER_BYTES + count_expected * INDEX_BYTES
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    raw_digest, pcm_digest = hashlib.sha256(), hashlib.sha256()
    raw_crc = pcm_crc = index_crc = 0
    header_digest = hashlib.sha256()
    header_crc = 0
    history = [[0, 0], [0, 0]]
    combinations: dict[tuple[int, int, int, int], int] = {}
    eof_variants = []
    selected_eof = []
    index = bytearray()
    count = 0
    parameters: dict[int, int] = {}
    try:
        with tempfile.NamedTemporaryFile(mode="w+b", prefix=".l3xa-", dir=output.parent,
                                         delete=False) as handle, binary.open("rb") as source:
            temporary = Path(handle.name)
            handle.write(bytes(pcm_offset))
            source.seek(spec.start_lba * SECTOR_BYTES)
            for lba in range(spec.start_lba, spec.end_lba):
                raw = source.read(SECTOR_BYTES)
                info = inspect_sector(raw, lba, allow_eof_variant=allow_eof_variant)
                raw_digest.update(raw)
                raw_crc = zlib.crc32(raw, raw_crc)
                header_digest.update(raw[:24])
                header_crc = zlib.crc32(raw[:24], header_crc)
                key = info.file, info.channel, info.submode, info.coding
                combinations[key] = combinations.get(key, 0) + 1
                if info.eof_variant:
                    eof_variants.append({"lba": lba, "first": raw[16:20].hex(),
                                         "second": raw[20:24].hex()})
                if info.file != spec.filter_file or info.channel != spec.filter_channel:
                    continue
                if not (info.submode & 4):
                    raise ValueError("Filtered source sector is not audio")
                if selected_eof:
                    raise ValueError("Filtered audio continues after its EOF marker")
                expected_lba = spec.start_lba + spec.filter_channel + count * spec.stride
                if lba != expected_lba or count >= MAX_SELECTED_SECTORS:
                    raise ValueError("Filtered source sectors violate measured interleave")
                for group_index in range(GROUP_COUNT):
                    group = raw[24 + group_index * GROUP_BYTES:24 + (group_index + 1) * GROUP_BYTES]
                    for parameter in validate_group(group):
                        parameters[parameter] = parameters.get(parameter, 0) + 1
                pcm = decode_sector(raw, history)
                if info.submode & 0x80:
                    selected_eof.append(lba)
                pcm_digest.update(pcm)
                pcm_crc = zlib.crc32(pcm, pcm_crc)
                entry = struct.pack(INDEX_FORMAT, lba, FRAMES_PER_SECTOR,
                                    count * FRAMES_PER_SECTOR, zlib.crc32(pcm), 0)
                index.extend(entry)
                index_crc = zlib.crc32(entry, index_crc)
                handle.write(pcm)
                count += 1
            if count != count_expected:
                raise ValueError("Incomplete filtered channel stream")
            header = encode_header(spec, count, pcm_crc, index_crc, raw_crc,
                                   raw_digest.digest(), bytes.fromhex(disc_sha),
                                   pcm_digest.digest(), bytes.fromhex(exe_sha))
            handle.seek(0)
            handle.write(header)
            handle.write(index)
            handle.flush()
            os.fsync(handle.fileno())
        # Hard-link publication refuses an unexpected existing destination.
        os.link(temporary, output)
        temporary.unlink()
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {
        "schema": "mml-xa-import/1", "status": "SOURCE_BACKED_PARTIAL",
        "source_request": hex(REQUEST), "stream_id": spec.stream_id,
        "source_file_lba": spec.file_lba, "source_file_sectors": spec.file_sectors,
        "start_lba": spec.start_lba, "end_lba_exclusive": spec.end_lba,
        "physical_sectors": spec.end_lba - spec.start_lba,
        "filter_file": spec.filter_file, "filter_channel": spec.filter_channel,
        "coding": "0x01", "sample_rate": SAMPLE_RATE, "channels": 2,
        "xa_sample_bits": 4, "pcm_sample_bits": 16, "pcm_byte_order": "little",
        "frames_per_selected_sector": FRAMES_PER_SECTOR, "selected_sectors": count,
        "total_frames": count * FRAMES_PER_SECTOR,
        "duration_seconds": count * FRAMES_PER_SECTOR / SAMPLE_RATE,
        "first_selected_lba": spec.start_lba + spec.filter_channel,
        "last_selected_lba": spec.start_lba + spec.filter_channel + (count - 1) * spec.stride,
        "interleave_stride": spec.stride, "selected_eof_lbas": selected_eof,
        "duplicate_subheader_eof_exceptions": eof_variants,
        "all_other_duplicate_subheaders_match": True,
        "sound_parameter_duplicates_match": True, "audio_padding20_zero": True,
        "audio_edc_zero_omitted": True, "edc_validated": False,
        "sector_header24_sha256": header_digest.hexdigest(),
        "sector_header24_crc32": f"{header_crc:08x}",
        "raw_full2352_span_sha256": raw_digest.hexdigest(),
        "raw_full2352_span_crc32": f"{raw_crc:08x}",
        "bin_sha256": disc_sha, "exe_sha256": exe_sha,
        "pcm_sha256": pcm_digest.hexdigest(), "pcm_crc32": f"{pcm_crc:08x}",
        "index_crc32": f"{index_crc:08x}",
        "package_sha256": sha256_file(output), "package_bytes": output.stat().st_size,
        "package_header_bytes": HEADER_BYTES, "package_index_entry_bytes": INDEX_BYTES,
        "package_header_crc32": f"{struct.unpack_from('<I', header, 96)[0]:08x}",
        "combinations": [{"file": k[0], "channel": k[1], "submode": hex(k[2]),
                          "coding": hex(k[3]), "count": v} for k, v in sorted(combinations.items())],
        "parameter_counts": {f"{k:02x}": v for k, v in sorted(parameters.items())},
        "final_predictor_history": history,
        "initial_predictor_history": [[0, 0], [0, 0]],
        "decoder_rounding": "nearest integer, ties toward positive infinity (+32)//64",
        "retail_seek_history_parity": False, "retail_cd_decoder_parity": False,
        "hardware_tested": False, "owner_payload_release_permitted": False,
        "working_set": "one full source sector, one PCM sector, bounded <=4096-entry index",
        "primary_codec_reference": PRIMARY_REFERENCE,
        "unsupported": ["mono", "18900Hz", "8-bit XA", "emphasis", "reserved coding",
                        "other disc profiles", "arbitrary duplicate-subheader corruption",
                        "PS1 44100Hz interpolation", "retail prior-session predictor history"]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cue", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--proof", type=Path, help="Metadata-only JSON evidence output")
    args = parser.parse_args()
    try:
        if args.exe.stat().st_size != 825344:
            raise ValueError("Unsupported owner executable size")
        if args.proof:
            if args.proof.resolve() == args.out.resolve():
                raise ValueError("Proof cannot replace owner audio package")
            if any(parent.name == "sources" for parent in args.proof.resolve().parents):
                raise ValueError("Synced reference sources are read-only")
            if args.proof.exists():
                raise ValueError("Refusing to overwrite existing XA evidence")
        spec = source_spec(args.exe.read_bytes())
        binary, disc_profile = verify_disc(args.cue.resolve())
        if spec.end_lba > disc_profile["track1_end_lba"]:
            raise ValueError("Selected source stream enters audio track")
        proof = cook_stream(binary, args.out, spec, disc_sha=BIN_SHA, exe_sha=EXE_SHA,
                            allow_eof_variant=True)
        proof["disc_profile"] = disc_profile
        if args.proof:
            args.proof.parent.mkdir(parents=True, exist_ok=True)
            with args.proof.open("x", encoding="utf-8") as handle:
                handle.write(json.dumps(proof, indent=2) + "\n")
        print(json.dumps(proof, indent=2))
        return 0
    except (ValueError, OSError) as error:
        parser.exit(2, f"XA import rejected: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())

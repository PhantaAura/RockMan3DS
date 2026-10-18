"""Authored XA sectors only. No owner media or decoder implementation fixtures."""
import hashlib
import importlib.util
from fractions import Fraction
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

_spec = importlib.util.spec_from_file_location(
    "xa_cooker", Path(__file__).resolve().parents[1] / "tools/cook_xa_stream.py")
xa = importlib.util.module_from_spec(_spec)
sys.modules[_spec.name] = xa
_spec.loader.exec_module(xa)


def group(parameters=None, rows=None):
    parameters = parameters or [12] * 8
    rows = rows or [[0] * 8 for _ in range(28)]
    output = bytearray()
    for unit in (0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7, 4, 5, 6, 7):
        output.append(parameters[unit])
    for row in rows:
        for pair in range(4):
            output.append((row[pair * 2] & 15) | ((row[pair * 2 + 1] & 15) << 4))
    return bytes(output)


def sector(lba, channel=3, submode=0x64, coding=1, groups=None, eof_variant=False):
    output = bytearray(xa.SYNC + xa.expected_msf(lba) + b"\2")
    first = bytes((1, channel, submode, coding))
    output.extend(first)
    output.extend(bytes((1, channel | 0x80, submode & 0x7F, coding)) if eof_variant else first)
    output.extend(b"".join(groups or [group()] * 18))
    output.extend(bytes(24))
    assert len(output) == 2352
    return bytes(output)


def rational_oracle(parameters, rows, state):
    """Mathematical oracle from published exact rational gains, not codec code.

    A full rational sum is rounded once and clipped before its next recurrence.
    Its gain fractions and input chronology are deliberately authored here.
    """
    gains = ((Fraction(0), Fraction(0)), (Fraction(15, 16), Fraction(0)),
             (Fraction(115, 64), Fraction(-13, 16)),
             (Fraction(49, 32), Fraction(-55, 64)))
    result = []
    for pair in range(4):
        for row in rows:
            for side in range(2):
                unit = pair * 2 + side
                value = row[unit]
                value = value if value < 8 else value - 16
                parameter = parameters[unit]
                a, b = gains[parameter >> 4]
                exact = (Fraction(value * 2 ** (12 - (parameter & 15))) +
                         a * state[side][0] + b * state[side][1])
                rounded = (exact + Fraction(1, 2)).__floor__()
                clipped = min(32767, max(-32768, rounded))
                result.append(clipped)
                state[side] = [clipped, state[side][0]]
    return result


class DecoderTests(unittest.TestCase):
    def test_authored_nibble_sign_channel_and_unit_parameter_layout(self):
        parameters = list(range(12, 4, -1))
        rows = [[1, 2, 3, 4, 5, 6, 7, 8]] * 28
        state = [[0, 0], [0, 0]]
        decoded = xa.decode_group(group(parameters, rows), state)
        expected = []
        for left, right in ((1, 4), (12, 32), (80, 192), (448, -1024)):
            expected.extend([left, right] * 28)
        self.assertEqual(decoded, expected)
        self.assertEqual(state, [[448, 448], [-1024, -1024]])

    def test_authored_signed_rounding_ties_and_clipping(self):
        params = [0x1C] * 8
        decoded = xa.decode_group(group(params), [[8, 0], [-8, 0]])
        self.assertEqual(decoded[:2], [8, -7])
        params = [0x30] * 8
        rows = [[7, 8] * 4] * 28
        state = [[32767, -32768], [-32768, 32767]]
        decoded = xa.decode_group(group(params, rows), state)
        self.assertEqual(decoded[:2], [32767, -32768])
        self.assertTrue(all(-32768 <= sample <= 32767 for sample in decoded))

    def test_all_filters_ranges_against_independent_exact_rational_oracle(self):
        # Exercise history across unit, group and sector-shaped boundaries.
        actual_state = [[1234, -987], [-456, 321]]
        oracle_state = [side[:] for side in actual_state]
        for sequence in range(52):
            params = [((sequence + unit) % 4 << 4) | ((sequence * 3 + unit) % 13)
                      for unit in range(8)]
            rows = [[(sequence * 11 + sample * 7 + unit * 5) % 16 for unit in range(8)]
                    for sample in range(28)]
            self.assertEqual(xa.decode_group(group(params, rows), actual_state),
                             rational_oracle(params, rows, oracle_state))
            self.assertEqual(actual_state, oracle_state)

    def test_predictor_continuity_across_selected_sectors(self):
        groups = [group([0x1C] * 8, [[1, 15] * 4] * 28)] * 18
        raw = sector(3, groups=groups)
        state = [[0, 0], [0, 0]]
        first = xa.decode_sector(raw, state)
        second = xa.decode_sector(raw, state)
        self.assertEqual(len(first), 8064)
        self.assertNotEqual(first[:128], second[:128])
        oracle_state = [[0, 0], [0, 0]]
        expected = []
        for _ in range(36):
            expected.extend(rational_oracle([0x1C] * 8, [[1, 15] * 4] * 28, oracle_state))
        self.assertEqual(first + second, struct.pack("<8064h", *expected))
        self.assertEqual(state, oracle_state)

    def test_malformed_parameters_rejected_without_sector_history_change(self):
        invalid = []
        for parameter in (0x0D, 0x0F, 0x40, 0xF0):
            invalid.append(group([parameter] * 8))
        mismatch = bytearray(group())
        mismatch[4] ^= 1
        invalid.append(bytes(mismatch))
        for bad in invalid:
            with self.subTest(header=bad[:16].hex()):
                state = [[8, 3], [-7, -2]]
                raw = sector(3, groups=[group()] * 17 + [bad])
                with self.assertRaises(ValueError):
                    xa.decode_sector(raw, state)
                self.assertEqual(state, [[8, 3], [-7, -2]])


class ImportGuardTests(unittest.TestCase):
    def test_headers_coding_padding_and_edc(self):
        original = sector(3)
        self.assertEqual(xa.inspect_sector(original, 3).coding, 1)
        for offset in (0, 12, 15, 20, 2328, 2348):
            bad = bytearray(original)
            bad[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                xa.inspect_sector(bytes(bad), 3)
        for coding in (0, 5, 0x11, 0x41, 0x81, 2, 9):
            with self.subTest(coding=coding), self.assertRaises(ValueError):
                xa.inspect_sector(sector(3, coding=coding), 3)
        for submode in (4, 0x44, 0x24, 0x66):
            with self.subTest(submode=submode), self.assertRaises(ValueError):
                xa.inspect_sector(sector(3, submode=submode), 3)
        with self.assertRaises(ValueError):
            xa.inspect_sector(original[:-1], 3)

    def test_exact_eof_subheader_profile_exception(self):
        raw = sector(3, submode=0xE4, eof_variant=True)
        with self.assertRaises(ValueError):
            xa.inspect_sector(raw, 3)
        self.assertTrue(xa.inspect_sector(raw, 3, allow_eof_variant=True).eof_variant)
        for offset in (16, 20, 21, 22, 23):
            bad = bytearray(raw)
            bad[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                xa.inspect_sector(bytes(bad), 3, allow_eof_variant=True)

    def test_source_and_span_bounds(self):
        for image in (b"", b"PS-X EXE", bytes(825344)):
            with self.assertRaises(ValueError):
                xa.source_spec(image)
        base = xa.StreamSpec(245, 0, 16, 0, 16, 1, 3)
        xa.validate_bounds(base, 16)
        for bad in (base._replace(start_lba=-1), base._replace(end_lba=17),
                    base._replace(file_sectors=40001), base._replace(start_lba=16),
                    base._replace(stride=7), base._replace(filter_channel=8),
                    base._replace(filter_file=0),
                    base._replace(file_sectors=40000, end_lba=40000)):
            with self.subTest(spec=bad), self.assertRaises(ValueError):
                xa.validate_bounds(bad, 40000)

    def test_cue_profile_and_binary_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary = root / "authored.bin"
            binary.write_bytes(bytes(2352))
            cue = root / "authored.cue"
            text = ('FILE "authored.bin" BINARY\n TRACK 01 MODE2/2352\n'
                    ' INDEX 01 00:00:00\n TRACK 02 AUDIO\n'
                    ' INDEX 00 46:01:70\n INDEX 01 46:03:70\n')
            cue.write_text(text)
            self.assertEqual(xa.parse_cue(cue)[0], binary.resolve())
            with self.assertRaises(ValueError):
                xa.verify_disc(cue)
            with patch.object(xa, "BIN_BYTES", 2352), self.assertRaisesRegex(ValueError, "SHA-256"):
                xa.verify_disc(cue)
            for bad in (text.replace("MODE2/2352", "MODE1/2048"),
                        text.replace("00:00:00", "00:02:00"),
                        text.replace("authored.bin", "../authored.bin"),
                        text + "TRACK 03 AUDIO\n", text.replace("46:03:70", "46:03:71")):
                cue.write_text(bad)
                with self.assertRaises(ValueError):
                    xa.parse_cue(cue)


class PackageTests(unittest.TestCase):
    def fixture(self, root, *, fault=None):
        binary = root / "authored.bin"
        rows = [[((sample * 3 + unit) % 16) for unit in range(8)] for sample in range(28)]
        groups = [group([0x1C] * 8, rows)] * 18
        sectors = [sector(lba, channel=lba % 8, groups=groups) for lba in range(16)]
        if fault:
            fault(sectors)
        binary.write_bytes(b"".join(sectors))
        return binary, xa.StreamSpec(245, 0, 16, 0, 16, 1, 3)

    def test_streamed_package_index_crc_sha_and_pcm_are_exact(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary, spec = self.fixture(root)
            output = root / "stream_245.l3xa"
            proof = xa.cook_stream(binary, output, spec, disc_sha=hashlib.sha256(binary.read_bytes()).hexdigest(),
                                   exe_sha="ab" * 32)
            blob = output.read_bytes()
            header = bytearray(blob[:256])
            saved_crc = struct.unpack_from("<I", header, 96)[0]
            struct.pack_into("<I", header, 96, 0)
            self.assertEqual(saved_crc, zlib.crc32(header))
            self.assertEqual(blob[:8], xa.MAGIC)
            self.assertEqual(len(blob), 256 + 48 + 2 * 8064)
            self.assertEqual(struct.unpack_from("<5Q", blob, 56), (4032, 256, 304, 16128, len(blob)))
            index = blob[256:304]
            state = [[0, 0], [0, 0]]
            with binary.open("rb") as source:
                source.seek(3 * 2352)
                first = xa.decode_sector(source.read(2352), state)
                source.seek(11 * 2352)
                second = xa.decode_sector(source.read(2352), state)
            self.assertEqual(blob[304:], first + second)
            self.assertEqual(struct.unpack_from(xa.INDEX_FORMAT, index, 0), (3, 2016, 0, zlib.crc32(first), 0))
            self.assertEqual(struct.unpack_from(xa.INDEX_FORMAT, index, 24), (11, 2016, 2016, zlib.crc32(second), 0))
            self.assertEqual(struct.unpack_from("<II", blob, 100), (zlib.crc32(index), zlib.crc32(first + second)))
            self.assertEqual(blob[128:160], hashlib.sha256(binary.read_bytes()).digest())
            self.assertEqual(blob[192:224], hashlib.sha256(first + second).digest())
            self.assertEqual(proof["package_sha256"], hashlib.sha256(blob).hexdigest())
            self.assertFalse(proof["retail_seek_history_parity"])
            self.assertFalse(proof["owner_payload_release_permitted"])
            with self.assertRaises(ValueError):
                xa.cook_stream(binary, output, spec, disc_sha="ab" * 32, exe_sha="ab" * 32)

    def test_failed_stream_leaves_no_output_or_partial(self):
        faults = [lambda sectors: sectors.__setitem__(11, sector(11, channel=2)),
                  lambda sectors: sectors.__setitem__(11, sectors[11][:-1]),
                  lambda sectors: sectors.__setitem__(11, sector(11, coding=5)),
                  lambda sectors: sectors.__setitem__(3, sector(3, submode=0xE4))]
        for fault in faults:
            with self.subTest(fault=fault), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                binary, spec = self.fixture(root, fault=fault)
                output = root / "stream.l3xa"
                with self.assertRaises(ValueError):
                    xa.cook_stream(binary, output, spec, disc_sha="ab" * 32, exe_sha="ab" * 32)
                self.assertFalse(output.exists())
                self.assertEqual({path.name for path in root.iterdir()}, {"authored.bin"})

    def test_eof_exception_requires_owner_hash_even_for_authored_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary, spec = self.fixture(root)
            with self.assertRaises(ValueError):
                xa.cook_stream(binary, root / "stream.l3xa", spec, disc_sha="ab" * 32,
                               exe_sha="ab" * 32, allow_eof_variant=True)

    def test_output_in_source_or_reference_tree_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary, spec = self.fixture(root)
            for output in (Path(xa.__file__).parents[1] / "audio.l3xa",
                           root / "sources" / "audio.l3xa"):
                with self.subTest(output=output), self.assertRaises(ValueError):
                    xa.cook_stream(binary, output, spec, disc_sha="ab" * 32, exe_sha="ab" * 32)
                self.assertFalse(output.exists())

    @unittest.skipUnless(shutil.which("ffmpeg"), "Existing FFmpeg is unavailable; no dependencies installed")
    def test_independent_ffmpeg_authored_decode_when_available(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary, spec = self.fixture(root)
            output = root / "stream.l3xa"
            xa.cook_stream(binary, output, spec, disc_sha="ab" * 32, exe_sha="ab" * 32)
            # Present only selected full sectors so the independent parser
            # receives one stream. Preserve headers and predictor chronology.
            raw = binary.read_bytes()
            selected = root / "selected.str"
            selected.write_bytes(raw[3 * 2352:4 * 2352] + raw[11 * 2352:12 * 2352])
            decoded = root / "ffmpeg.pcm"
            process = subprocess.run([shutil.which("ffmpeg"), "-v", "error", "-f", "psxstr",
                                      "-i", str(selected), "-map", "0:a:0", "-acodec", "pcm_s16le",
                                      "-f", "s16le", str(decoded)], capture_output=True, timeout=30)
            self.assertEqual(process.returncode, 0, process.stderr.decode())
            self.assertEqual(decoded.read_bytes(), output.read_bytes()[304:])


if __name__ == "__main__":
    unittest.main()

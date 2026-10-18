#!/usr/bin/env python3
"""Authored phase6 cooker cases contain no retail executable or table payload."""

import contextlib
import hashlib
import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

SOURCE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE / 'tools'))
import cook_source_event_phase6 as cooker


class FakeOwner:
    def __init__(self, image):
        if image != b'authored owner':
            raise ValueError('Unsupported authored owner')


class Phase6CookerTests(unittest.TestCase):
    def setUp(self):
        overlay = bytearray(0x1b378)
        self.camera = [index * 12345 for index in range(cooker.CAMERA_WORDS)]
        self.camera[-1] = 0xffffffff
        struct.pack_into('<231I', overlay, 0x1af4c, *self.camera)
        self.timeline = [(index * 29, 0x80101000 + index * 4) for index in range(18)]
        self.timeline[0] = (0, 0x80100738)
        self.timeline[-1] = (255, 0)
        for index, item in enumerate(self.timeline):
            struct.pack_into('<II', overlay, 0x1b2e8 + index * 8, *item)
        # Artificial owner field deliberately differs from the retail value.
        struct.pack_into('<H', overlay, 0x1ac54 + 36, 43210)
        self.overlay = bytes(overlay)

    @contextlib.contextmanager
    def owners(self, overlay=None, admitted=True, resource_count=1, resource_type=0):
        actual = self.overlay if overlay is None else overlay
        def digest(data):
            return cooker.OVERLAY_SHA if admitted and data == actual else hashlib.sha256(data).hexdigest()
        records = [{'load': cooker.OVERLAY_BASE, 'internal': 'authored PROGBIN',
                    'type': resource_type, 'payload': actual}] * resource_count
        with patch.object(cooker, 'OwnerExe', FakeOwner), \
             patch.object(cooker, 'iter_resources', return_value=records), \
             patch.object(cooker, 'sha', side_effect=digest):
            yield

    def run_cook(self, **kwargs):
        with self.owners(**kwargs):
            return cooker.cook(b'authored owner', Path('authored stage'))

    def mutate(self, package, offset, value, width=4, recalc=True):
        out = bytearray(package)
        out[offset:offset + width] = value.to_bytes(width, 'little')
        if recalc:
            struct.pack_into('<I', out, 28, zlib.crc32(out[96:]) & 0xffffffff)
        return bytes(out)

    def test_exact_header_scalar_tables_and_crc(self):
        package, evidence = self.run_cook()
        self.assertEqual(len(package), 1180)
        self.assertEqual(struct.unpack_from('<4sHH6I', package),
                         (b'L3P6', 1, 96, 1, 0, 231, 18, 1180,
                          zlib.crc32(package[96:]) & 0xffffffff))
        self.assertEqual(package[32:64], bytes.fromhex(cooker.OWNER_SHA))
        self.assertEqual(package[64:96], bytes.fromhex(cooker.OVERLAY_SHA))
        self.assertEqual(struct.unpack_from('<4I', package, 96),
                         (cooker.CAMERA_TOKEN, cooker.CALLBACK_TOKEN, 43210, 0))
        self.assertEqual(list(struct.unpack_from('<231I', package, 112)), self.camera)
        self.assertEqual([struct.unpack_from('<II', package, 1036 + index * 8)
                          for index in range(18)], self.timeline)
        self.assertEqual(evidence['package_sha256'], hashlib.sha256(package).hexdigest())
        self.assertEqual(evidence['field_owner']['record_offset'], 36)
        self.assertFalse(evidence['retail_parity'])
        self.assertFalse(evidence['hardware_tested'])
        self.assertNotIn('field_800bc81c', evidence)  # No original scalar printed in public evidence.

    def test_original_owner_guard_rejects_unrelated_or_truncated_exe(self):
        for image in (b'', b'PS-X EXE', b'authored unsupported input'):
            with self.subTest(size=len(image)), self.assertRaises(ValueError):
                cooker.cook(image, Path('unused'))

    def test_overlay_identity_and_resource_ambiguity_rejected(self):
        for parameters in ({'admitted': False}, {'resource_count': 0},
                           {'resource_count': 2}, {'resource_type': 1}):
            with self.subTest(parameters=parameters), self.assertRaises(ValueError):
                self.run_cook(**parameters)

    def test_truncated_source_tables_rejected(self):
        for end in (0, 0x1ac54 + 36, 0x1b2e8, 0x1b370, 0x1b377):
            with self.subTest(end=end), self.assertRaises(ValueError):
                self.run_cook(overlay=self.overlay[:end])

    def test_cooked_header_identity_counts_and_crc_rejected(self):
        package, _ = self.run_cook()
        candidates = [package[:-1], package + b'\0',
                      self.mutate(package, 0, 0, 1), self.mutate(package, 4, 2, 2),
                      self.mutate(package, 6, 32, 2), self.mutate(package, 8, 2),
                      self.mutate(package, 12, 1), self.mutate(package, 16, 232),
                      self.mutate(package, 20, 17), self.mutate(package, 24, 1179),
                      self.mutate(package, 32, 0, 1), self.mutate(package, 64, 0, 1),
                      self.mutate(package, 112, 1, recalc=False)]
        for index, bad in enumerate(candidates):
            with self.subTest(candidate=index), self.assertRaises(ValueError):
                cooker.validate_package(bad)

    def test_profile_tokens_field_domain_and_table_controls_rejected(self):
        package, _ = self.run_cook()
        candidates = [self.mutate(package, 96, 0), self.mutate(package, 100, 0),
                      self.mutate(package, 104, 65536), self.mutate(package, 108, 1),
                      self.mutate(package, 112 + 230 * 4, 0),
                      self.mutate(package, 1040, 0),
                      self.mutate(package, 1036 + 17 * 8, 256),
                      self.mutate(package, 1040 + 17 * 8, 1)]
        for index, bad in enumerate(candidates):
            with self.subTest(candidate=index), self.assertRaises(ValueError):
                cooker.validate_package(bad)

    def test_supported_unsigned_field_extremes_preserved(self):
        for value in (0, 65535):
            authored = bytearray(self.overlay)
            struct.pack_into('<H', authored, 0x1ac54 + 36, value)
            package, _ = self.run_cook(overlay=bytes(authored))
            self.assertEqual(struct.unpack_from('<I', package, 104)[0], value)

    def test_cli_creates_only_new_private_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            base = Path(folder)
            image = base / 'authored.exe'
            image.write_bytes(b'authored owner')
            output = base / 'new-output'
            argv = ['cook_source_event_phase6.py', '--exe', str(image), '--st00', 'authored stage',
                    '--out', str(output)]
            with self.owners(), patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(cooker.main(), 0)
            cooked = output / 'source_event_phase6.l3p6'
            cooker.validate_package(cooked.read_bytes())
            self.assertEqual({item.name for item in output.iterdir()},
                             {'source_event_phase6.l3p6', 'cook-evidence.json'})
            before = cooked.read_bytes()
            with patch.object(sys, 'argv', argv), self.assertRaises(ValueError):
                cooker.main()
            self.assertEqual(cooked.read_bytes(), before)

    def test_source_tree_output_rejected_before_reading_inputs(self):
        argv = ['cook_source_event_phase6.py', '--exe', 'missing', '--st00', 'missing',
                '--out', str(SOURCE / 'authored-not-created')]
        with patch.object(sys, 'argv', argv), self.assertRaises(ValueError):
            cooker.main()
        self.assertFalse((SOURCE / 'authored-not-created').exists())


if __name__ == '__main__':
    unittest.main()

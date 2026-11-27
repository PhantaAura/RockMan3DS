#!/usr/bin/env python3
"""Unit tests for tools/function_coverage.py (synthetic data only)."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import function_coverage as fc  # noqa: E402


class FunctionCoverageTest(unittest.TestCase):
    def test_merge_splits_ranges_and_prefers_real_names(self):
        a = [(0x80010000, 0x800100FF, 'FUN_80010000'), (0x80010200, 0x8001020F, 'FUN_80010200')]
        b = [(0x80010000, 0x800101FF, 'InitGeom'), (0x80010080, 0x800100BF, 'FUN_80010080')]
        m = fc.merge_funcs(a, b)
        self.assertEqual(m[0], (0x80010000, 0x8001007F, 'InitGeom'))
        self.assertEqual(m[1], (0x80010080, 0x800100BF, 'FUN_80010080'))
        self.assertEqual(m[2], (0x80010200, 0x8001020F, 'FUN_80010200'))

    def test_image_find_covers_whole_range_only(self):
        img = fc.Image('EXE', 0x80010000, bytes(0x400), [(0x80010000, 0x8001003F, 'f'), (0x80010100, 0x8001013F, 'g')])
        self.assertEqual(img.find(0x80010020)[2], 'f')
        self.assertIsNone(img.find(0x80010080))
        self.assertEqual(img.find(0x8001013F)[2], 'g')

    def test_stage_tokens_only_1_0_stages(self):
        self.assertEqual(fc.stage_tokens('R3_ST03 event 8, st0c gate, ST15 later'), ['ST03', 'ST0C'])

    def test_load_syms(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'syms.txt'
            p.write_text('main = 0x80011C90;\n\nRand_x = 0x8007FE50;\nScratch = 0x1F800002;\n')
            self.assertEqual(fc.load_syms(p), {0x80011C90: 'main', 0x8007FE50: 'Rand_x', 0x1F800002: 'Scratch'})

    def test_bios_stub_and_hardware_guess(self):
        words = [0x240A00B0, 0x01400008, 0x24090012]          # li t2,0xB0; jr t2; li t1,0x12
        img = fc.Image('EXE', 0x80010000, struct.pack('<3I', *words), [(0x80010000, 0x8001000B, 'FUN')])
        fc.attach_stub_names(img)
        guess, lib = fc.analyse(img, img.funcs[0], [img])
        self.assertEqual((guess, lib), ('BIOS B0:12 InitPad', 'BIOS'))
        hw = [0x3C081F80, 0x8D091814, 0x03E00008, 0x00000000]  # lui t0,0x1F80; lw t1,0x1814(t0)
        img2 = fc.Image('EXE', 0x80010000, struct.pack('<4I', *hw), [(0x80010000, 0x8001000F, 'FUN')])
        fc.attach_stub_names(img2)
        guess, lib = fc.analyse(img2, img2.funcs[0], [img2])
        self.assertIn('hardware: GPU', guess)
        self.assertEqual(lib, 'library (hardware driver)')

    def test_identify_overlay_by_prefix(self):
        ovl = {'ST02': b'\x01' * 64 + b'x', 'ST03': b'\x02' * 64}
        self.assertEqual(fc.identify_overlay(b'\x02' * 64, ovl), 'ST03')
        self.assertIsNone(fc.identify_overlay(b'\x03' * 64, ovl))


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
"""Authored metadata/header/path guards; no owner game bytes in fixtures."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

SOURCE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('cook_actor_combat', SOURCE / 'tools/cook_actor_combat.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
sys.path.insert(0,str(SOURCE / 'tools'))
import build_tutorial_pack as tutorial


class SyntheticOwner:
    def __init__(self):
        self.fields = {1: bytearray(16), 2: bytearray(16)}
        for selector, record in self.fields.items():
            record[:4] = struct.pack('<I', 0x10203040 + selector)
            record[8], record[9], record[10], record[11], record[12], record[15] = 11 + selector, 47 + selector, 23 + selector, 59 + selector, 35 + selector, 4
        # Subtype 0x19 rows (detection profile 6).
        self.fields19 = {1: bytearray(16), 2: bytearray(16)}
        for selector, record in self.fields19.items():
            record[:4] = struct.pack('<I', 0x103)
            record[8], record[9], record[10], record[15] = 32 * selector, 16 * selector, 16 * selector, 6
        self.bounds = (-9, 9, -11, 11, -13, 13)

    def word(self, address):
        for selector, (table, _) in module.STATS_TABLES.items():
            if address == table + 12:
                return 0x1000 + selector * 0x100
            if address == table + 0x19 * 4:
                return 0x2000 + selector * 0x100
        for selector, (table, _) in module.E0_TABLES.items():
            if address == table + 5 * 4:
                return 0x3000 + selector * 0x100
        raise ValueError('Unexpected authored pointer read')

    def read(self, address, size):
        for selector, data in self.fields.items():
            if address == 0x1000 + selector * 0x100 and size == 16:
                return bytes(data)
        for selector, data in self.fields19.items():
            if address == 0x2000 + selector * 0x100 and size == 16:
                return bytes(data)
        for selector in (1, 2):
            for index in range(3):
                if address == 0x3000 + selector * 0x100 + 16 * index and size == 16:
                    return struct.pack('<I', 0x3020305 + index) + bytes(4) + bytes((10, 64, 10, 34)) + bytes(4)
        if address == module.DETECTION_PROFILE_TABLE + 16 and size == 3:
            return bytes((27, 18, 19))
        if address == module.DETECTION_PROFILE_TABLE + 24 and size == 3:
            return bytes((40, 20, 21))
        if address == module.NORMALIZATION_ADDRESS and size == 384:
            return struct.pack('<192h', *(i - 96 for i in range(192)))
        if address == module.DETECTOR_BOUNDS_ADDRESS and size == 12:
            return struct.pack('<6h', *self.bounds)
        raise ValueError('Unexpected authored bounded read')


class CookActorCombatTests(unittest.TestCase):
    def test_authored_metadata_formats_and_unsigned_fields(self):
        packages, evidence = module.build_metadata(SyntheticOwner())
        self.assertEqual(set(packages), {'actor_combat.l3cb', 'actor_detection.l3dp',
                                         'actor_normalization.l3nr', 'actor_detection_bounds.l3db'})
        combat = packages['actor_combat.l3cb']
        self.assertEqual(struct.unpack('<4sHH6I', combat[:32]),
                         (b'L3CB', 5, 32, 1, 10, 20, 232, zlib.crc32(combat[32:]) & 0xFFFFFFFF, 0))
        self.assertEqual(combat[32:52], bytes((32,3,1,0,0,0,12,24,4,36,15,48,65,48,32,16,60,0,0,0)))
        self.assertEqual(combat[52:72], bytes((32,3,2,1,0,0,13,25,4,37,15,49,66,48,32,16,61,0,0,0)))
        self.assertEqual(evidence['combat_format_version'], 5)
        self.assertEqual([o['subtype'] for o in evidence['selection_origins']], [3, 3, 0x19, 0x19, 5, 5, 5, 5, 5, 5])
        self.assertEqual(combat[112:132], bytes((0xE0,5,1,0,0,0,10,10,0,0,15,64,5,3,2,3,34,0,0,0)))
        for origin in evidence['selection_origins']:
            self.assertEqual(origin['retained_offsets'], [0, 8, 9, 10, 11, 12, 15])
            self.assertEqual(origin['action4_variant32E'], 0)
        self.assertEqual(combat[72:92], bytes((32,0x19,1,0,0,0,32,16,6,0,15,16,3,1,0,0,0,0,0,0)))
        self.assertEqual(len(packages['actor_detection.l3dp']), 40)
        self.assertEqual(len(packages['actor_normalization.l3nr']), 416)
        self.assertEqual(len(packages['actor_detection_bounds.l3db']), 44)
        self.assertFalse(evidence['retail_values_embedded_in_source'])
        for name, blob in packages.items():
            self.assertEqual(evidence['outputs'][name]['sha256'], module.sha(blob))
            self.assertEqual(evidence['outputs'][name]['bytes'], len(blob))

    def test_bytec_zero_and_unsigned_maximum_are_present(self):
        owner = SyntheticOwner(); owner.fields[1][12] = 0; owner.fields[2][12] = 255
        packages, _ = module.build_metadata(owner)
        combat = packages['actor_combat.l3cb']
        self.assertEqual(combat[41:44], bytes((0,15,48)))
        self.assertEqual(combat[61:64], bytes((255,15,49)))

    def test_byte09_zero_and_unsigned_maximum_are_present(self):
        owner = SyntheticOwner(); owner.fields[1][9] = 0; owner.fields[2][9] = 255
        packages, _ = module.build_metadata(owner)
        combat = packages['actor_combat.l3cb']
        self.assertEqual(combat[42:44], bytes((15,0)))
        self.assertEqual(combat[62:64], bytes((15,255)))

    def test_unknown_detector_profile_and_unordered_bounds_reject(self):
        owner = SyntheticOwner(); owner.fields[2][15] = 5
        with self.assertRaises(ValueError): module.build_metadata(owner)
        owner = SyntheticOwner(); owner.bounds = (9, -9, -11, 11, -13, 13)
        with self.assertRaises(ValueError): module.build_metadata(owner)
        owner = SyntheticOwner(); owner.bounds = (-9, -9, -11, 11, -13, 13)
        with self.assertRaises(ValueError): module.build_metadata(owner)

    def test_exact_owner_hash_guard(self):
        with self.assertRaises(ValueError): module.cook(bytes(2048))
        with self.assertRaises(ValueError): module.cook(b'PS-X EXE' + bytes(2040))

    def test_cli_invalid_input_and_existing_output_preserve(self):
        with tempfile.TemporaryDirectory(prefix='l3-combat-cook-test-') as temporary:
            root = Path(temporary); image = root / 'authored-exe.bin'; image.write_bytes(bytes(2048))
            output = root / 'new-sidecars'
            process = subprocess.run([sys.executable, '-B', str(SOURCE / 'tools/cook_actor_combat.py'),
                '--exe', str(image), '--out', str(output)], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0); self.assertFalse(output.exists())
            output.mkdir(); sentinel = output / 'existing-note.txt'; sentinel.write_text('preserve')
            process = subprocess.run([sys.executable, '-B', str(SOURCE / 'tools/cook_actor_combat.py'),
                '--exe', str(image), '--out', str(output)], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0); self.assertEqual(sentinel.read_text(), 'preserve')
            forbidden = SOURCE / 'do-not-create-combat-sidecars-test'
            self.assertFalse(forbidden.exists())
            process = subprocess.run([sys.executable, '-B', str(SOURCE / 'tools/cook_actor_combat.py'),
                '--exe', str(image), '--out', str(forbidden)], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0); self.assertFalse(forbidden.exists())

    def test_tutorial_helper_authored_sidecars_and_embedded_provenance(self):
        with tempfile.TemporaryDirectory(prefix='l3-tutorial-actor-test-') as temporary:
            root=Path(temporary); image=root/'authored-owner.bin'; image.write_bytes(bytes(2048))
            out=root/'unpublished-pack'; runtime=out/'legends3ds/tutorial'; runtime.mkdir(parents=True)
            manifest={'resources':[],'existing_metadata':'preserved'}
            expected,_=module.build_metadata(SyntheticOwner())
            with patch.object(tutorial,'OwnerExe',return_value=SyntheticOwner()) as owner:
                tutorial._write_actor_source_metadata(image,runtime,out,manifest)
                owner.assert_called_once_with(bytes(2048))
            self.assertEqual({p.name for p in runtime.iterdir()},set(expected))
            provenance=manifest['actor_source_metadata']
            self.assertFalse(provenance['runtime_selector_default'])
            self.assertFalse(provenance['retail_parity'])
            self.assertEqual(manifest['existing_metadata'],'preserved')
            for name,blob in expected.items():
                self.assertEqual((runtime/name).read_bytes(),blob)
                self.assertEqual(provenance['outputs'][name]['path'],'legends3ds/tutorial/'+name)
                self.assertEqual(provenance['outputs'][name]['sha256'],module.sha(blob))

    def test_tutorial_helper_rejects_identity_existing_output_and_path(self):
        with tempfile.TemporaryDirectory(prefix='l3-tutorial-actor-guard-') as temporary:
            root=Path(temporary); image=root/'authored-owner.bin'; image.write_bytes(bytes(2048))
            out=root/'unpublished-pack'; runtime=out/'legends3ds/tutorial'; runtime.mkdir(parents=True)
            manifest={'resources':[]}
            # Actual OwnerExe remains unpatched here: an unknown identity cannot
            # produce any files or provenance, even in existing pack staging.
            with self.assertRaises(ValueError): tutorial._write_actor_source_metadata(image,runtime,out,manifest)
            self.assertEqual(list(runtime.iterdir()),[]); self.assertNotIn('actor_source_metadata',manifest)
            sentinel=runtime/'actor_combat.l3cb'; sentinel.write_bytes(b'preserve')
            with patch.object(tutorial,'OwnerExe',return_value=SyntheticOwner()):
                with self.assertRaises(ValueError): tutorial._write_actor_source_metadata(image,runtime,out,manifest)
            self.assertEqual(list(runtime.iterdir()),[sentinel]); self.assertEqual(sentinel.read_bytes(),b'preserve')
            self.assertNotIn('actor_source_metadata',manifest)
            with self.assertRaises(ValueError): tutorial._write_actor_source_metadata(image,root/'outside',out,manifest)
            with self.assertRaises(ValueError): tutorial._write_actor_source_metadata(image,SOURCE,SOURCE,manifest)
            manifest['actor_source_metadata']={'sentinel':'preserve'}
            with self.assertRaises(ValueError): tutorial._write_actor_source_metadata(image,runtime,out,manifest)
            self.assertEqual(manifest['actor_source_metadata'],{'sentinel':'preserve'})


if __name__ == '__main__':
    unittest.main()

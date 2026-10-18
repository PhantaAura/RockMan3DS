#!/usr/bin/env python3
"""Authored cooker unit cases; guards are mocked only for the positive fixture.

This file contains no supported executable, overlay, EBD or cooked retail data.
The independent source probe and owner-local cook supply original evidence.
"""
import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

SOURCE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(SOURCE/'tools'))
import cook_source_event_phase5 as cooker

class FakeOwner:
    def __init__(self,image):
        if image!=b'authored owner':raise ValueError('Unsupported authored input')
    def read(self,address,size):return bytes(size)

class Phase5CookerTests(unittest.TestCase):
    def setUp(self):
        overlay=bytearray(0x1b400)
        for i in range(16):
            raw=bytearray((i*19+j*23)&255 for j in range(20))
            raw[0]=i;raw[1]=5 if i==0 else 3;raw[3]=0x40 if i==0 else 0xe0
            overlay[0x1ae0c+i*20:0x1ae0c+(i+1)*20]=raw
            overlay[0x1b377+i]=(i-1)%4 if i else 0
        ebd=bytearray(113808);struct.pack_into('<I',ebd,0,14)
        for i in range(14):
            entry=bytes((0xe0,3,i%4,0))+struct.pack('<III',0x80180000+(i%4)*128,0,0)
            ebd[4+i*16:20+i*16]=entry
        self.overlay=bytes(overlay);self.ebd=bytes(ebd)
    def run_cook(self,overlay=None,ebd=None,admit_overlay=True,admit_ebd=True):
        actual_overlay=self.overlay if overlay is None else overlay
        actual_ebd=self.ebd if ebd is None else ebd
        # SHA identity substitution belongs exclusively to this authored test.
        # Production OwnerExe/overlay/EBD guards remain untouched.
        def digest(data):
            if admit_overlay and data==actual_overlay:return cooker.OVERLAY_SHA
            if admit_ebd and data==actual_ebd:return cooker.EBD_SHA
            return hashlib.sha256(data).hexdigest()
        def resources(path):
            if str(path)=='stage':return [{'payload':actual_overlay,'load':0x80100000,'internal':'authored PROGBIN'}]
            return [{'payload':actual_ebd,'load':0x8016c000,'internal':'authored EBD'}]
        with patch.object(cooker,'OwnerExe',FakeOwner),patch.object(cooker,'iter_resources',side_effect=resources),patch.object(cooker,'sha',side_effect=digest):
            return cooker.cook(b'authored owner',Path('stage'),Path('area'))
    def test_package_records_crc_and_first_match_order(self):
        package,evidence=self.run_cook()
        self.assertEqual(len(package),620)
        self.assertEqual(struct.unpack_from('<4sHH6I',package),
            (b'L3P5',1,96,1,16,620,zlib.crc32(package[96:])&0xffffffff,32,12))
        for i in range(16):
            self.assertEqual(package[108+i*32:128+i*32],self.overlay[0x1ae0c+i*20:0x1ae0c+(i+1)*20])
            token,index,variant=struct.unpack_from('<IIB',package,128+i*32)
            if i:self.assertEqual((token,index,variant),(0x80180000+((i-1)%4)*128,(i-1)%4,(i-1)%4))
            else:self.assertEqual((token,index,variant),(0,0xffffffff,0))
        self.assertEqual(evidence['selected_resource_bindings'],15)
        self.assertEqual(evidence['ebd_payload_bytes'],len(self.ebd))
        self.assertEqual(evidence['runtime_owner_admission'],'ACTUAL_RETAINED_SHA256_AND_FIRST_MATCH_SOURCE_DIRECTORY_JOIN')
        self.assertFalse(evidence['retail_parity'])
    def test_original_executable_guard_rejects_unrelated_input(self):
        with self.assertRaises(ValueError):cooker.cook(b'authored unsupported input',Path('unused'),Path('unused'))
    def test_overlay_identity_rejected(self):
        with self.assertRaises(ValueError):self.run_cook(admit_overlay=False)
    def test_ebd_identity_rejected(self):
        with self.assertRaises(ValueError):self.run_cook(admit_ebd=False)
    def test_record_id_and_family_rejected(self):
        for offset,value in ((0x1ae0c+20,31),(0x1ae0c+23,0x20)):
            bad=bytearray(self.overlay);bad[offset]=value
            with self.assertRaises(ValueError):self.run_cook(overlay=bytes(bad))
    def test_resource_count_rejected(self):
        bad=bytearray(self.ebd);struct.pack_into('<I',bad,0,15)
        with self.assertRaises(ValueError):self.run_cook(ebd=bytes(bad))
    def test_resource_miss_rejected(self):
        bad=bytearray(self.overlay);bad[0x1b378]=99
        with self.assertRaises(ValueError):self.run_cook(overlay=bytes(bad))
    def test_resource_pointer_outside_payload_rejected(self):
        bad=bytearray(self.ebd);struct.pack_into('<I',bad,8,0x87654321)
        with self.assertRaises(ValueError):self.run_cook(ebd=bytes(bad))
    def test_existing_output_never_overwritten(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(sys,'argv',['cook_source_event_phase5.py','--exe','missing','--st00','missing','--area','missing','--out',folder]):
                with self.assertRaises(ValueError):cooker.main()
            self.assertEqual(list(Path(folder).iterdir()),[])

if __name__=='__main__':unittest.main()

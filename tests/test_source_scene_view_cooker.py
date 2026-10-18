#!/usr/bin/env python3
"""Authored cooker regression; does not require/export owner game data."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import unittest
import zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import cook_source_scene_view as cook

class AuthoredOwner:
    def __init__(self,image):
        if image!=b'AUTHORED':raise ValueError('Authored owner identity rejected')
    def word(self,address):
        # Encode authored semantic caller records; never retain owner code.
        add=lambda register,value:(9<<26)|(register<<16)|value
        jump=lambda target:(3<<26)|((target>>2)&0x3ffffff)
        return {0x80011f48:add(4,160),0x80011f4c:jump(0x8007b118),
                0x80011f50:add(5,120),0x80011f54:jump(0x8007b138),0x80011f58:add(4,384)}[address]
    def read(self,address,n):
        if address!=0x80092348 or n!=2050:raise ValueError('Unexpected owner read')
        return struct.pack('<1025h',*[i*4 for i in range(1025)])

class TestCook(unittest.TestCase):
    def test_authored_layout(self):
        original=cook.OwnerExe;cook.OwnerExe=AuthoredOwner
        try:package=cook.cook(b'AUTHORED')
        finally:cook.OwnerExe=original
        self.assertEqual(len(package),2126)
        self.assertEqual(struct.unpack_from('<4sHH6I',package),
            (b'L3SV',1,64,1,0x80092348,1025,2126,zlib.crc32(package[64:])&0xffffffff,0))
        self.assertEqual(package[32:64],bytes.fromhex(cook.OWNER_SHA))
        self.assertEqual(struct.unpack_from('<3I',package,64),(160<<16,120<<16,384))
        self.assertEqual(struct.unpack_from('<1025h',package,76),tuple(i*4 for i in range(1025)))
    def test_wrong_caller_atomic_rejection(self):
        class WrongCaller(AuthoredOwner):
            def word(self,address):return super().word(address)^1
        original=cook.OwnerExe;cook.OwnerExe=WrongCaller
        try:
            with self.assertRaises(ValueError):cook.cook(b'AUTHORED')
        finally:cook.OwnerExe=original
    def test_wrong_scalar_table_rejection(self):
        class WrongTable(AuthoredOwner):
            def read(self,address,n):return struct.pack('<1025h',*([0]*1025))
        original=cook.OwnerExe;cook.OwnerExe=WrongTable
        try:
            with self.assertRaises(ValueError):cook.cook(b'AUTHORED')
        finally:cook.OwnerExe=original
    def test_unowned_executable_rejection(self):
        with self.assertRaises(ValueError):cook.cook(bytes(4096))
if __name__=='__main__':unittest.main()

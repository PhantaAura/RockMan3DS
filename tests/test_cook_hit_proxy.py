#!/usr/bin/env python3
import hashlib
from pathlib import Path
import struct
import sys
import unittest
import zlib

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import cook_hit_proxy


class FakeOwner:
    def __init__(self,image): self.image=image
    def read(self,address,size):
        if address==0x80097D00 and size==8: return self.image
        if address==0x80097DDC and size==4: return bytes((10,12,16,8))
        raise AssertionError('unexpected range')


class Tests(unittest.TestCase):
    def test_canonical_authored_table(self):
        original=cook_hit_proxy.OwnerExe
        try:
            cook_hit_proxy.OwnerExe=FakeOwner
            for table in (bytes(8),bytes([255]*8),bytes(range(8))):
                package=cook_hit_proxy.build_metadata(table)
                payload=table+bytes((10,12,16,8))
                self.assertEqual(len(package),36)
                self.assertEqual(struct.unpack_from('<4sHHIIII',package),
                    (b'L3PR',2,24,36,12,zlib.crc32(payload),0))
                self.assertEqual(package[24:],payload)
        finally: cook_hit_proxy.OwnerExe=original

    def test_unsupported_exe_rejected(self):
        self.assertRaises(ValueError,cook_hit_proxy.build_metadata,b'not owner data')


if __name__=='__main__':unittest.main()

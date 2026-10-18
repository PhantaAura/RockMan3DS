import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('source_player_head', TOOLS / 'source_player_head.py')
head = importlib.util.module_from_spec(spec)
spec.loader.exec_module(head)


class AuthoredOwner:
    def __init__(self):
        self.regions = {head.FRESH_STORE: struct.pack('<I', (40 << 26) | (16 << 21) | 0x16A),
                        head.HEAD_TABLE: struct.pack('<h', 7),
                        head.FILE_NAMES + 7 * 64: b'E:\\AUTHORED\\CDDATA\\DAT\\HEAD42.BIN\0'.ljust(64, b'\0'),
                        head.FILE_DIRECTORY + 7 * 12: struct.pack('<III', 42, 4096, 0)}
    def read(self, address, count):
        data = self.regions[address]
        if len(data) != count:
            raise ValueError('Unexpected authored read')
        return data
    def word(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]


def resource_blob():
    data = bytearray(4096)
    struct.pack_into('<II', data, 0, 0, 32)
    struct.pack_into('<I', data, 12, 0x80010000)
    path = b'..\\PLBIN\\PL42H42P.PBD\0'
    data[64:64 + len(path)] = path
    data[2048:2080] = bytes(range(32))
    return data


class FreshHeadSelectionGuards(unittest.TestCase):
    def test_owner_identity_guard(self):
        for data in (b'', b'PS-X EXE', bytes(2048), b'PS-X EXE' + bytes(4096)):
            with self.subTest(length=len(data)), self.assertRaises(ValueError):
                head.select_fresh_player_head(data, Path('/unused'))

    def test_authored_filename_is_derived_and_input_preserved(self):
        owner = AuthoredOwner()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'HEAD42.BIN'
            original = resource_blob()
            path.write_bytes(original)
            selection = head._decode_fresh_head(owner, directory)
            self.assertEqual((selection['selector'], selection['file_id']), (0, 7))
            self.assertEqual(selection['resource'], 'PL42H42P.PBD')
            self.assertEqual(selection['container'], 'HEAD42.BIN')
            self.assertTrue(selection['provenance']['fresh_only'])
            self.assertEqual(path.read_bytes(), original)

    def test_unsupported_instruction_name_directory_or_resource_rejected(self):
        changes = ['store', 'selector', 'name', 'padding', 'tag', 'size', 'short', 'kind', 'span', 'load', 'missing']
        for mutation in changes:
            owner = AuthoredOwner()
            blob = resource_blob()
            if mutation == 'store': owner.regions[head.FRESH_STORE] = bytes(4)
            if mutation == 'selector': owner.regions[head.HEAD_TABLE] = struct.pack('<h', -1)
            if mutation == 'name': owner.regions[head.FILE_NAMES + 448] = b'E:\\AUTHORED\\CDDATA\\DAT\\..\\HEAD42.BIN\0'.ljust(64, b'\0')
            if mutation == 'padding': owner.regions[head.FILE_NAMES + 448] = owner.regions[head.FILE_NAMES + 448][:-1] + b'x'
            if mutation == 'tag': owner.regions[head.FILE_DIRECTORY + 84] = struct.pack('<III', 42, 4096, 1)
            if mutation == 'size': owner.regions[head.FILE_DIRECTORY + 84] = struct.pack('<III', 42, 8192, 0)
            if mutation == 'short': blob = blob[:-1]
            if mutation == 'kind': struct.pack_into('<I', blob, 0, 1)
            if mutation == 'span': struct.pack_into('<I', blob, 4, 8192)
            if mutation == 'load': struct.pack_into('<I', blob, 12, 0x80010001)
            if mutation == 'missing': blob[64] = 0
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                (Path(directory) / 'HEAD42.BIN').write_bytes(blob)
                with self.assertRaises(ValueError): head._decode_fresh_head(owner, directory)


if __name__ == '__main__':
    unittest.main()

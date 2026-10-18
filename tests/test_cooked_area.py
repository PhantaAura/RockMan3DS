#!/usr/bin/env python3
"""Synthetic-only tests for the explicit cooked-area wrapper."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import zlib

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("package_cooked_area", ROOT / "tools/package_cooked_area.py")
wrapper = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(wrapper)


def synthetic_map(areas=1):
    # Independent 1x1 areas with empty grid cells. No retail data or cooker.
    payload = b"".join(struct.pack("<hhHHIIii", 0, 0, 1, 1, i, 1, 0, 0)
                       for i in range(areas)) + struct.pack("<" + "h" * areas, *([0] * areas))
    stats = (areas, areas, 0, 0, 0, 0, 0, 0, 0)
    offsets = (84, 84 + 24 * areas, *([84 + 26 * areas] * 5))
    return (struct.pack("<4sHHI9I7I2I", b"L3MP", 1, 84, 0, *stats,
                        *offsets, 84 + len(payload), zlib.crc32(payload)) + payload)


def synthetic_render():
    return struct.pack("<4sHHI6I6I2I3I", b"L3RG", 3, 80, 7,
                       *([0] * 6), *([80] * 6), 80, 0, 0, 0, 0)


def synthetic_negative_materials(areas=1):
    payload = (struct.pack("<IBBH", 0x1234001d, 9, 1, 0) + bytes(31 * 8)) * areas
    return struct.pack("<4sHHII", b"L3NM", 1, areas, 16 + len(payload), zlib.crc32(payload)) + payload


def synthetic_flow():
    objects = bytes((7, 0x20, 2, 3)) + bytes(16)
    rows = b"".join(struct.pack("<12I", area, int(area == 4), int(area == 3),
                                0, 0, 0, 0, 0, 0, 0, 0, 0) for area in range(5))
    payload = rows + objects + bytes(5 * 40)
    return struct.pack("<4sHH8I", b"L3SF", 1, 40, 31, 5, 1, 40, 280,
                       300, 40 + len(payload), zlib.crc32(payload)) + payload


def synthetic_ebd():
    base = 0x80400000
    entries = struct.pack("<4B3I", 0x20, 2, 3, 0, 20, 0xffffffff, 0xffffffff)
    source = struct.pack("<I4B3I4B", 1, 0x20, 2, 3, 0, base + 20, 0, 0, 12, 34, 56, 78)
    payload = entries + source
    return struct.pack("<4sHH8I", b"L3EB", 2, 40, 7, 1, base, 40,
                       40 + len(payload), zlib.crc32(payload), 56, len(source)) + payload


def synthetic_spawns():
    areas=b"".join(struct.pack("<2I",int(area>2),int(area==2)) for area in range(5))
    region=struct.pack("<hhbbBBII",11,3,-1,-2,4,5,0,1)
    record=bytes((7,0x20))+bytes(18)+struct.pack("<I",17)
    payload=areas+region+record
    return struct.pack("<4sHH8I",b"L3SP",1,40,5,1,1,40,80,96,40+len(payload),zlib.crc32(payload))+payload


class CookedAreaTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="mml-cooked-area-test-")
        # macOS temporary roots can themselves have /var aliases. Supply a
        # canonical path, since the wrapper deliberately refuses such aliases.
        self.root = Path(self.directory.name).resolve()
        self.map_path = self.root / "test.l3map"
        self.render_path = self.root / "test.l3render"
        self.output = self.root / "test.mmlpkg"
        self.map_path.write_bytes(synthetic_map())
        self.render_path.write_bytes(synthetic_render())

    def tearDown(self):
        self.directory.cleanup()

    def build(self, **changes):
        args = dict(map_path=self.map_path, render_path=self.render_path,
                    state_id="SYNTHETIC_TEST", out_path=self.output)
        args.update(changes)
        return wrapper.package_cooked_area(**args)

    def test_optional_map_initializer_crc_binding_and_profile(self):
        path = self.root / "test.l3init"
        data = struct.pack("<4sHH10I", b"L3MI", 1, 48, 1, 0, 0, 0, 0,
            zlib.crc32(self.map_path.read_bytes()), zlib.crc32(self.render_path.read_bytes()), 48, 48, 0)
        path.write_bytes(data)
        result = self.build(map_initialization_path=path)
        blob=self.output.read_bytes();offset,size=struct.unpack_from("<2I",blob,24)
        self.assertEqual(json.loads(blob[offset:offset+size])["components"]["map_initialization"]["section_kind"],35)
        self.output.unlink()
        for offset, value in ((8,2),(12,2049),(28,0),(32,0),(24,1)):
            with self.subTest(offset=offset):
                invalid=bytearray(data);struct.pack_into("<I",invalid,offset,value);path.write_bytes(invalid)
                with self.assertRaises(ValueError): self.build(map_initialization_path=path)
                self.assertFalse(self.output.exists())

    def test_v2_directory_crc_and_input_identity(self):
        result = self.build()
        blob = self.output.read_bytes()
        magic, version, count, total, profile, table_crc = struct.unpack_from("<IHHIII", blob)
        self.assertEqual((magic, version, count, total), (0x324C4D4D, 2, 3, len(blob)))
        self.assertEqual(profile, zlib.crc32(wrapper.PROFILE.encode("ascii")))
        table = blob[20:68]
        self.assertEqual(table_crc, zlib.crc32(table))
        payloads = {}
        cursor = 68
        for index in range(count):
            kind, offset, size, crc = struct.unpack_from("<IIII", table, index * 16)
            self.assertEqual(offset, cursor)
            payload = blob[offset:offset + size]
            self.assertEqual(crc, zlib.crc32(payload))
            payloads[kind] = payload
            cursor += size
        self.assertEqual(cursor, len(blob))
        self.assertEqual(payloads[2], synthetic_map())
        self.assertEqual(payloads[3], synthetic_render())
        metadata = json.loads(payloads[1])
        self.assertEqual(metadata["state_id"], "SYNTHETIC_TEST")
        self.assertEqual(metadata["status"], "SOURCE_BACKED_PARTIAL")
        for key, payload in (("map", synthetic_map()), ("render", synthetic_render())):
            self.assertEqual(metadata["inputs"][key]["bytes"], len(payload))
            self.assertEqual(metadata["inputs"][key]["sha256"], hashlib.sha256(payload).hexdigest())
        self.assertFalse(metadata["source_release_payload"])
        self.assertEqual(result["sha256"], hashlib.sha256(blob).hexdigest())

    def test_deterministic_across_output_paths(self):
        first = self.build()
        second_path = self.root / "second.mmlpkg"
        second = self.build(out_path=second_path)
        self.assertEqual(self.output.read_bytes(), second_path.read_bytes())
        self.assertEqual(first["sha256"], second["sha256"])

    def test_optional_negative_material_section_is_copied_and_cli_compatible(self):
        negative = self.root / "test.l3nm"
        negative.write_bytes(synthetic_negative_materials())
        with patch("sys.stdout"):
            self.assertEqual(wrapper.main(["--map", str(self.map_path), "--render", str(self.render_path),
                "--state-id", "SYNTHETIC_TEST", "--out", str(self.output),
                "--negative-materials", str(negative)]), 0)
        blob = self.output.read_bytes()
        self.assertEqual(struct.unpack_from("<H", blob, 6)[0], 4)
        kind, offset, size, crc = struct.unpack_from("<IIII", blob, 20 + 3 * 16)
        self.assertEqual((kind, size, crc), (32, negative.stat().st_size, zlib.crc32(negative.read_bytes())))
        self.assertEqual(blob[offset:offset + size], negative.read_bytes())

    def test_negative_material_area_count_rows_and_crc_are_validated_before_publish(self):
        negative = self.root / "test.l3nm"
        cases = [(synthetic_negative_materials(2), "area count mismatch")]
        original = synthetic_negative_materials()
        cases += [(b"BAD!" + original[4:], "input is not"), (original[:-1], "byte length")]
        for offset, fmt, value, message in ((4, "<H", 2, "version"), (6, "<H", 0, "area count"),
                (8, "<I", 0, "size"), (12, "<I", 1, "CRC"),
                (16, "<I", 0x1234001e, "binding"), (20, "<B", 64, "binding"),
                (21, "<B", 2, "flags"), (22, "<H", 1, "reserved"),
                (24, "<I", 0x1234001d, "binding")):
            malformed = bytearray(original)
            struct.pack_into(fmt, malformed, offset, value)
            if offset >= 16:
                struct.pack_into("<I", malformed, 12, zlib.crc32(malformed[16:]))
            cases.append((bytes(malformed), message))
        for blob, message in cases:
            negative.write_bytes(blob)
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                self.build(negative_materials_path=negative)
            self.assertFalse(self.output.exists())

    def runtime_inputs(self):
        flow, ebd = self.root / "test.l3flow", self.root / "test.l3ebd"
        flow.write_bytes(synthetic_flow())
        ebd.write_bytes(synthetic_ebd())
        return flow, ebd

    def test_optional_spawn_catalog_is_copied_without_changing_legacy_inputs(self):
        spawns=self.root/"test.l3spawn";spawns.write_bytes(synthetic_spawns())
        self.map_path.write_bytes(synthetic_map(5))
        with patch("sys.stdout"):
            self.assertEqual(wrapper.main(["--map",str(self.map_path),"--render",str(self.render_path),
                "--state-id","SYNTHETIC_TEST","--out",str(self.output),"--spawns",str(spawns)]),0)
        blob=self.output.read_bytes()
        self.assertEqual(struct.unpack_from("<H",blob,6)[0],4)
        kind,offset,size,crc=struct.unpack_from("<4I",blob,20+3*16)
        self.assertEqual((kind,size,crc),(34,len(synthetic_spawns()),zlib.crc32(synthetic_spawns())))
        self.assertEqual(blob[offset:offset+size],synthetic_spawns())
        result=self.build(out_path=self.root/"second.mmlpkg",spawns_path=spawns)
        self.assertEqual(result["metadata"]["components"]["spawns"]["backend"],"ST00_IDENTITY_V1")
        self.assertEqual(result["metadata"]["inputs"]["spawns"]["sha256"],hashlib.sha256(synthetic_spawns()).hexdigest())
        self.assertEqual(blob,(self.root/"second.mmlpkg").read_bytes())

    def test_spawn_catalog_bounds_domains_identity_and_crc_fail_before_publish(self):
        spawns=self.root/"test.l3spawn";spawns.write_bytes(synthetic_spawns())
        with self.assertRaisesRegex(ValueError,"five local map areas"):
            self.build(spawns_path=spawns)
        self.map_path.write_bytes(synthetic_map(5))
        for offset,value in ((8,4),(12,65),(16,257),(20,41),(40,1),(80,128),
                             (80,128<<16),(88,1),(116,2048)):
            data=bytearray(synthetic_spawns());struct.pack_into("<I",data,offset,value)
            struct.pack_into("<I",data,36,zlib.crc32(data[40:]));spawns.write_bytes(data)
            with self.subTest(offset=offset,value=value),self.assertRaises(ValueError):
                self.build(spawns_path=spawns)
            self.assertFalse(self.output.exists())
        data=bytearray(synthetic_spawns());data[96]=0x80
        struct.pack_into("<I",data,36,zlib.crc32(data[40:]));spawns.write_bytes(data)
        with self.assertRaisesRegex(ValueError,"conditional object"):
            self.build(spawns_path=spawns)
        data=bytearray(synthetic_spawns());data[-1]^=1;spawns.write_bytes(data)
        with self.assertRaisesRegex(ValueError,"CRC"):
            self.build(spawns_path=spawns)
        spawns.write_bytes(synthetic_spawns())
        flow,ebd=self.runtime_inputs()
        with self.assertRaisesRegex(ValueError,"identity runtime area selectors"):
            self.build(spawns_path=spawns,runtime_flow_path=flow,runtime_ebd_paths=[(1,3,ebd)])
        self.assertFalse(self.output.exists())
        self.build(spawns_path=spawns,runtime_flow_path=flow,runtime_ebd_paths=[(3,3,ebd)])

    def test_runtime_bindings_preserve_exact_slices_and_explicit_area_identity(self):
        flow, ebd = self.runtime_inputs()
        self.map_path.write_bytes(synthetic_map(2))
        with patch("sys.stdout"):
            self.assertEqual(wrapper.main(["--map", str(self.map_path), "--render", str(self.render_path),
                "--state-id", "SYNTHETIC_TEST", "--out", str(self.output),
                "--runtime-flow", str(flow), "--runtime-ebd", f"1:3:{ebd}"]), 0)
        blob = self.output.read_bytes()
        self.assertEqual(struct.unpack_from("<H", blob, 6)[0], 4)
        kind, offset, size, crc = struct.unpack_from("<IIII", blob, 20 + 3 * 16)
        runtime = blob[offset:offset + size]
        self.assertEqual((kind, crc), (33, zlib.crc32(runtime)))
        header = struct.unpack_from("<4sHH8I", runtime)
        self.assertEqual(header[:7], (b"L3RB", 1, 40, 1, 0, 1, 40))
        self.assertEqual(header[9], len(runtime))
        self.assertEqual(header[10], zlib.crc32(runtime[40:]))
        local, source, ebd_offset, ebd_bytes, ebd_crc = struct.unpack_from("<5I", runtime, 40)
        self.assertEqual((local, source, ebd_bytes, ebd_crc), (1, 3, len(synthetic_ebd()), zlib.crc32(synthetic_ebd())))
        self.assertEqual(runtime[header[7]:header[7] + header[8]], synthetic_flow())
        self.assertEqual(runtime[ebd_offset:ebd_offset + ebd_bytes], synthetic_ebd())
        meta_kind, meta_offset, meta_size, _ = struct.unpack_from("<IIII", blob, 20)
        self.assertEqual(meta_kind, 1)
        metadata = json.loads(blob[meta_offset:meta_offset + meta_size])
        self.assertEqual(metadata["components"]["runtime_bindings"]["backend"], "ST00_FLOW_V1")
        self.assertEqual(metadata["inputs"]["runtime_bindings"]["areas"][0]["local_area"], 1)
        self.assertFalse(metadata["source_release_payload"])

    def test_runtime_binding_mapping_pairing_and_aliases_are_rejected(self):
        flow, ebd = self.runtime_inputs()
        for changes, message in (({"runtime_flow_path": flow}, "together"),
                ({"runtime_ebd_paths": [(0, 3, ebd)]}, "together"),
                ({"runtime_flow_path": flow, "runtime_ebd_paths": [(0, 3, ebd), (0, 4, ebd)]}, "aliases"),
                ({"runtime_flow_path": flow, "runtime_ebd_paths": [(1, 3, ebd)]}, "area index"),
                ({"runtime_flow_path": flow, "runtime_ebd_paths": [(0, 5, ebd)]}, "area index"),
                ({"runtime_flow_path": flow, "runtime_ebd_paths": [(True, 3, ebd)]}, "binding"),
                ({"runtime_flow_path": self.map_path, "runtime_ebd_paths": [(0, 3, ebd)]}, "aliases")):
            with self.subTest(changes=changes), self.assertRaisesRegex(ValueError, message):
                self.build(**changes)
            self.assertFalse(self.output.exists())
        alias = self.root / "hardlinked.l3ebd"
        os.link(ebd, alias)
        self.map_path.write_bytes(synthetic_map(2))
        with self.assertRaisesRegex(ValueError, "same file"):
            self.build(runtime_flow_path=flow, runtime_ebd_paths=[(0, 3, ebd), (1, 4, alias)])
        with patch("sys.stderr"):
            self.assertEqual(wrapper.main(["--map", str(self.map_path), "--render", str(self.render_path),
                "--state-id", "SYNTHETIC_TEST", "--out", str(self.output),
                "--runtime-flow", str(flow), "--runtime-ebd", "bad"]), 1)

    def test_runtime_cooked_structures_and_crcs_are_validated_before_publish(self):
        flow, ebd = self.runtime_inputs()
        for path, original, offsets in ((flow, synthetic_flow(), ((4, "<H", 2), (8, "<I", 15),
                (12, "<I", 4), (20, "<I", 41), (24, "<I", 279), (28, "<I", 299),
                (32, "<I", 0), (36, "<I", 1), (40, "<I", 1), (40 + 44, "<I", 1))),
                (ebd, synthetic_ebd(), ((4, "<H", 1), (8, "<I", 3), (12, "<I", 2),
                (20, "<I", 39), (24, "<I", 0), (28, "<I", 1), (32, "<I", 55),
                (36, "<I", 3), (40, "<B", 0), (44, "<I", 19), (56, "<I", 2),
                (64, "<I", 0x803fffff)))):
            for offset, fmt, value in offsets:
                malformed = bytearray(original)
                struct.pack_into(fmt, malformed, offset, value)
                crc_offset = 36 if path == flow else 28
                if offset >= 40:
                    struct.pack_into("<I", malformed, crc_offset, zlib.crc32(malformed[40:]))
                path.write_bytes(malformed)
                with self.subTest(path=path, offset=offset), self.assertRaises(ValueError):
                    self.build(runtime_flow_path=flow, runtime_ebd_paths=[(0, 3, ebd)])
                self.assertFalse(self.output.exists())
                path.write_bytes(original)

    def test_runtime_bindings_are_sorted_deterministic_and_section_bounded(self):
        flow, ebd = self.runtime_inputs()
        second = self.root / "second.l3ebd"
        second.write_bytes(synthetic_ebd())
        self.map_path.write_bytes(synthetic_map(2))
        self.build(runtime_flow_path=flow, runtime_ebd_paths=[(1, 4, second), (0, 3, ebd)])
        other = self.root / "second.mmlpkg"
        self.build(out_path=other, runtime_flow_path=flow, runtime_ebd_paths=[(0, 3, ebd), (1, 4, second)])
        self.assertEqual(self.output.read_bytes(), other.read_bytes())
        self.output.unlink()
        with patch.object(wrapper, "MAX_INPUT_BYTES", len(synthetic_flow()) + 1):
            with self.assertRaisesRegex(ValueError, "section admission"):
                self.build(runtime_flow_path=flow, runtime_ebd_paths=[(0, 3, ebd)])
        self.assertFalse(self.output.exists())

    def test_negative_material_paths_preserve_strict_alias_and_no_clobber_policy(self):
        with self.assertRaisesRegex(ValueError, "aliases"):
            self.build(negative_materials_path=self.map_path)
        negative = self.root / "test.l3nm"
        negative.write_bytes(synthetic_negative_materials())
        linked = self.root / "linked.l3nm"
        linked.symlink_to(negative)
        with self.assertRaises(OSError):
            self.build(negative_materials_path=linked)
        self.output.write_bytes(b"prior checkpoint")
        with self.assertRaisesRegex(ValueError, "already exists"):
            self.build(negative_materials_path=negative)
        self.assertEqual(self.output.read_bytes(), b"prior checkpoint")

    def test_bad_magic_and_suffix_are_rejected(self):
        wrong = self.root / "raw.bin"
        wrong.write_bytes(synthetic_map())
        with self.assertRaisesRegex(ValueError, "l3map"):
            self.build(map_path=wrong)
        for path in (self.map_path, self.render_path):
            original = path.read_bytes()
            path.write_bytes(b"XXXX" + original[4:])
            with self.assertRaisesRegex(ValueError, "input is not"):
                self.build()
            path.write_bytes(original)
        self.assertFalse(self.output.exists())

    def test_bad_version_size_and_crc_are_rejected(self):
        original = self.map_path.read_bytes()
        for offset, value, message in ((4, 2, "version"), (76, 0, "size"), (80, 1, "CRC")):
            blob = bytearray(original)
            struct.pack_into("<H" if offset == 4 else "<I", blob, offset, value)
            self.map_path.write_bytes(blob)
            with self.assertRaisesRegex(ValueError, message):
                self.build()
        self.assertFalse(self.output.exists())

    def test_existing_output_is_preserved(self):
        self.output.write_bytes(b"prior checkpoint")
        with self.assertRaisesRegex(ValueError, "already exists"):
            self.build()
        self.assertEqual(self.output.read_bytes(), b"prior checkpoint")
        self.assertEqual(sorted(p.name for p in self.root.iterdir()),
                         ["test.l3map", "test.l3render", "test.mmlpkg"])

    def test_source_tree_output_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "source tree"):
            self.build(out_path=ROOT / "forbidden-test.mmlpkg")
        self.assertFalse((ROOT / "forbidden-test.mmlpkg").exists())

    def test_symlink_inputs_and_output_parent_are_rejected(self):
        linked = self.root / "linked.l3map"
        linked.symlink_to(self.map_path)
        with self.assertRaises(OSError):
            self.build(map_path=linked)
        output_dir = self.root / "linked-output"
        output_dir.symlink_to(self.root, target_is_directory=True)
        with self.assertRaises(OSError):
            self.build(out_path=output_dir / "test.mmlpkg")
        self.assertFalse(self.output.exists())

    def test_output_symlink_is_not_followed_or_replaced(self):
        self.output.symlink_to(self.map_path)
        original = self.map_path.read_bytes()
        with self.assertRaisesRegex(ValueError, "already exists"):
            self.build()
        self.assertTrue(self.output.is_symlink())
        self.assertEqual(self.map_path.read_bytes(), original)

    def test_nonregular_input_is_rejected_without_waiting_for_a_writer(self):
        fifo = self.root / "fifo.l3map"
        os.mkfifo(fifo)
        with self.assertRaisesRegex(ValueError, "regular"):
            self.build(map_path=fifo)
        self.assertFalse(self.output.exists())

    def test_temporary_name_collision_preserves_other_files(self):
        colliding = self.root / ".test.mmlpkg.collision"
        colliding.write_bytes(b"another process owns this")
        with patch.object(wrapper.secrets, "token_hex", return_value="collision"):
            with self.assertRaisesRegex(OSError, "temporary"):
                self.build()
        self.assertEqual(colliding.read_bytes(), b"another process owns this")
        self.assertFalse(self.output.exists())

    def test_alias_and_owner_data_paths_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "aliases"):
            self.build(map_path=str(self.root / "alias") + "/../test.l3map")
        with self.assertRaisesRegex(ValueError, "aliases"):
            self.build(render_path=self.map_path)
        with self.assertRaisesRegex(ValueError, "owner-data"):
            self.build(map_path=self.root / "owner-data" / "test.l3map")

    def test_input_and_package_caps(self):
        with patch.object(wrapper, "MAX_INPUT_BYTES", 100):
            with self.assertRaisesRegex(ValueError, "size bounds"):
                self.build()
        with patch.object(wrapper, "MAX_PACKAGE_BYTES", 100):
            with self.assertRaisesRegex(ValueError, "admission cap"):
                self.build()
        self.assertFalse(self.output.exists())

    def test_state_id_and_output_suffix(self):
        for state in ("", "../area", "area\n", "a" * 129):
            with self.assertRaisesRegex(ValueError, "state-id"):
                self.build(state_id=state)
        with self.assertRaisesRegex(ValueError, "mmlpkg"):
            self.build(out_path=self.root / "bad.zip")

    def test_cli_uses_same_validating_api(self):
        with patch("sys.stdout"):
            self.assertEqual(wrapper.main(["--map", str(self.map_path), "--render", str(self.render_path),
                                           "--state-id", "SYNTHETIC_TEST", "--out", str(self.output)]), 0)
        with patch("sys.stderr"):
            self.assertEqual(wrapper.main(["--map", str(self.map_path), "--render", str(self.render_path),
                                           "--state-id", "SYNTHETIC_TEST", "--out", str(self.output)]), 1)


if __name__ == "__main__":
    unittest.main()

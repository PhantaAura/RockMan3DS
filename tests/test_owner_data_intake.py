#!/usr/bin/env python3
"""Security and compatibility checks using synthetic, original fixture bytes."""
from pathlib import Path
import hashlib
import json
import os
import stat
import struct
import sys
import tempfile
import unittest
from unittest import mock
import warnings
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import owner_data_intake as intake


class OwnerDataIntakeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mml-intake-test-")
        self.base = Path(self.temp.name).resolve()
        self.owner = self.base / "owner"
        self.owner.mkdir()
        self.payload = {
            "ROCK_NEO.EXE": b"synthetic opaque fixture; never executed",
            "SLUS_006.03": b"synthetic loader fixture",
            "SYSTEM.CNF": b"synthetic system fixture",
            "CDDATA/DAT/ST00.BIN": b"synthetic geometry fixture",
            "CDDATA/DAT/ST01.BIN": b"synthetic additional data fixture",
            "CDDATA/XA/XA37S00E.XA": b"synthetic audio fixture",
            "CDDATA/STR/CAPCOM20.STR": b"synthetic video fixture",
        }
        for relative, data in self.payload.items():
            path = self.owner / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        profile = {
            "format_version": 1, "profile_id": "synthetic-test-profile",
            "counts": {"DAT": 2, "XA": 1, "STR": 1},
            "files": [{"path": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                      for name, data in sorted(self.payload.items())],
        }
        self.profile = self.base / "profile.json"
        self.profile.write_text(json.dumps(profile))
        self.profile_patch = mock.patch.object(intake, "PROFILE_PATH", self.profile)
        self.profile_patch.start()
        self.stage = self.base / "stage"

    def tearDown(self):
        self.profile_patch.stop()
        self.temp.cleanup()

    def make_zip(self, prefix="dump/", extras=()):
        path = self.base / "owner.zip"
        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in self.payload.items():
                archive.writestr(prefix + name, data)
            for name, data in extras:
                archive.writestr(name, data)
        return path

    def test_directory_root_cddata_and_dat(self):
        for path in (self.owner, self.owner / "CDDATA", self.owner / "CDDATA/DAT"):
            with self.subTest(path=path):
                root, report = intake.prepare_owner_input(path, self.stage)
                self.assertEqual(root, self.owner)
                self.assertEqual(report["verified_file_count"], 7)
                self.assertFalse(report["executables_executed"])
                self.assertEqual(report["profile_sha256"], hashlib.sha256(self.profile.read_bytes()).hexdigest())
                self.assertFalse(self.stage.exists())

    def test_non_anchor_alteration_is_rejected(self):
        path = self.owner / "CDDATA/DAT/ST01.BIN"
        data = path.read_bytes()
        path.write_bytes(bytes([data[0] ^ 1]) + data[1:])
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "SHA-256 mismatch"):
            intake.prepare_owner_input(self.owner, self.stage)

    def test_unexpected_canonical_file_is_rejected(self):
        (self.owner / "CDDATA/DAT/EXTRA.BIN").write_bytes(b"extra")
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "inventory mismatch"):
            intake.prepare_owner_input(self.owner, self.stage)

    def test_missing_file_is_rejected(self):
        (self.owner / "ROCK_NEO.EXE").unlink()
        with self.assertRaises(intake.OwnerDataIntakeError):
            intake.prepare_owner_input(self.owner, self.stage)

    def test_file_symlink_is_rejected(self):
        path = self.owner / "CDDATA/DAT/ST01.BIN"
        path.unlink()
        path.symlink_to(self.owner / "CDDATA/DAT/ST00.BIN")
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "regular files"):
            intake.prepare_owner_input(self.owner, self.stage)

    def test_input_directory_symlink_is_rejected(self):
        link = self.base / "link"
        link.symlink_to(self.owner, target_is_directory=True)
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "Symlinks"):
            intake.prepare_owner_input(link, self.stage)

    def test_nested_directory_is_rejected(self):
        (self.owner / "CDDATA/DAT/nested").mkdir()
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "regular files"):
            intake.prepare_owner_input(self.owner, self.stage)

    def test_existing_stage_is_preserved(self):
        self.stage.mkdir()
        sentinel = self.stage / "sentinel"
        sentinel.write_bytes(b"preserve")
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "already exists"):
            intake.prepare_owner_input(self.make_zip(), self.stage)
        self.assertEqual(sentinel.read_bytes(), b"preserve")

    def test_stage_cannot_overlap_owner_or_source_project(self):
        for stage in (self.owner, self.owner / "staged", intake.PROJECT_ROOT / "staged"):
            with self.subTest(stage=stage), self.assertRaisesRegex(intake.OwnerDataIntakeError, "separate"):
                intake.prepare_owner_input(self.owner, stage)

    def test_stage_symlink_ancestor_is_rejected(self):
        link = self.base / "stage-parent"
        link.symlink_to(self.base, target_is_directory=True)
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "Symlinks"):
            intake.prepare_owner_input(self.make_zip(), link / "stage")

    def test_zip_extracts_only_profile_files(self):
        archive = self.make_zip(extras=(("dump/notes.txt", b"ignored"),))
        before = hashlib.sha256(archive.read_bytes()).hexdigest()
        root, report = intake.prepare_owner_input(archive, self.stage)
        self.assertEqual(root, self.stage / "extracted")
        self.assertEqual(report["archive_sha256"], before)
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), before)
        self.assertEqual({p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}, set(self.payload))
        for name, data in self.payload.items():
            self.assertEqual((root / name).read_bytes(), data)
        self.assertFalse((root / "notes.txt").exists())

    def test_flat_zip_root_is_supported(self):
        root, _ = intake.prepare_owner_input(self.make_zip(prefix=""), self.stage)
        self.assertEqual((root / "SYSTEM.CNF").read_bytes(), self.payload["SYSTEM.CNF"])

    def test_unsafe_zip_paths_are_rejected_even_if_ignored(self):
        for path in ("../escaped", "/absolute", "C:/drive", "folder\\escaped", "./dot", "double//slash"):
            with self.subTest(path=path), self.assertRaisesRegex(intake.OwnerDataIntakeError, "unsafe path"):
                intake.prepare_owner_input(self.make_zip(extras=((path, b"unsafe"),)), self.stage)
            self.assertFalse(self.stage.exists())

    def test_zip_symlink_is_rejected_even_if_ignored(self):
        archive_path = self.make_zip()
        info = zipfile.ZipInfo("ignored-link")
        info.create_system = 3
        info.external_attr = (stat.S_IFLNK | 0o777) << 16
        with zipfile.ZipFile(archive_path, "a") as archive:
            archive.writestr(info, "../elsewhere")
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "nonregular"):
            intake.prepare_owner_input(archive_path, self.stage)

    def test_encrypted_zip_flags_are_rejected(self):
        archive_path = self.make_zip()
        data = bytearray(archive_path.read_bytes())
        for signature, flag_offset in ((b"PK\x03\x04", 6), (b"PK\x01\x02", 8)):
            start = data.find(signature)
            flags = struct.unpack_from("<H", data, start + flag_offset)[0]
            struct.pack_into("<H", data, start + flag_offset, flags | 1)
        archive_path.write_bytes(data)
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "Encrypted ZIP"):
            intake.prepare_owner_input(archive_path, self.stage)

    def test_duplicate_zip_members_are_rejected(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            archive = self.make_zip(extras=(("dump/ROCK_NEO.EXE", b"duplicate"),))
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "duplicate"):
            intake.prepare_owner_input(archive, self.stage)

    def test_multiple_zip_roots_are_rejected(self):
        archive = self.make_zip(extras=(("other/ROCK_NEO.EXE", b"other"),))
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "exactly one"):
            intake.prepare_owner_input(archive, self.stage)

    def test_extra_zip_game_file_is_rejected(self):
        archive = self.make_zip(extras=(("dump/CDDATA/DAT/EXTRA.BIN", b"extra"),))
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "Unexpected file"):
            intake.prepare_owner_input(archive, self.stage)

    def test_incomplete_zip_is_rejected(self):
        self.payload.pop("CDDATA/DAT/ST01.BIN")
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "incomplete"):
            intake.prepare_owner_input(self.make_zip(), self.stage)

    def test_hash_failure_removes_partial_staging(self):
        data = self.payload["CDDATA/DAT/ST01.BIN"]
        self.payload["CDDATA/DAT/ST01.BIN"] = bytes([data[0] ^ 1]) + data[1:]
        archive = self.make_zip()
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "SHA-256 mismatch"):
            intake.prepare_owner_input(archive, self.stage)
        self.assertFalse(self.stage.exists())

    def test_disc_inputs_are_explicitly_unsupported(self):
        for extension in ("chd", "bin", "cue", "iso"):
            path = self.base / f"disc.{extension}"
            path.write_bytes(b"synthetic unsupported input")
            with self.subTest(extension=extension), self.assertRaisesRegex(intake.OwnerDataIntakeError, "Disc images"):
                intake.prepare_owner_input(path, self.stage)
        self.assertFalse(self.stage.exists())

    def test_invalid_zip_is_rejected(self):
        archive = self.base / "invalid.zip"
        archive.write_bytes(b"not a ZIP")
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "Could not read"):
            intake.prepare_owner_input(archive, self.stage)

    def test_archive_and_member_budgets(self):
        archive = self.make_zip()
        for setting, value in (("MAX_ARCHIVE_BYTES", 1), ("MAX_MEMBERS", 1), ("MAX_SELECTED_BYTES", 1), ("MAX_ENTRY_BYTES", 1)):
            with self.subTest(setting=setting), mock.patch.object(intake, setting, value):
                with self.assertRaises(intake.OwnerDataIntakeError):
                    intake.prepare_owner_input(archive, self.stage)
            self.assertFalse(self.stage.exists())

    def test_invalid_profile_is_rejected(self):
        profile = json.loads(self.profile.read_text())
        profile["files"][0]["path"] = "../escaped"
        self.profile.write_text(json.dumps(profile))
        with self.assertRaisesRegex(intake.OwnerDataIntakeError, "Invalid owner-data profile"):
            intake.prepare_owner_input(self.owner, self.stage)


if __name__ == "__main__":
    unittest.main()

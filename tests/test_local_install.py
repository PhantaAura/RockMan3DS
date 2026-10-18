#!/usr/bin/env python3
"""Offline installer regressions. Every game-data byte is an original fixture.

Integration cases replace expensive compiler/cooker processes, while retaining
the real intake, content graph, task cache, package, manifest and SD validators.
"""
from pathlib import Path
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_mml3ds as installer
import mml_pipeline as pipeline
import owner_data_intake as intake
import build_hardware_test as hardware
from test_cooked_area import synthetic_map, synthetic_render, synthetic_negative_materials


def make_pack(path, negative_materials=None, spawns=None):
    """A complete manifest inventory, with small original opaque sidecars."""
    files = {"st00.l3map": synthetic_map(), "st00.l3render": synthetic_render(),
             "st00.l3flow": b"synthetic flow", "reference.l3ref": b"synthetic refs",
             "messages/st00_01.l3msg": b"synthetic message one",
             "messages/st00_02.l3msg": b"synthetic message two",
             "player/player.l3pc": b"synthetic player",
             "player/main.l3anim": b"synthetic animation",
             "player/main.l3skel": b"synthetic skeleton",
             "player/buster.l3bp": b"synthetic buster profile"}
    if negative_materials is not None:
        files['st00.l3nm'] = negative_materials
    if spawns is not None:
        files['st00.l3spawn'] = spawns
    for part in ("body", "head", "right", "left", "feet"):
        files[f"player/{part}.l3model"] = ("synthetic " + part).encode()
    for area in range(5):
        files[f"ebd/area_{area:02d}.l3ebd"] = b"synthetic event refs"
        files[f"materials/area_{area:02d}.l3mc"] = b"synthetic terrain materials"
        files[f"materials/player_area_{area:02d}.l3mc"] = b"synthetic player materials"
    outputs = []
    for name, data in sorted(files.items()):
        destination = path / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        outputs.append({"path": "legends3ds/tutorial/" + name, "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest()})
    (path / "manifest.json").write_text(json.dumps({"format": 1, "outputs": outputs}))
    return path


def synthetic_container():
    blob = bytearray()
    for suffix, load in (("IDX", 0x8015C000), ("MDT", 0x80164000),
                         ("STG", 0x80194000), ("HED", 0x8013A000)):
        internal = (r"..\MAP\DATA\ST00_MAP." + suffix if suffix in ("IDX", "MDT")
                    else r"..\SCR\DATA\ST00." + suffix).encode()
        record = bytearray(4096)
        payload = ("synthetic " + suffix).encode()
        struct.pack_into("<II", record, 0, 0, len(payload))
        struct.pack_into("<I", record, 12, load)
        record[64:64 + len(internal)] = internal
        record[2048:2048 + len(payload)] = payload
        blob += record
    return bytes(blob)


class LocalInstallTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mml-local-install-test-")
        self.base = Path(self.temp.name).resolve()
        self.owner, self.out, self.cache = (self.base / name for name in ("owner", "out", "cache"))
        self.owner.mkdir()
        self.payloads = {"ROCK_NEO.EXE": b"synthetic opaque executable; never run",
                         "SLUS_006.03": b"synthetic loader", "SYSTEM.CNF": b"synthetic config"}
        self.payloads.update({"CDDATA/DAT/" + name: synthetic_container() if name == "ST00.BIN"
                             else ("synthetic " + name).encode() for name in installer.CORE_CONTAINERS})
        for name, data in self.payloads.items():
            path = self.owner / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        for name in ("XA", "STR"):
            (self.owner / "CDDATA" / name).mkdir()
        self.profile = self.base / "profile.json"
        self.profile.write_text(json.dumps({"format_version": 1, "profile_id": "synthetic-only",
            "counts": {"DAT": len(installer.CORE_CONTAINERS), "XA": 0, "STR": 0},
            "files": [{"path": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                      for name, data in sorted(self.payloads.items())]}))
        self.profile_patch = patch.object(intake, "PROFILE_PATH", self.profile)
        self.profile_patch.start()
        self.pack = make_pack(self.base / "fixture-pack")
        self.boot = self.base / "boot.mmlpkg"
        self.boot.write_bytes(pipeline.package([(1, pipeline.encoded({"state_id": "SYNTHETIC_TEST"})),
                                               (2, synthetic_map()), (3, synthetic_render())]))
        self.host = self.base / "host"
        self.make_host(self.host)
        self.logs = self.base / "logs"
        self.logs.mkdir()
        self.result = {"commands": []}

    def tearDown(self):
        self.profile_patch.stop()
        self.temp.cleanup()

    def make_host(self, path):
        path.mkdir(parents=True, exist_ok=True)
        for name in installer.TUTORIAL_TOOLS:
            (path / name).write_bytes(("synthetic tool " + name).encode())

    def cook_process(self, command, **kwargs):
        shutil.copytree(self.pack, Path(command[4]) / "legends3ds/tutorial")
        return subprocess.CompletedProcess(command, 0, stdout="", stderr="")

    def integration_process(self, command, **kwargs):
        argv = [str(arg) for arg in command]
        name = Path(argv[0]).name
        if name == "cmake" and "-S" in argv:
            self.make_host(Path(argv[argv.index("-B") + 1]))
        elif name in ("l3cook_st00", "l3cook_render"):
            Path(argv[-1]).write_bytes(synthetic_map() if name == "l3cook_st00" else synthetic_render())
        elif len(argv) > 1 and Path(argv[1]).name == "build_tutorial_pack.py":
            return self.cook_process(argv, **kwargs)
        elif len(argv) > 1 and Path(argv[1]).name == "mml_replay_suite.py":
            destination = Path(argv[4])
            destination.mkdir()
            (destination / "summary.json").write_text(json.dumps({"scenarios": [{"repeatable": True}]}))
        return subprocess.CompletedProcess(command, 0, stdout="", stderr="")

    def test_locations_normalize_dat_and_preserve_existing_output(self):
        actual = installer.validate_locations(self.owner / "CDDATA/DAT", self.out, self.cache)
        self.assertEqual(actual, (self.owner / "CDDATA/DAT", self.out, self.cache))
        self.out.mkdir()
        sentinel = self.out / "sentinel"
        sentinel.write_bytes(b"earlier checkpoint")
        with self.assertRaisesRegex(ValueError, "already exists"):
            installer.install(self.owner, self.out, self.cache, host_only=True)
        self.assertEqual(sentinel.read_bytes(), b"earlier checkpoint")
        self.assertFalse((self.out / "build-result.json").exists())

    def test_hardware_builder_wraps_spawns_only_from_verified_manifest(self):
        legacy=hardware.validate_pack(self.pack)
        command=hardware.existing_area_package_command(self.pack,self.boot,legacy)
        self.assertNotIn("--spawns",command)
        # An undeclared file cannot opt itself into the already verified inputs.
        (self.pack/"st00.l3spawn").write_bytes(b"original synthetic spawn fixture")
        self.assertNotIn("--spawns",hardware.existing_area_package_command(self.pack,self.boot,legacy))
        with self.assertRaisesRegex(ValueError,"inventory differs"):
            hardware.validate_pack(self.pack)
        (self.pack/"st00.l3spawn").unlink()
        declared=make_pack(self.base/"spawn-pack",spawns=b"original synthetic spawn fixture")
        validation=hardware.validate_pack(declared)
        command=hardware.existing_area_package_command(declared,self.boot,validation)
        self.assertEqual(command[command.index("--spawns")+1],declared/"st00.l3spawn")
        (declared/"st00.l3spawn").write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError,"size/SHA-256 mismatch"):
            hardware.validate_pack(declared)

    def test_locations_reject_all_protected_overlap_directions(self):
        for out, cache in ((self.owner / "output", self.cache), (self.owner.parent, self.cache),
                           (ROOT / "forbidden-output", self.cache), (ROOT.parent, self.cache),
                           (self.out, self.owner / "cache"), (self.out, ROOT / "forbidden-cache"),
                           (self.out, self.out / "cache"), (self.out, self.base)):
            with self.subTest(out=out, cache=cache), self.assertRaisesRegex(ValueError, "separate"):
                installer.validate_locations(self.owner / "CDDATA/DAT", out, cache)
        self.assertFalse(self.out.exists())

    def test_locations_reject_symlink_input_output_ancestor_and_cache_entry(self):
        link = self.base / "link"
        link.symlink_to(self.owner, target_is_directory=True)
        for owner, out, cache in ((link, self.out, self.cache),
                                  (self.owner, link / "output", self.cache),
                                  (self.owner, self.out, link / "cache")):
            with self.subTest(owner=owner, out=out, cache=cache), self.assertRaisesRegex(ValueError, "symlink"):
                installer.validate_locations(owner, out, cache)
        self.cache.mkdir()
        (self.cache / "link").symlink_to(self.owner / "ROCK_NEO.EXE")
        with self.assertRaisesRegex(ValueError, "symlink"):
            installer.validate_locations(self.owner, self.out, self.cache)

    def test_locations_reject_nonregular_cache_without_reading_it(self):
        self.cache.mkdir()
        os.mkfifo(self.cache / "fifo")
        with self.assertRaisesRegex(ValueError, "non-regular"):
            installer.validate_locations(self.owner, self.out, self.cache)

    def test_cache_lock_exclusion_and_exception_cleanup(self):
        with self.assertRaisesRegex(RuntimeError, "synthetic failure"):
            with installer.cache_lock(self.cache):
                self.assertTrue((self.cache / ".installer.lock").is_file())
                with self.assertRaisesRegex(ValueError, "locked"):
                    with installer.cache_lock(self.cache):
                        self.fail("a second installer acquired the same lock")
                raise RuntimeError("synthetic failure")
        self.assertFalse((self.cache / ".installer.lock").exists())

    def test_tutorial_recipe_changes_with_input_or_tool_bytes(self):
        original = installer.tutorial_key(self.owner, self.host)
        for path in (self.owner / "ROCK_NEO.EXE", self.owner / "CDDATA/DAT/ST00.BIN",
                     self.host / installer.TUTORIAL_TOOLS[0]):
            before = path.read_bytes()
            path.write_bytes(before + b"change")
            self.assertNotEqual(original, installer.tutorial_key(self.owner, self.host))
            path.write_bytes(before)
            self.assertEqual(original, installer.tutorial_key(self.owner, self.host))

    def tutorial(self):
        return installer.obtain_tutorial(self.owner, self.host, self.cache, self.logs, self.result, {})

    def test_scene_cooker_changes_invalidate_tutorial_cache(self):
        original = installer.tutorial_key(self.owner, self.host)
        digest = installer.digest_file
        for name in ("cook_source_fade.py", "cook_source_event_phase5.py", "cook_source_event_phase6.py", "cook_source_scene_view.py"):
            target = ROOT / "tools" / name
            with patch.object(installer, "digest_file", side_effect=lambda path: (
                    "0" * 64 if path == target else digest(path))):
                self.assertNotEqual(original, installer.tutorial_key(self.owner, self.host))
        with patch.object(installer, "digest_file", side_effect=lambda path: (
                "0" * 64 if path.suffix == ".cpp" else digest(path))):
            self.assertEqual(original, installer.tutorial_key(self.owner, self.host))

    def test_tutorial_cache_hit_uses_real_manifest_verification(self):
        with patch.object(installer.subprocess, "run", side_effect=self.cook_process) as process:
            cooked = self.tutorial()
            self.assertFalse(self.result["tutorial"]["cache_hit"])
            self.assertEqual(process.call_count, 1)
            self.assertEqual(self.tutorial(), cooked)
            self.assertTrue(self.result["tutorial"]["cache_hit"])
            self.assertEqual(process.call_count, 1)
        self.assertEqual(installer.validate_pack(cooked), installer.validate_pack(self.pack))

    def test_tutorial_corruption_is_quarantined_and_recooked(self):
        with patch.object(installer.subprocess, "run", side_effect=self.cook_process) as process:
            cooked = self.tutorial()
            (cooked / "st00.l3map").write_bytes(b"corrupt")
            rebuilt = self.tutorial()
            self.assertEqual(process.call_count, 2)
            self.assertFalse(self.result["tutorial"]["cache_hit"])
            self.assertEqual((rebuilt / "st00.l3map").read_bytes(), synthetic_map())
        invalid = list((self.cache / "tutorial").glob("*.invalid-*"))
        self.assertEqual(len(invalid), 1)
        self.assertEqual((invalid[0] / "pack/legends3ds/tutorial/st00.l3map").read_bytes(), b"corrupt")

    def test_tutorial_invalid_manifest_and_unlisted_files_cannot_hit(self):
        with patch.object(installer.subprocess, "run", side_effect=self.cook_process) as process:
            cooked = self.tutorial()
            (cooked / "manifest.json").write_text("{")
            self.tutorial()
            (cooked / "unexpected.l3map").write_bytes(b"unlisted")
            self.tutorial()
            self.assertEqual(process.call_count, 3)
            self.assertFalse(self.result["tutorial"]["cache_hit"])

    def test_failed_tutorial_cook_is_never_marked_valid(self):
        def corrupt(command, **kwargs):
            result = self.cook_process(command, **kwargs)
            (Path(command[4]) / "legends3ds/tutorial/st00.l3map").write_bytes(b"bad")
            return result
        with patch.object(installer.subprocess, "run", side_effect=corrupt):
            with self.assertRaisesRegex(ValueError, "mismatch"):
                self.tutorial()
        self.assertNotIn("tutorial", self.result)

    def test_boot_sections_and_strict_sidecar_identity(self):
        sections = installer.package_sections(self.boot)
        self.assertEqual(sections[2], synthetic_map())
        self.assertEqual(sections[3], synthetic_render())
        report = installer.verify_boot_pair(self.boot, self.pack)
        self.assertTrue(report["sidecar_map_render_byte_identical"])
        self.assertEqual(report["native_bindings"], ["map", "render"])
        # A different, structurally valid map still fails the binding contract.
        other = bytearray(synthetic_map())
        struct.pack_into("<I", other, 8, 1)
        self.boot.write_bytes(pipeline.package([(2, bytes(other)), (3, synthetic_render())]))
        with self.assertRaisesRegex(ValueError, "binding refused"):
            installer.verify_boot_pair(self.boot, self.pack)

    def test_optional_negative_binding_and_pack_inventory_are_byte_identical(self):
        negative = synthetic_negative_materials()
        make_pack(self.pack, negative)
        installer.validate_pack(self.pack)
        self.boot.write_bytes(pipeline.package([(2,synthetic_map()),(3,synthetic_render()),(32,negative)]))
        report = installer.verify_boot_pair(self.boot,self.pack)
        self.assertTrue(report['sidecar_negative_materials_byte_identical'])
        self.assertEqual(report['native_bindings'],['map','render','negative_materials'])
        for sections in ([(2,synthetic_map()),(3,synthetic_render())],
                         [(2,synthetic_map()),(3,synthetic_render()),(32,b'other table')]):
            self.boot.write_bytes(pipeline.package(sections))
            with self.assertRaisesRegex(ValueError,'negative materials.*binding refused'):
                installer.verify_boot_pair(self.boot,self.pack)
        self.boot.write_bytes(pipeline.package([(2,synthetic_map()),(3,synthetic_render()),(32,negative)]))
        (self.pack/'st00.l3nm').unlink()
        with self.assertRaisesRegex(ValueError,'negative materials.*binding refused'):
            installer.verify_boot_pair(self.boot,self.pack)

    def test_boot_header_directory_and_payload_corruption_are_rejected(self):
        original = self.boot.read_bytes()
        damaged = [original[:19], b"BAD!" + original[4:]]
        for offset, fmt, value in ((4, "<H", 1), (6, "<H", 65), (8, "<I", len(original) + 1),
                                   (16, "<I", 0)):
            data = bytearray(original)
            struct.pack_into(fmt, data, offset, value)
            damaged.append(bytes(data))
        damaged.append(original[:-1] + bytes([original[-1] ^ 1]))
        for blob in damaged:
            with self.subTest(blob=blob[:20]):
                self.boot.write_bytes(blob)
                with self.assertRaisesRegex(ValueError, "boot package"):
                    installer.package_sections(self.boot)

    def test_boot_duplicate_overlapping_and_out_of_bounds_sections_are_rejected(self):
        original = self.boot.read_bytes()
        first = struct.unpack_from("<IIII", original, 20)
        for index, value in ((0, first[0]), (1, first[1]), (1, 20),
                             (1, len(original) + 1), (2, len(original))):
            blob = bytearray(original)
            struct.pack_into("<I", blob, 36 + index * 4, value)
            struct.pack_into("<I", blob, 16, zlib.crc32(blob[20:68]))
            self.boot.write_bytes(blob)
            with self.subTest(index=index, value=value), self.assertRaisesRegex(ValueError, "boot package section"):
                installer.package_sections(self.boot)

    def test_missing_boot_bindings_are_rejected(self):
        self.boot.write_bytes(pipeline.package([(2, synthetic_map())]))
        with self.assertRaisesRegex(ValueError, "binding refused"):
            installer.verify_boot_pair(self.boot, self.pack)

    def test_sd_deployment_preserves_inventory_and_has_no_fake_binary(self):
        self.out.mkdir()
        report = installer.deploy_sd(self.pack, self.boot, None, self.out)
        sd = self.out / "sd_out"
        destination = sd / "3ds/Legends3DS/data/legends3ds/tutorial"
        self.assertEqual(installer.validate_pack(destination), installer.validate_pack(self.pack))
        self.assertEqual((sd / "3ds/Legends3DS/data/legends3ds/areas/boot.mmlpkg").read_bytes(), self.boot.read_bytes())
        self.assertFalse(list(self.out.rglob("*.3dsx")))
        manifest = json.loads((self.out / "sd-manifest.json").read_text())
        self.assertFalse(manifest["redistributable"])
        self.assertEqual(report["files"], len(manifest["files"]))
        for entry in manifest["files"]:
            self.assertEqual(entry["sha256"], installer.digest_file(sd / entry["path"]))

    def test_invalid_binding_cannot_publish_sd_tree(self):
        self.out.mkdir()
        self.boot.write_bytes(pipeline.package([(2, b"wrong map"), (3, synthetic_render())]))
        with self.assertRaisesRegex(ValueError, "binding refused"):
            installer.deploy_sd(self.pack, self.boot, None, self.out)
        self.assertFalse((self.out / "sd_out").exists())
        self.assertFalse((self.out / "sd-manifest.json").exists())

    def test_sd_existing_output_is_preserved_even_when_empty(self):
        self.out.mkdir()
        previous = self.out / "sd_out"
        previous.mkdir()
        with self.assertRaises((ValueError, OSError)):
            installer.deploy_sd(self.pack, self.boot, None, self.out)
        self.assertEqual(list(previous.iterdir()), [])

    def test_host_only_install_real_synthetic_data_flow(self):
        before = {name: (self.owner / name).read_bytes() for name in self.payloads}
        with patch.object(installer.subprocess, "run", side_effect=self.integration_process), patch("sys.stdout"):
            report = installer.install(self.owner, self.out, self.cache, host_only=True, tutorial_only=True)
        self.assertEqual(report["status"], "DATA_READY")
        self.assertEqual(report["native_build"], "NOT_REQUESTED")
        self.assertEqual(report["content"]["packages"], len(installer.CORE_CONTAINERS))
        self.assertEqual(report["content"]["geometry_families"], 1)
        self.assertEqual(report["content"]["packages_with_errors"], 0)
        self.assertEqual(report["validated_packages"], len(installer.CORE_CONTAINERS))
        self.assertTrue(report["boot_package"]["sidecar_map_render_byte_identical"])
        self.assertEqual(report['area_inspection_probe'],'PASS')
        self.assertTrue(any(Path(command['argv'][0]).name == 'l3area_inspection_probe'
                            for command in report['commands']))
        self.assertFalse(report["ownership_verified"])
        self.assertFalse(report["retail_parity_tested"])
        self.assertFalse(list(self.out.rglob("*.3dsx")))
        self.assertFalse((self.out / "native-source").exists())
        self.assertEqual(json.loads((self.out / "build-result.json").read_text()), report)
        self.assertEqual({name: (self.owner / name).read_bytes() for name in self.payloads}, before)
        self.assertFalse((self.cache / ".installer.lock").exists())

    def test_native_toolchain_unavailable_keeps_data_ready_without_fake_binary(self):
        with patch.dict(os.environ, {"DEVKITARM": str(self.base / "missing-toolchain")}), \
             patch.object(installer.subprocess, "run", side_effect=self.integration_process), patch("sys.stdout"):
            report = installer.install(self.owner, self.out, self.cache, tutorial_only=True)
        self.assertEqual(report["native_build"], "TOOLCHAIN_UNAVAILABLE")
        self.assertEqual(report["status"], "DATA_READY")
        self.assertFalse(list(self.out.rglob("*.3dsx")))

    def test_fresh_checkpoint_reuses_verified_cooked_outputs(self):
        with patch.object(installer.subprocess, "run", side_effect=self.integration_process), patch("sys.stdout"):
            first = installer.install(self.owner, self.out, self.cache, host_only=True, tutorial_only=True)
            next_out = self.base / "next-checkpoint"
            second = installer.install(self.owner, next_out, self.cache, host_only=True, tutorial_only=True)
        self.assertEqual(first["content"]["cache_misses"], 2)
        self.assertEqual(second["content"]["cache_misses"], 0)
        self.assertEqual(second["content"]["cache_hits"], 2)
        self.assertFalse(first["tutorial"]["cache_hit"])
        self.assertTrue(second["tutorial"]["cache_hit"])
        self.assertEqual(first["tutorial"]["recipe_key"], second["tutorial"]["recipe_key"])
        self.assertEqual((self.out / "content/areas/ST00.mmlpkg").read_bytes(),
                         (next_out / "content/areas/ST00.mmlpkg").read_bytes())
        self.assertEqual(first["boot_package"]["sha256"], second["boot_package"]["sha256"])
        self.assertFalse(any(Path(row["argv"][1]).name == "build_tutorial_pack.py"
                             for row in second["commands"] if len(row["argv"]) > 1))

    def test_failed_inherited_checks_prevent_cooking_and_sd_publication(self):
        def failing(command, **kwargs):
            if str(command[0]) == "ctest":
                return subprocess.CompletedProcess(command, 8)
            return self.integration_process(command, **kwargs)
        with patch.object(installer.subprocess, "run", side_effect=failing), patch("sys.stdout"):
            with self.assertRaisesRegex(RuntimeError, "command failed"):
                installer.install(self.owner, self.out, self.cache, host_only=True)
        report = json.loads((self.out / "build-result.json").read_text())
        self.assertEqual(report["status"], "FAILED")
        self.assertEqual(report["commands"][-1]["returncode"], 8)
        self.assertNotIn("host_tests", report)
        self.assertFalse((self.out / "content").exists())
        self.assertFalse((self.cache / "tutorial").exists())
        self.assertFalse((self.out / "sd_out").exists())

    def test_failed_probe_is_recorded_and_never_publishes_sd(self):
        def failing(command, **kwargs):
            if Path(str(command[0])).name == "l3area_runtime_probe":
                return subprocess.CompletedProcess(command, 7)
            return self.integration_process(command, **kwargs)
        with patch.object(installer.subprocess, "run", side_effect=failing), patch("sys.stdout"):
            with self.assertRaisesRegex(RuntimeError, "command failed"):
                installer.install(self.owner, self.out, self.cache, host_only=True, tutorial_only=True)
        report = json.loads((self.out / "build-result.json").read_text())
        self.assertEqual(report["status"], "FAILED")
        self.assertEqual(report["commands"][-1]["returncode"], 7)
        self.assertEqual(Path(report["commands"][-1]["argv"][0]).name, "l3area_runtime_probe")
        self.assertFalse((self.out / "sd_out").exists())
        self.assertFalse((self.out / "sd-manifest.json").exists())
        self.assertFalse((self.cache / ".installer.lock").exists())

    def test_failed_process_launch_is_recorded(self):
        with patch.object(installer.subprocess, "run", side_effect=FileNotFoundError("synthetic missing executable")), patch("sys.stdout"):
            with self.assertRaisesRegex(RuntimeError, "could not launch"):
                installer.install(self.owner, self.out, self.cache, host_only=True)
        report = json.loads((self.out / "build-result.json").read_text())
        self.assertEqual(report["status"], "FAILED")
        self.assertEqual(len(report["commands"]), 1)
        self.assertIn("synthetic missing executable", report["commands"][0]["error"])
        self.assertFalse((self.out / "sd_out").exists())

    def test_cli_reports_failure_without_replacing_checkpoint(self):
        self.out.mkdir()
        (self.out / "sentinel").write_bytes(b"keep")
        with patch("sys.stderr"):
            result = installer.main([str(self.owner), "--out", str(self.out), "--cache", str(self.cache), "--host-only"])
        self.assertEqual(result, 1)
        self.assertEqual((self.out / "sentinel").read_bytes(), b"keep")


class ExternalCacheTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mml-external-cache-test-")
        self.base = Path(self.temp.name).resolve()
        self.tools = self.base / "tools"
        self.tools.mkdir()
        tool = self.tools / "tool"
        tool.write_text(f"#!{sys.executable}\nfrom pathlib import Path\nimport sys\n"
                        "Path(sys.argv[2]).write_bytes(Path(sys.argv[1]).read_bytes() + b'-cooked')\n")
        tool.chmod(0o755)
        self.cache = self.base / "cache"

    def tearDown(self):
        self.temp.cleanup()

    def test_external_cache_reuses_tasks_across_output_checkpoints(self):
        first = pipeline.Cache(self.base / "output-one", self.tools, self.cache)
        data, info = first.run("tool", {"source": b"original synthetic input"}, ["@source", "@out"])
        second = pipeline.Cache(self.base / "output-two", self.tools, self.cache)
        reused, reused_info = second.run("tool", {"source": b"original synthetic input"}, ["@source", "@out"])
        self.assertEqual((data, info), (reused, reused_info))
        self.assertEqual((first.misses, second.hits, second.misses), (1, 1, 0))
        self.assertFalse(first.out.exists())
        self.assertFalse(second.out.exists())
        (self.cache / info["key"] / "payload").write_bytes(b"corrupt")
        rebuilt, _ = second.run("tool", {"source": b"original synthetic input"}, ["@source", "@out"])
        self.assertEqual(rebuilt, b"original synthetic input-cooked")
        self.assertEqual(second.misses, 1)

    def test_corrupt_provenance_is_recooked_instead_of_aborting(self):
        cache = pipeline.Cache(self.base / "output", self.tools, self.cache)
        data, info = cache.run("tool", {"source": b"original synthetic input"}, ["@source", "@out"])
        meta = self.cache / info["key"] / "provenance.json"
        for corrupt in ("{", "{}", "null", "[]"):
            with self.subTest(corrupt=corrupt):
                meta.write_text(corrupt)
                rebuilt, _ = cache.run("tool", {"source": b"original synthetic input"}, ["@source", "@out"])
                self.assertEqual(rebuilt, data)
        self.assertEqual(cache.misses, 5)
        self.assertEqual(cache.hits, 0)

    def test_selection_rejects_duplicate_and_escaping_basenames(self):
        dat = self.base / "owner/CDDATA/DAT"
        dat.mkdir(parents=True)
        (dat / "ST00.BIN").write_bytes(b"synthetic empty container")
        for names in (("ST00.BIN", "ST00.BIN"), ("../ST00.BIN",), ("/ST00.BIN",),
                      ("folder/ST00.BIN",), ("ST00.bin",)):
            with self.subTest(names=names), self.assertRaisesRegex(ValueError, "unique BIN basenames"):
                pipeline.build(dat, self.tools, self.base / "output", container_names=names)
        self.assertFalse((self.base / "output").exists())

    def test_missing_selected_container_is_not_silently_omitted(self):
        dat = self.base / "owner/CDDATA/DAT"
        dat.mkdir(parents=True)
        with self.assertRaisesRegex(ValueError, "selected DAT container is missing"):
            pipeline.build(dat, self.tools, self.base / "output", container_names=("ST00.BIN",))


if __name__ == "__main__":
    unittest.main()

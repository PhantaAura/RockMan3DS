#!/usr/bin/env python3
"""Offline owner-data installation, incremental cooking and optional Old 3DS build.

The input is a supported local extraction folder or extraction ZIP. Every retail
and cooked payload stays outside this distributable source tree. Fingerprints
identify compatible files; they do not prove ownership or gameplay parity.
"""
from __future__ import annotations

import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import sys
import time
import uuid
import zlib

from build_hardware_test import native_snapshot, validate_pack
from build_tutorial_pack import CORE_CONTAINERS, _publish_exclusive
from mml_pipeline import build, encoded, sha
from owner_data_intake import normalize_owner_directory, prepare_owner_input

ROOT = Path(__file__).resolve().parents[1]
TUTORIAL_RECIPE = "tutorial-install/01516-opening-1"
TUTORIAL_SCRIPTS = (
    "build_tutorial_pack.py", "cook_reference_tables.py", "mml_pipeline.py",
    "build_stage_catalog.py", "build_stage_object_catalog.py", "cook_actor_combat.py",
    "cook_source_fade.py", "cook_source_event_phase5.py", "cook_source_event_phase6.py",
    "source_player_head.py", "cook_source_scene_view.py", "cook_source_buster_profile.py", "cook_source_tutorial_triggers.py",
    "cook_hit_proxy.py", "cook_class60_06.py", "cook_reaction_profile.py", "cook_actor_angle.py",
    "cook_source_player_target.py", "cook_source_reward_profile.py",
    "cook_source_event_poll.py", "cook_source_event_phase2.py", "cook_source_event_command.py",
    "cook_source_prop_lift.py", "cook_source_effect_dispatch.py",
)
TUTORIAL_TOOLS = (
    "l3cook_st00", "l3cook_render", "l3cook_map_init", "l3cook_st00_flow", "l3cook_st00_spawn", "l3cook_pbd",
    "l3cook_player_comp", "l3cook_anim", "l3cook_skeleton", "l3cook_msg",
    "l3cook_ebd", "l3cook_texture", "l3cook_palette", "l3cook_material_cache",
    "l3cook_player_material_cache", "l3cook_actor_material_cache", "l3cook_negative_materials", "l3cook_scene_e0_material", "l3cook_scene_actor40_material", "l3cook_frontend_materials",
)


def digest_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def no_symlinks(path: Path) -> None:
    for part in (path, *path.parents):
        if part.is_symlink():
            raise ValueError(f"symlink path is unsupported: {part}")


def overlaps(a: Path, b: Path) -> bool:
    return a == b or a in b.parents or b in a.parents


def validate_locations(owner: Path, out: Path, cache: Path) -> tuple[Path, Path, Path]:
    for path in (owner, out, cache):
        no_symlinks(path.absolute())
    owner, out, cache = owner.resolve(), out.resolve(), cache.resolve()
    data_root = normalize_owner_directory(owner) if owner.is_dir() else owner
    if overlaps(out, ROOT) or overlaps(out, data_root):
        raise ValueError("output must be separate from source and owner input")
    if overlaps(cache, ROOT) or overlaps(cache, data_root) or overlaps(cache, out):
        raise ValueError("cache must be separate from source, owner input and output")
    if out.exists():
        raise ValueError("output already exists; choose a fresh --out checkpoint")
    if cache.exists():
        if not cache.is_dir():
            raise ValueError("cache must be a directory")
        for path in cache.rglob("*"):
            if path.is_symlink() or (not path.is_file() and not path.is_dir()):
                raise ValueError("cache contains a symlink or non-regular entry")
    return owner, out, cache


def write_json(path: Path, value) -> None:
    path.write_bytes(encoded(value))


@contextmanager
def cache_lock(cache: Path):
    cache.mkdir(parents=True, exist_ok=True)
    lock = cache / ".installer.lock"
    try:
        fd = os.open(lock, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    except FileExistsError as error:
        raise ValueError("cache is locked by another installer; if interrupted, confirm it stopped before removing .installer.lock") from error
    try:
        with os.fdopen(fd, "w") as stream:
            stream.write(str(os.getpid()) + "\n")
        yield
    finally:
        lock.unlink(missing_ok=True)


def run(command, log: Path, result: dict, env: dict) -> None:
    argv = [str(arg) for arg in command]
    start = time.monotonic()
    with log.open("w") as stream:
        stream.write(shlex.join(argv) + "\n")
        stream.flush()
        try:
            process = subprocess.run(argv, stdout=stream, stderr=subprocess.STDOUT, env=env)
        except OSError as error:
            stream.write(f"Launch failed: {error}\n")
            result["commands"].append({"argv": argv, "returncode": None,
                                       "seconds": round(time.monotonic() - start, 6),
                                       "log": str(log), "error": str(error)})
            raise RuntimeError(f"command could not launch; see {log}") from error
    result["commands"].append({"argv": argv, "returncode": process.returncode,
                               "seconds": round(time.monotonic() - start, 6),
                               "log": str(log)})
    if process.returncode:
        raise RuntimeError(f"command failed ({process.returncode}); see {log}")


def tutorial_key(owner: Path, host: Path) -> str:
    inputs = [owner / "ROCK_NEO.EXE"] + [owner / "CDDATA/DAT" / name for name in CORE_CONTAINERS]
    tools = [host / name for name in TUTORIAL_TOOLS]
    scripts = [ROOT / "tools" / name for name in TUTORIAL_SCRIPTS]
    recipe = {"recipe": TUTORIAL_RECIPE,
              "inputs": {str(path.relative_to(owner)): digest_file(path) for path in inputs},
              "tools": {path.name: digest_file(path) for path in tools},
              "scripts": {path.name: digest_file(path) for path in scripts}}
    return sha(encoded(recipe))


def obtain_tutorial(owner: Path, host: Path, cache: Path, logs: Path, result: dict, env: dict) -> Path:
    key = tutorial_key(owner, host)
    slot = cache / "tutorial" / key
    pack = slot / "pack"
    hit = False
    if pack.exists():
        try:
            validate_pack(pack / "legends3ds/tutorial")
            hit = True
        except (ValueError, OSError, KeyError, TypeError, json.JSONDecodeError):
            # Retain corrupt generated data for diagnosis; never delete owner data.
            slot.rename(slot.with_name(key + ".invalid-" + uuid.uuid4().hex))
    if not hit:
        slot.mkdir(parents=True, exist_ok=True)
        run([sys.executable, ROOT / "tools/build_tutorial_pack.py", owner / "CDDATA/DAT", host, pack,
             "--with-indexed", "--with-material-cache"], logs / "tutorial-cook.log", result, env)
    validation = validate_pack(pack / "legends3ds/tutorial")
    result["tutorial"] = {"recipe_key": key, "cache_hit": hit,
                          "manifest_sha256": validation["manifest_sha256"],
                          "files": validation["manifest_outputs"], "bytes": validation["total_bytes"]}
    return pack / "legends3ds/tutorial"


def package_sections(path: Path) -> dict[int, bytes]:
    # Used after the native-format validator, and independently verifies bounds/CRC
    # for the byte-identity contract used to bind the existing tutorial sidecars.
    data = path.read_bytes()
    if len(data) < 20:
        raise ValueError("truncated boot package")
    magic, version, count, total, _profile, table_crc = struct.unpack_from("<IHHIII", data)
    table_end = 20 + count * 16
    if magic != 0x324C4D4D or version != 2 or count > 64 or total != len(data) or table_end > total:
        raise ValueError("invalid boot package header")
    if zlib.crc32(data[20:table_end]) != table_crc:
        raise ValueError("invalid boot package directory CRC")
    sections = {}
    ranges = []
    for index in range(count):
        kind, offset, size, crc = struct.unpack_from("<IIII", data, 20 + index * 16)
        if kind in sections or offset < table_end or offset > total or size > total - offset:
            raise ValueError("invalid boot package section")
        if any(offset < end and begin < offset + size for begin, end in ranges):
            raise ValueError("overlapping boot package sections")
        payload = data[offset:offset + size]
        if zlib.crc32(payload) != crc:
            raise ValueError("invalid boot package section CRC")
        sections[kind] = payload
        ranges.append((offset, offset + size))
    return sections


def verify_boot_pair(boot: Path, pack: Path) -> dict:
    sections = package_sections(boot)
    for kind, name in ((2, "st00.l3map"), (3, "st00.l3render")):
        if sections.get(kind) != (pack / name).read_bytes():
            raise ValueError("generalized boot map/render differ from tutorial sidecars; binding refused")
    negative_sidecar = pack / "st00.l3nm"
    negative_present = negative_sidecar.is_file()
    if 32 in sections or negative_present:
        if not negative_present or sections.get(32) != negative_sidecar.read_bytes():
            raise ValueError("boot negative materials differ from tutorial sidecar; binding refused")
    return {"source": "generalized importer", "bytes": boot.stat().st_size,
            "sha256": digest_file(boot), "sidecar_map_render_byte_identical": True,
            "sidecar_negative_materials_byte_identical": negative_present,
            "native_bindings": ["map", "render"] + (["negative_materials"] if negative_present else []),
            "unsupported_bindings": ["generalized objects", "events", "camera", "audio", "progression"]}


def deploy_sd(pack: Path, boot: Path, binary: Path | None, out: Path) -> dict:
    stage = out / ".sd_out-staging"
    if any(path.exists() or path.is_symlink() for path in (stage, out / "sd_out", out / "sd-manifest.json")):
        raise ValueError("SD output already exists; prior data is preserved")
    target = stage / "3ds/Legends3DS"
    target.mkdir(parents=True)
    destination = target / "data/legends3ds/tutorial"
    shutil.copytree(pack, destination)
    if validate_pack(destination) != validate_pack(pack):
        raise RuntimeError("copied tutorial pack differs from verified source")
    area = target / "data/legends3ds/areas/boot.mmlpkg"
    area.parent.mkdir(parents=True)
    shutil.copyfile(boot, area)
    boot_report = verify_boot_pair(area, destination)
    if digest_file(area) != digest_file(boot):
        raise RuntimeError("copied boot package differs from verified source")
    if binary is not None:
        shutil.copyfile(binary, target / "Legends3DS.3dsx")
        if digest_file(target / "Legends3DS.3dsx") != digest_file(binary):
            raise RuntimeError("copied native binary differs from compiled source")
    files = [{"path": path.relative_to(stage).as_posix(), "bytes": path.stat().st_size,
              "sha256": digest_file(path)} for path in sorted(stage.rglob("*")) if path.is_file()]
    write_json(out / "sd-manifest.json", {"local_game_data": True, "redistributable": False, "files": files})
    _publish_exclusive(stage, out / "sd_out")
    return {"root": str(out / "sd_out"), "files": len(files),
            "bytes": sum(entry["bytes"] for entry in files), "boot": boot_report}


def install(owner: Path, out: Path, cache: Path, *, host_only: bool = False, tutorial_only: bool = False) -> dict:
    owner, out, cache = validate_locations(owner, out, cache)
    out.mkdir(parents=True, exist_ok=False)
    logs = out / "logs"
    logs.mkdir()
    start = time.monotonic()
    result = {"format": "mml-local-install/1", "created_utc": datetime.now(timezone.utc).isoformat(),
              "status": "BUILDING", "source": str(ROOT), "input": str(owner), "cache": str(cache),
              "local_game_data": True, "redistributable": False, "ownership_verified": False,
              "hardware": "HARDWARE_PENDING", "PHYSICS_1TO1": False, "commands": []}
    env = os.environ.copy()
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    try:
        with cache_lock(cache):
            print("Checking source and supported local game-data profile", flush=True)
            run([sys.executable, ROOT / "tools/verify_0146_source.py"], logs / "source-verification.json", result, env)
            run([sys.executable, ROOT / "tools/p2/verify_cumulative_tree.py", ROOT], logs / "cumulative-verification.json", result, env)
            source, intake = prepare_owner_input(owner, out / "owner-input")
            write_json(out / "input-validation.json", intake)
            result["intake"] = intake
            host = cache / "host" / sha(str(ROOT).encode())[:16]
            print("Building host tools and running inherited checks", flush=True)
            run(["cmake", "-S", ROOT, "-B", host, "-DCMAKE_BUILD_TYPE=Release"], logs / "host-configure.log", result, env)
            run(["cmake", "--build", host, "--parallel", "4"], logs / "host-build.log", result, env)
            run(["ctest", "--test-dir", host, "--output-on-failure"], logs / "host-tests.log", result, env)
            result["host_tests"] = "PASS"
            print("Cooking or reusing neutral local game-data packages", flush=True)
            names = CORE_CONTAINERS if tutorial_only else None
            summary = build(source / "CDDATA/DAT", host, out / "content", cache_dir=cache / "tasks", container_names=names)
            result["content"] = summary
            if summary["packages_with_errors"]:
                raise RuntimeError("generalized cooker reported unresolved package errors; see content manifests")
            packages = sorted((out / "content/areas").glob("*.mmlpkg"))
            for index, package in enumerate(packages):
                run([host / "l3area_validate", package], logs / f"area-{index:03d}-validation.log", result, env)
            result["validated_packages"] = len(packages)
            pack = obtain_tutorial(source, host, cache, logs, result, env)
            boot = out / "content/areas/ST00.mmlpkg"
            result["boot_package"] = verify_boot_pair(boot, pack)
            run([host / "l3area_runtime_probe", boot], logs / "area-runtime-probe.json", result, env)
            run([host / "l3area_placement_probe", boot], logs / "positive-placement-probe.json", result, env)
            result["positive_placement_probe"] = "PASS"
            run([host / "l3area_inspection_probe", boot], logs / "area-inspection-probe.json", result, env)
            result["area_inspection_probe"] = "PASS"
            for name, args in (("l3player_runtime_probe", [pack.parents[1]]),
                               ("l3runtime_session_probe", [pack]),
                               ("l3flight_render_probe", [pack]), ("l3flight_material_probe", [pack]),
                               ("l3flight_animation_probe", [pack / "player/main.l3anim", "0x0c"])):
                run([host / name, *args], logs / (name + ".log"), result, env)
            run([sys.executable, ROOT / "tools/mml_replay_suite.py", host, pack, out / "replays"], logs / "replays.log", result, env)
            replay_result = json.loads((out / "replays/summary.json").read_text())
            result["repeatable_native_replays"] = sum(row.get("repeatable", False) for row in replay_result["scenarios"])
            result["retail_parity_tested"] = False
            run([host / "l3animation_evidence", pack / "player/main.l3anim"], logs / "animation-evidence.json", result, env)
            binary = None
            devkit = Path(env.get("DEVKITARM", "/opt/devkitpro/devkitARM")).resolve()
            if host_only:
                result["native_build"] = "NOT_REQUESTED"
            elif not (devkit / "3ds_rules").is_file():
                result["native_build"] = "TOOLCHAIN_UNAVAILABLE"
            else:
                print("Compiling the actual Old 3DS target", flush=True)
                env["DEVKITARM"] = str(devkit)
                env.setdefault("DEVKITPRO", str(devkit.parent))
                native = out / "native-source"
                write_json(out / "native-source-manifest.json", native_snapshot(native))
                run(["make", "-C", native / "platform/3ds", "-j4"], logs / "native-build.log", result, env)
                binaries = list((native / "platform/3ds").glob("*.3dsx"))
                if len(binaries) != 1:
                    raise RuntimeError("native build did not produce exactly one 3dsx")
                binary = out / "build/Legends3DS.3dsx"
                binary.parent.mkdir()
                shutil.copyfile(binaries[0], binary)
                result.update(native_build="PASS", native_binary_sha256=digest_file(binary),
                              native_binary_bytes=binary.stat().st_size,
                              native_binary_path=str(binary))
            result["sd"] = deploy_sd(pack, boot, binary, out)
            result["status"] = "PASS" if binary else "DATA_READY"
    except Exception as error:
        result.update(status="FAILED", error=str(error))
        raise
    finally:
        result["seconds"] = round(time.monotonic() - start, 6)
        write_json(out / "build-result.json", result)
    print(json.dumps({key: result[key] for key in ("status", "content", "tutorial", "native_build", "sd", "hardware")}, indent=2))
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("owner", type=Path, help="supported extracted game folder, CDDATA/DAT, or extraction ZIP")
    parser.add_argument("--out", required=True, type=Path, help="new local output directory; existing checkpoints are preserved")
    parser.add_argument("--cache", type=Path, help="separate reusable local cooker/build cache")
    parser.add_argument("--host-only", action="store_true", help="validate/cook/install local data without compiling 3DS code")
    parser.add_argument("--tutorial-only", action="store_true", help="cook the eleven currently supported tutorial/player containers")
    args = parser.parse_args(argv)
    cache = args.cache or args.out.absolute().parent / "mml3ds-local-cache"
    try:
        install(args.owner, args.out, cache, host_only=args.host_only, tutorial_only=args.tutorial_only)
        return 0
    except (ValueError, OSError, RuntimeError) as error:
        print(f"Local installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

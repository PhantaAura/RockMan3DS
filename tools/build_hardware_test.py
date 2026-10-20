#!/usr/bin/env python3
"""Build the hardware test and prepare a LOCAL SD tree from an existing cooked pack.

NO COOKING OR EXTRACTION: accepts only a complete, manifest-verified L3 runtime
pack. Never accepts owner data, invokes cookers/extractors, or makes an asset
archive. The SD tree contains the supplied local assets; do not redistribute it.
All build products and logs go into a fresh --out directory outside the source.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import struct
import zlib
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PACK_PREFIX = PurePosixPath("legends3ds/tutorial")
SD_DATA_PATH = PurePosixPath("3ds/Legends3DS/data/legends3ds/tutorial")
COOKED_SUFFIXES = {".l3tr", ".l3rw", ".l3tg", ".l3pl", ".l3f2", ".l3sr", ".l3ls", ".l3a4", ".l3r7", ".l3sv", ".l3sm", ".l3p6", ".l3p5", ".l3fd", ".l3xa", ".l3eq", ".l3ec", ".l3rg", ".l3c6", ".l3ep", ".l3pr", ".l3anim", ".l3det", ".l3dp", ".l3nr", ".l3db", ".l3cb", ".l3ebd", ".l3flow", ".l3init", ".l3map", ".l3mc", ".l3bp",
                   ".l3model", ".l3msg", ".l3nm", ".l3pal", ".l3pc", ".l3ref",
                   ".l3render", ".l3skel", ".l3spawn", ".l3tex"}
SOURCE_SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".pica"}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_json(path: Path, value) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def validate_pack(pack: Path) -> dict:
    """Validate the existing format-1 manifest, inventory, sizes and hashes."""
    if not pack.is_dir():
        raise ValueError("--pack must name the existing cooked tutorial directory")
    inventory = set()
    for path in pack.rglob("*"):
        if path.is_symlink():
            raise ValueError(f"pack symlinks are not supported: {path}")
        if path.is_file():
            inventory.add(path.relative_to(pack).as_posix())
        elif not path.is_dir():
            raise ValueError(f"pack contains a non-regular entry: {path}")
    manifest_path = pack / "manifest.json"
    manifest = json.loads(manifest_path.read_text())
    if not isinstance(manifest, dict) or type(manifest.get("format")) is not int or manifest["format"] != 1:
        raise ValueError("expected cooked pack manifest format 1")
    outputs = manifest.get("outputs")
    if not isinstance(outputs, list) or not outputs:
        raise ValueError("manifest outputs must be a nonempty list")
    expected = {"manifest.json"}
    checked = []
    for entry in outputs:
        if not isinstance(entry, dict):
            raise ValueError("manifest output must be an object")
        name, size, digest = entry.get("path"), entry.get("bytes"), entry.get("sha256")
        if not isinstance(name, str) or "\\" in name:
            raise ValueError("manifest output path must be a POSIX path")
        rel = PurePosixPath(name)
        if rel.is_absolute() or ".." in rel.parts or rel.as_posix() != name or rel.parts[:2] != PACK_PREFIX.parts:
            raise ValueError(f"manifest output escapes the cooked tutorial path: {name}")
        rel = rel.relative_to(PACK_PREFIX)
        if rel.suffix not in COOKED_SUFFIXES:
            raise ValueError(f"manifest output is not a supported cooked runtime file: {name}")
        if rel.as_posix() in expected:
            raise ValueError(f"duplicate manifest output: {name}")
        if type(size) is not int or size < 0 or not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
            raise ValueError(f"invalid size or SHA-256 in manifest output: {name}")
        expected.add(rel.as_posix())
        path = pack / rel
        if not path.is_file() or path.stat().st_size != size or sha256(path) != digest:
            raise ValueError(f"manifest size/SHA-256 mismatch: {name}")
        checked.append({"path": rel.as_posix(), "bytes": size, "sha256": digest})
    if inventory != expected:
        raise ValueError(f"pack inventory differs from manifest: missing={sorted(expected-inventory)}, extra={sorted(inventory-expected)}")
    required = {"st00.l3map", "st00.l3render", "st00.l3flow", "reference.l3ref",
                "messages/st00_01.l3msg", "messages/st00_02.l3msg", "player/player.l3pc",
                "player/main.l3anim", "player/main.l3skel", "player/buster.l3bp"}
    required.update(f"player/{part}.l3model" for part in ("body", "head", "right", "left", "feet"))
    for area in range(5):
        required.update((f"ebd/area_{area:02d}.l3ebd", f"materials/area_{area:02d}.l3mc",
                         f"materials/player_area_{area:02d}.l3mc"))
    if not required <= expected:
        raise ValueError(f"pack lacks frontend files: {sorted(required-expected)}")
    actor_materials=manifest.get("actor_materials")
    if actor_materials is not None:
        if (not isinstance(actor_materials,dict) or type(actor_materials.get("format")) is not int
                or actor_materials["format"]!=1 or actor_materials.get("optional") is not True
                or actor_materials.get("default_enabled") is not False
                or actor_materials.get("runtime_gate")!="ACTOR_TEXTURES=1"
                or actor_materials.get("shade")!="UNKNOWN"):
            raise ValueError("actor material manifest must retain the optional default-OFF shade-UNKNOWN gate")
        actor_paths={f"materials/actor_area_{area:02d}.l3mc" for area in range(5)}
        if not actor_paths<=expected:
            raise ValueError("actor material manifest lacks complete area cache inventory")
    _validate_scene_metadata(pack, manifest, expected)
    return {"status": "PASS", "manifest_sha256": sha256(manifest_path),
            "manifest_outputs": len(checked), "total_bytes": sum(item["bytes"] for item in checked),
            "files": checked, "scope": "Existing cooked files only; hash integrity is not retail accuracy verification."}



def _validate_scene_metadata(pack: Path, manifest: dict, inventory: set[str]) -> None:
    """Optional source-scene provenance, bounded envelopes and lifecycle gates.

    Current-frame palette/camera truth is still established by runtime owner
    bindings. Hash integrity and initial snapshots do not prove retail parity.
    Legacy packs lacking these optional files/metadata remain accepted.
    """
    lift_name = "source_prop_lift.l3pl"
    lift = manifest.get("source_prop_lift")
    if lift_name in inventory or lift is not None:
        if (not isinstance(lift, dict) or type(lift.get("format")) is not int
                or lift["format"] != 1 or lift.get("optional") is not True
                or lift.get("default_enabled") is not False
                or lift.get("runtime_gate") != "SOURCE_EVENT_BOOT=1"
                or lift.get("status") != "SOURCE_BACKED_PARTIAL"
                or lift.get("path") != "legends3ds/tutorial/" + lift_name
                or lift_name not in inventory):
            raise ValueError("source prop lift profile must retain optional owner admission")
        from cook_source_prop_lift import validate_package as validate_lift
        validate_lift((pack / lift_name).read_bytes())
    effect_name = "source_effect_dispatch.l3f2"
    effect = manifest.get("source_effect_dispatch")
    if effect_name in inventory or effect is not None:
        if (not isinstance(effect, dict) or type(effect.get("format")) is not int
                or effect["format"] != 1 or effect.get("optional") is not True
                or effect.get("default_enabled") is not False
                or effect.get("runtime_gate") != "SOURCE_EVENT_BOOT=1"
                or effect.get("status") != "SOURCE_BACKED_PARTIAL"
                or effect.get("path") != "legends3ds/tutorial/" + effect_name
                or effect_name not in inventory):
            raise ValueError("source effect profile must retain explicit optional owner admission")
        from cook_source_effect_dispatch import validate_package as validate_effect
        validate_effect((pack / effect_name).read_bytes())
    view_name="source_scene_view.l3sv"
    view=manifest.get("source_scene_view")
    if view_name in inventory or view is not None:
        expected_path="legends3ds/tutorial/"+view_name
        if (not isinstance(view,dict) or type(view.get("format")) is not int or view["format"]!=1
                or view.get("optional") is not True or view.get("default_enabled") is not False
                or view.get("runtime_gate")!="SOURCE_EVENT_BOOT=1" or view.get("status")!="SOURCE_BACKED_PARTIAL"
                or view.get("projection_lifecycle")!="EXPLICIT_FRESH_EXECUTABLE" or view.get("path")!=expected_path
                or view_name not in inventory):
            raise ValueError("scene view manifest must retain optional default-OFF fresh projection ownership")
        from cook_actor_combat import OWNER_SHA
        if view.get("owner_exe_sha256")!=OWNER_SHA:
            raise ValueError("scene view manifest EXE identity rejected")
        data=(pack/view_name).read_bytes()
        if len(data)!=2126 or struct.unpack_from("<4sHH6I",data)!=(
                b"L3SV",1,64,1,0x80092348,1025,2126,zlib.crc32(data[64:])&0xffffffff,0):
            raise ValueError("scene view bounded header/CRC rejected")
        if data[32:64]!=bytes.fromhex(OWNER_SHA) or struct.unpack_from("<3I",data,64)!=(160<<16,120<<16,384):
            raise ValueError("scene view projection/source identity rejected")
        table=struct.unpack_from("<1025h",data,76)
        if table[0]!=0 or table[-1]!=4096 or any(v<0 or v>4096 for v in table):
            raise ValueError("scene view scalar table domain rejected")
    tape_name="source_scene_rng.l3sr"
    tape=manifest.get("source_scene_rng")
    if tape_name in inventory or tape is not None:
        if (not isinstance(tape,dict) or tape.get("format")!=1 or tape.get("optional") is not True
                or tape.get("default_enabled") is not False or tape.get("runtime_gate")!="SOURCE_SCENE_RNG_REPLAY=1"
                or tape.get("scope")!="RECORDED_OPENBIOS_SCENE" or tape.get("retail_chronology") is not False
                or tape.get("path")!="legends3ds/tutorial/"+tape_name or tape_name not in inventory
                or "source_event_phase5.l3p5" not in inventory):
            raise ValueError("scene RNG tape must retain bounded optional default-OFF replay ownership")
        from cook_source_scene_rng_tape import validate_package
        data=(pack/tape_name).read_bytes();validate_package(data)
        phase5=(pack/"source_event_phase5.l3p5").read_bytes()
        if len(phase5)<96 or data[96:128]!=phase5[64:96]:
            raise ValueError("scene RNG tape EBD identity rejected")
    loop_name="source_loop_snapshot.l3ls"
    loop=manifest.get("source_loop_snapshot")
    if loop_name in inventory or loop is not None:
        if (not isinstance(loop,dict) or loop.get("format")!=1 or loop.get("optional") is not True
                or loop.get("default_enabled") is not False or loop.get("runtime_gate")!="SOURCE_LOOP_REPLAY=1"
                or loop.get("status")!="MATCHES_MEASURED_FIELDS" or loop.get("scope")!="RECORDED_OPENBIOS_CONSTRUCTOR"
                or loop.get("retail_chronology") is not False or loop.get("path")!="legends3ds/tutorial/"+loop_name
                or loop_name not in inventory or "source_event_phase5.l3p5" not in inventory):
            raise ValueError("loop snapshot metadata must retain default-OFF recorded replay scope")
        from cook_source_loop_snapshot import validate_package
        snapshot=(pack/loop_name).read_bytes();validate_package(snapshot)
        phase5=(pack/"source_event_phase5.l3p5").read_bytes()
        if len(phase5)<96 or snapshot[96:128]!=phase5[64:96]:
            raise ValueError("loop snapshot EBD identity rejected")
    for scene_name, key in (("materials/scene_e0_initial.l3sm", "scene_materials"),
                            ("materials/scene_actor40_initial.l3sm", "scene_actor40_materials")):
        scene=manifest.get(key)
        if scene_name in inventory or scene is not None:
            if (not isinstance(scene,dict) or type(scene.get("format")) is not int or scene["format"]!=1
                    or scene.get("optional") is not True or scene.get("default_enabled") is not False
                    or scene.get("runtime_gate")!="SOURCE_SCENE_TEXTURES=1"
                    or scene.get("snapshot")!="INITIAL_UPLOAD_BATCH" or scene.get("chronology")!="SOURCE_BACKED_PARTIAL"
                    or scene.get("live_palette")!="UNKNOWN" or scene.get("path")!="legends3ds/tutorial/"+scene_name
                    or scene_name not in inventory or "source_event_phase5.l3p5" not in inventory):
                raise ValueError("scene material manifest must retain optional default-OFF initial snapshot and UNKNOWN live palette")
            uploads=scene.get("uploads")
            expected_uploads=("INIT_DAT.BIN","HEAD01.BIN","ARM00R.BIN","ARM00L.BIN","LEG00.BIN","ST00.BIN","ST00_00.BIN")
            if (not isinstance(uploads,list) or len(uploads)!=len(expected_uploads)
                    or any(not isinstance(row,dict) or row.get("name")!=name
                        or not isinstance(row.get("sha256"),str) or not re.fullmatch(r"[0-9a-f]{64}",row["sha256"])
                        for row,name in zip(uploads,expected_uploads))):
                raise ValueError("scene material initial upload order/provenance rejected")
            data=(pack/scene_name).read_bytes()
            if (len(data)<112 or len(data)>2*1024*1024+4096
                    or struct.unpack_from("<4s5I",data)!=(b"L3SM",0x00400001,1,len(data),64,zlib.crc32(data[64:])&0xffffffff)
                    or data[56:64]!=bytes(8) or data[24:56]==bytes(32)):
                raise ValueError("scene material bounded header/CRC rejected")
            phase5=(pack/"source_event_phase5.l3p5").read_bytes()
            if len(phase5)<96 or phase5[:4]!=b"L3P5" or data[24:56]!=phase5[64:96]:
                raise ValueError("scene material initial snapshot EBD identity rejected")
            # Validate the nested bounded cache envelope too. Exact E0 resource
            # sets, each CLUT and decoded page semantics are rechecked by C++ loader.
            payload=data[64:]
            fields=struct.unpack_from("<4sHH4I4H4I",payload)
            magic,version,header,flags,count,resolved,unresolved,width,height,record,reserved,records,offset,total,digest=fields
            if (magic!=b"L3MC" or version!=1 or header!=48 or flags!=3
                    or not 1<=count<=45 or count!=resolved+unresolved
                    or (width,height,record,reserved)!=(256,256,40,0)
                    or records!=48 or offset!=48+count*40 or offset>len(payload)
                    or total!=len(payload) or digest!=zlib.crc32(payload[48:])&0xffffffff
                    or resolved*65536+count*40>2*1024*1024):
                raise ValueError("scene material bounded cache envelope rejected")

def run(command, log: Path, commands: list, env: dict) -> None:
    argv = [str(arg) for arg in command]
    print(f"Running {log.name}", flush=True)
    with log.open("w") as stream:
        stream.write(shlex.join(argv) + "\n")
        stream.flush()
        process = subprocess.run(argv, stdout=stream, stderr=subprocess.STDOUT, env=env)
    commands.append({"argv": argv, "returncode": process.returncode, "log": str(log)})
    if process.returncode:
        raise RuntimeError(f"command failed ({process.returncode}); see {log}")


def existing_area_package_command(pack: Path, area_package: Path, validation: dict) -> list:
    """Wrap only cooked optional sidecars present in the verified manifest."""
    verified = {entry["path"] for entry in validation["files"]}
    command = [sys.executable, ROOT / "tools/package_cooked_area.py",
        "--map", pack / "st00.l3map", "--render", pack / "st00.l3render",
        "--state-id", "local:verified-tutorial-pair", "--out", area_package]
    if "st00.l3nm" in verified:
        command += ["--negative-materials", pack / "st00.l3nm"]
    if "st00.l3spawn" in verified:
        command += ["--spawns", pack / "st00.l3spawn"]
    if "st00.l3init" in verified:
        command += ["--map-initialization", pack / "st00.l3init"]
    command += ["--runtime-flow", pack / "st00.l3flow"]
    for area in range(5):
        command += ["--runtime-ebd", f"{area}:{area}:{pack / f'ebd/area_{area:02d}.l3ebd'}"]
    return command


def native_snapshot(destination: Path) -> list:
    """Copy only buildable source/header/shader text, never asset directories."""
    files = []
    for relative in ("include", "src", "platform/3ds/source"):
        for path in sorted((ROOT / relative).rglob("*")):
            if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                if path.is_symlink():
                    raise ValueError(f"source snapshot cannot include symlink: {path}")
                files.append(path)
    files.append(ROOT / "platform/3ds/Makefile")
    hashes = []
    for source in files:
        relative = source.relative_to(ROOT)
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        hashes.append({"path": relative.as_posix(), "sha256": sha256(target)})
    return hashes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pack", required=True, type=Path, help="existing cooked tutorial directory containing manifest.json")
    parser.add_argument("--out", required=True, type=Path, help="fresh or empty output directory outside source and pack")
    parser.add_argument("--host-build", type=Path, help="optional external host build cache; reconfigured, rebuilt and fully tested")
    args = parser.parse_args()
    pack, out = args.pack.resolve(), args.out.resolve()
    if out == ROOT or ROOT in out.parents or out in ROOT.parents or out == pack or pack in out.parents or out in pack.parents:
        raise ValueError("output must be separate from source and the supplied pack")
    if out.exists() and (not out.is_dir() or any(out.iterdir())):
        raise ValueError("output already exists and is nonempty; choose a fresh --out")
    expected_root = '"sdmc:/' + SD_DATA_PATH.as_posix() + '/"'
    if expected_root not in (ROOT / "platform/3ds/source/main.cpp").read_text():
        raise ValueError("frontend data root changed; review the SD layout before building")
    validation = validate_pack(pack)
    host = args.host_build.resolve() if args.host_build else out / "host"
    if host == ROOT or ROOT in host.parents or host in ROOT.parents or host == pack or pack in host.parents or host in pack.parents:
        raise ValueError("host build cache must be separate from source and cooked pack")
    cache=host / "CMakeCache.txt"
    if cache.is_file() and f"CMAKE_HOME_DIRECTORY:INTERNAL={ROOT}" not in cache.read_text().splitlines():
        raise ValueError("host build cache belongs to another source tree")
    env = os.environ.copy()
    devkit = Path(env.get("DEVKITARM", "/opt/devkitpro/devkitARM")).resolve()
    if not (devkit / "3ds_rules").is_file():
        raise ValueError("devkitARM toolchain unavailable; set DEVKITARM")
    env["DEVKITARM"] = str(devkit)
    env.setdefault("DEVKITPRO", str(devkit.parent))
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    out.mkdir(parents=True, exist_ok=True)
    logs = out / "logs"
    logs.mkdir()
    write_json(out / "pack-validation.json", validation)
    result = {"created_utc": datetime.now(timezone.utc).isoformat(), "status": "BUILDING",
              "source": str(ROOT), "pack": str(pack), "pack_manifest_sha256": validation["manifest_sha256"],
              "cookers_or_extractors_invoked": False, "hardware": "PENDING", "commands": []}
    try:
        result["host_build_cache"] = str(host)
        run(["cmake", "-S", ROOT, "-B", host, "-DCMAKE_BUILD_TYPE=Release"], logs / "host-configure.log", result["commands"], env)
        run(["cmake", "--build", host, "--parallel", "4"], logs / "host-build.log", result["commands"], env)
        run(["ctest", "--test-dir", host, "--output-on-failure"], logs / "host-tests.log", result["commands"], env)
        result["host_tests"] = "PASS"
        # Wrap existing verified cooked data; this does not decode/extract retail
        # files or recook payloads. Keep the resulting package LOCAL ONLY.
        local_data = out / "local-data"
        local_data.mkdir()
        area_package = local_data / "boot.mmlpkg"
        # Compilation may be lengthy; reject a pack changed since initial hash
        # validation before copying any optional source-backed cooked payload.
        if validate_pack(pack) != validation:
            raise ValueError("cooked pack changed after initial manifest validation")
        package_command = existing_area_package_command(pack,area_package,validation)
        run(package_command,
            logs / "area-package.log", result["commands"], env)
        run([host / "l3area_runtime_probe", area_package], logs / "area-runtime-probe.log", result["commands"], env)
        run([host / "l3area_inspection_probe", area_package], logs / "area-inspection-probe.log", result["commands"], env)
        result["area_package"] = {"path": str(area_package), "sha256": sha256(area_package),
                                  "bytes": area_package.stat().st_size,
                                  "scope": "Existing cooked map/render, optional negative materials/spawns and explicit ST00_FLOW_V1 actor bindings; local only. Wider flow backends remain unsupported."}
        result["packaging_existing_cooked_payloads"] = True
        native = out / "native-source"
        write_json(out / "native-source-manifest.json", native_snapshot(native))
        run(["make", "-C", native / "platform/3ds", "-j4"], logs / "native-build.log", result["commands"], env)
        binaries = list((native / "platform/3ds").glob("*.3dsx"))
        if len(binaries) != 1:
            raise RuntimeError("native build did not produce exactly one 3dsx")
        build = out / "build"
        build.mkdir()
        binary = build / binaries[0].name
        shutil.copyfile(binaries[0], binary)
        sd = out / "sd_out/3ds/Legends3DS"
        sd.mkdir(parents=True)
        shutil.copyfile(binary, sd / binary.name)
        destination = out / "sd_out" / SD_DATA_PATH
        shutil.copytree(pack, destination)
        if validate_pack(destination) != validation:
            raise RuntimeError("copied pack differs from validated input")
        package_destination = out / "sd_out/3ds/Legends3DS/data/legends3ds/areas/boot.mmlpkg"
        package_destination.parent.mkdir(parents=True)
        shutil.copyfile(area_package, package_destination)
        if sha256(package_destination) != sha256(area_package):
            raise RuntimeError("copied area package differs from validated input")
        result.update(status="PASS", native_build="PASS", native_binary_sha256=sha256(binary),
                      native_binary_bytes=binary.stat().st_size,
                      native_binary_path=binary.relative_to(out).as_posix(),
                      sd_binary_path=(sd / binary.name).relative_to(out).as_posix(),
                      sd_root=str(out / "sd_out"))
    except Exception as exc:
        result.update(status="FAILED", error=str(exc))
        raise
    finally:
        write_json(out / "build-result.json", result)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError) as error:
        print(f"Hardware test build failed: {error}", file=sys.stderr)
        sys.exit(1)

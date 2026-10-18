#!/usr/bin/env python3
"""Wrap explicitly supplied cooked files in an MML2 area package.

This tool does not extract or decode retail files, invoke cookers, or scan for
inputs. It copies existing L3MP/L3RG, optional L3NM and explicitly mapped
L3SF-v1/L3EB-v2 runtime bindings and optional L3SP-v1 spawns into the v2 envelope.
Cooked payloads remain local game data and do not belong in a source release.
The native area loader must still validate cooked records before admission.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import stat
import struct
import sys
import zlib

SOURCE_ROOT = Path(__file__).resolve().parents[1]
PROFILE = "explicit-cooked-area-v1"
MAX_INPUT_BYTES = 8 * 1024 * 1024
MAX_PACKAGE_BYTES = 16 * 1024 * 1024
STATE_ID = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.:-]{0,127}\Z")
RUNTIME_BINDINGS_SECTION = 33
SPAWNS_SECTION = 34
MAP_INITIALIZATION_SECTION = 35
RUNTIME_BACKEND_ST00_FLOW_V1 = 1


def _path(value: str | os.PathLike[str]) -> Path:
    """Reject ambiguous user paths before any input is opened."""
    raw = os.fspath(value)
    if not raw or "\x00" in raw or "//" in raw:
        raise ValueError("empty or aliased path")
    components = raw.split("/")
    if any(part in (".", "..") for part in components):
        raise ValueError("path aliases ('.' or '..') are not allowed")
    if any(part.casefold() == "owner-data" for part in components):
        raise ValueError("owner-data paths are outside this cooked-only tool")
    path = Path(raw)
    if not path.is_absolute():
        path = Path.cwd().resolve() / path
    return path


def _open_parent(path: Path) -> int:
    """Walk absolute parent components without following symbolic links."""
    flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
    fd = os.open(path.anchor, flags)
    try:
        for component in path.parent.parts[1:]:
            next_fd = os.open(component, flags, dir_fd=fd)
            os.close(fd)
            fd = next_fd
        return fd
    except BaseException:
        os.close(fd)
        raise


def _read_cooked(path: Path, suffix: str, magic: bytes, version: int,
                 header_bytes: int, file_offset: int, crc_offset: int,
                 max_bytes: int, *, area_count_header: bool = False) -> tuple[bytes, tuple[int, int]]:
    if path.suffix != suffix:
        raise ValueError(f"expected explicit {suffix} input")
    parent_fd = _open_parent(path)
    try:
        # Nonblocking open lets us reject FIFOs/devices with fstat instead of
        # hanging before their non-regular type can be checked.
        fd = os.open(path.name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK,
                     dir_fd=parent_fd)
    finally:
        os.close(parent_fd)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode):
            raise ValueError("input must be a regular cooked file")
        if info.st_size < header_bytes or info.st_size > max_bytes:
            raise ValueError("input exceeds cooked-file size bounds")
        with os.fdopen(fd, "rb") as stream:
            fd = -1
            data = stream.read(max_bytes + 1)
        if len(data) != info.st_size or len(data) > max_bytes:
            raise ValueError("input changed size while being read")
    finally:
        if fd >= 0:
            os.close(fd)
    if data[:4] != magic:
        raise ValueError(f"input is not {magic.decode('ascii')}")
    actual_version, header_field = struct.unpack_from("<HH", data, 4)
    if actual_version != version or (not area_count_header and header_field != header_bytes):
        raise ValueError("unsupported cooked version/header")
    if area_count_header and (not 1 <= header_field <= 256 or
                              len(data) != header_bytes + header_field * 32 * 8):
        raise ValueError("invalid L3NM area count or byte length")
    if struct.unpack_from("<I", data, file_offset)[0] != len(data):
        raise ValueError("cooked declared size mismatch")
    if struct.unpack_from("<I", data, crc_offset)[0] != zlib.crc32(data[header_bytes:]):
        raise ValueError("cooked payload CRC mismatch")
    if area_count_header:
        for offset in range(header_bytes, len(data), 8):
            word, tile, known, reserved = struct.unpack_from("<IBBH", data, offset)
            if known > 1 or reserved:
                raise ValueError("L3NM unknown flags or reserved field")
            if (not known and (word or tile)) or (known and (tile >= 64 or word & 0xffff != 0x1d)):
                raise ValueError("invalid L3NM material binding")
    return data, (info.st_dev, info.st_ino)


def _encoded(value: dict) -> bytes:
    return (json.dumps(value, sort_keys=True, separators=(",", ":"),
                       allow_nan=False) + "\n").encode("utf-8")


def _validate_flow(data: bytes) -> None:
    flags, areas, objects, off_areas, off_objects, off_blobs = struct.unpack_from("<6I", data, 8)
    if flags != 31 or areas != 5 or off_areas != 40 or off_objects != 40 + 5 * 48:
        raise ValueError("unsupported runtime L3SF backend tables")
    object_cursor = aux_cursor = 0
    for index in range(5):
        row = struct.unpack_from("<12I", data, off_areas + index * 48) if len(data) >= off_objects else None
        if (row is None or row[0] != index or row[1] != object_cursor or row[9] != aux_cursor or row[11] or
                object_cursor + row[2] > objects):
            raise ValueError("runtime L3SF area coverage invalid")
        object_cursor += row[2]
        aux_cursor += row[10]
    if (object_cursor != objects or off_blobs != off_objects + objects * 20 + aux_cursor * 24 or
            off_blobs + 5 * 40 != len(data)):
        raise ValueError("runtime L3SF payload ranges invalid")


def _validate_ebd(data: bytes) -> None:
    flags, entries, base, off_entries = struct.unpack_from("<4I", data, 8)
    off_source, source_bytes = struct.unpack_from("<2I", data, 32)
    if (flags != 7 or off_entries != 40 or off_source != 40 + entries * 16 or source_bytes < 4 or
            off_source + source_bytes != len(data) or base + source_bytes > 1 << 32 or
            4 + entries * 16 > source_bytes):
        raise ValueError("runtime L3EB v2 source payload range invalid")
    if struct.unpack_from("<I", data, off_source)[0] != entries:
        raise ValueError("runtime L3EB source directory count mismatch")
    for index in range(entries):
        row = off_entries + index * 16
        raw = off_source + 4 + index * 16
        if data[row:row + 4] != data[raw:raw + 4]:
            raise ValueError("runtime L3EB source directory key mismatch")
        for slot in range(3):
            pointer = struct.unpack_from("<I", data, raw + 4 + slot * 4)[0]
            if pointer and not base <= pointer < base + source_bytes:
                raise ValueError("runtime L3EB resource pointer outside source payload")
            normalized = pointer - base if pointer else 0xffffffff
            if struct.unpack_from("<I", data, row + 4 + slot * 4)[0] != normalized:
                raise ValueError("runtime L3EB source resource offset mismatch")


def _runtime_envelope(flow: bytes, bindings: list[tuple[int, int, bytes]]) -> bytes:
    flow_offset = 40 + len(bindings) * 20
    if flow_offset + len(flow) + sum(len(entry[2]) for entry in bindings) > MAX_INPUT_BYTES:
        raise ValueError("runtime bindings exceed native default section admission cap")
    cursor = flow_offset + len(flow)
    rows = bytearray()
    payload = bytearray(flow)
    for local, source, ebd in bindings:
        rows += struct.pack("<5I", local, source, cursor, len(ebd), zlib.crc32(ebd))
        payload += ebd
        cursor += len(ebd)
    body = rows + payload
    if cursor > MAX_INPUT_BYTES:
        raise ValueError("runtime bindings exceed native default section admission cap")
    return struct.pack("<4sHH8I", b"L3RB", 1, 40, RUNTIME_BACKEND_ST00_FLOW_V1, 0,
                       len(bindings), 40, flow_offset, len(flow), cursor, zlib.crc32(body)) + body


def _validate_spawns(data: bytes, map_areas: int) -> None:
    areas, regions, records, off_areas, off_regions, off_records, file_bytes = struct.unpack_from("<7I",data,8)
    if (areas != 5 or map_areas != 5 or regions > 64 or records > 256 or off_areas != 40 or
            off_regions != 40 + areas * 8 or off_records != off_regions + regions * 16 or
            file_bytes != off_records + records * 24 or file_bytes != len(data)):
        raise ValueError("ST00 spawn catalog requires five local map areas and canonical bounded layout")
    cursor=0
    for area in range(areas):
        first,count=struct.unpack_from("<2I",data,off_areas+area*8)
        if first != cursor or cursor+count > regions:
            raise ValueError("ST00 spawn catalog area coverage invalid")
        cursor+=count
    if cursor != regions:
        raise ValueError("ST00 spawn catalog incomplete area coverage")
    cursor=0
    for region in range(regions):
        selector,qualifier,_,_,_,_,first,count=struct.unpack_from("<hhbbBBII",data,off_regions+region*16)
        if not 0 <= selector <= 127 or not -128 <= qualifier <= 127 or first != cursor or cursor+count > records:
            raise ValueError("ST00 spawn catalog selector or object coverage invalid")
        cursor+=count
    if cursor != records:
        raise ValueError("ST00 spawn catalog incomplete object coverage")
    for record in range(records):
        offset=off_records+record*24
        if data[offset] & 0x80 or struct.unpack_from("<I",data,offset+20)[0] >= 2048:
            raise ValueError("ST00 spawn catalog conditional object or predicate invalid")


def _validate_map_initialization(data: bytes, map_data: bytes, render_data: bytes) -> None:
    profile, idx, descriptors, records, count, map_crc, render_crc, offset, size, _ = struct.unpack_from("<10I", data, 8)
    if (profile != 1 or idx > 2048 or count > idx or descriptors > 65535 or records > 65535
            or offset != 48 or size != 48 + count * 16
            or map_crc != zlib.crc32(map_data) or render_crc != zlib.crc32(render_data)
            or idx != struct.unpack_from("<I", map_data, 28)[0]
            or descriptors != struct.unpack_from("<I", map_data, 32)[0]
            or descriptors != struct.unpack_from("<I", render_data, 12)[0]
            or records != struct.unpack_from("<I", render_data, 16)[0]):
        raise ValueError("map initialization profile/count/CRC binding mismatch")
    idx_offset = struct.unpack_from("<I", map_data, 56)[0]
    desc_offset, record_offset = struct.unpack_from("<2I", render_data, 36)
    if (idx_offset < 84 or idx_offset + idx * 12 > len(map_data)
            or desc_offset < 80 or desc_offset + descriptors * 12 > len(render_data)
            or record_offset < 80 or record_offset + records * 24 > len(render_data)):
        raise ValueError("map initialization bound tables exceed cooked inputs")
    previous = -1
    for i in range(count):
        identity, descriptor, record, expected, seed, *reserved = struct.unpack_from("<8H", data, 48 + i * 16)
        if (identity <= previous or identity >= idx or descriptor >= descriptors or record >= records
                or any(reserved)):
            raise ValueError("map initialization index/order/reserved field invalid")
        previous = identity
        raw6, bound_descriptor = struct.unpack_from("<2H", map_data, idx_offset + identity * 12 + 6)
        bound_record = struct.unpack_from("<H", render_data, desc_offset + descriptor * 12)[0]
        r = record_offset + record * 24
        bound_seed = render_data[r + 7] | (render_data[r + 8] << 8)
        if expected != raw6 or descriptor != bound_descriptor or record != bound_record or seed != bound_seed:
            raise ValueError("map initialization entry disagrees with bound map/render")


def _envelope(sections: list[tuple[int, bytes]]) -> bytes:
    offset = 20 + 16 * len(sections)
    table = bytearray()
    payload = bytearray()
    for kind, data in sections:
        table += struct.pack("<IIII", kind, offset, len(data), zlib.crc32(data))
        payload += data
        offset += len(data)
    return (struct.pack("<IHHIII", 0x324C4D4D, 2, len(sections), offset,
                        zlib.crc32(PROFILE.encode("ascii")), zlib.crc32(table))
            + table + payload)


def _publish_exclusive(path: Path, data: bytes) -> None:
    """Publish complete bytes atomically; never replace an existing path."""
    parent_fd = _open_parent(path)
    temporary = None
    owns_temporary = False
    fd = -1
    try:
        # The parent is already proven link-free. Exclusive publication uses
        # its open directory descriptor even if another process renames it.
        for _ in range(100):
            temporary = "." + path.name + "." + secrets.token_hex(8)
            try:
                fd = os.open(temporary, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                             os.O_NOFOLLOW, 0o600, dir_fd=parent_fd)
                owns_temporary = True
                break
            except FileExistsError:
                continue
        if fd < 0:
            raise OSError("unable to allocate package output temporary")
        with os.fdopen(fd, "wb") as stream:
            fd = -1
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.link(temporary, path.name, src_dir_fd=parent_fd,
                dst_dir_fd=parent_fd, follow_symlinks=False)
    finally:
        if fd >= 0:
            os.close(fd)
        if owns_temporary:
            try:
                os.unlink(temporary, dir_fd=parent_fd)
            except FileNotFoundError:
                pass
        os.close(parent_fd)


def package_cooked_area(map_path: str | os.PathLike[str],
                        render_path: str | os.PathLike[str], state_id: str,
                        out_path: str | os.PathLike[str],
                        negative_materials_path: str | os.PathLike[str] | None = None,
                        runtime_flow_path: str | os.PathLike[str] | None = None,
                        runtime_ebd_paths: list[tuple[int, int, str | os.PathLike[str]]] | None = None,
                        spawns_path: str | os.PathLike[str] | None = None,
                        map_initialization_path: str | os.PathLike[str] | None = None) -> dict:
    """Validate/copy explicit cooked inputs and return neutral build metadata.

    Output parent must exist. Input bytes and package bytes are bounded by
    MAX_INPUT_BYTES and MAX_PACKAGE_BYTES, matching native default admission
    caps; these limits are policy, not measurements of Old 3DS performance.
    """
    if not isinstance(state_id, str) or STATE_ID.fullmatch(state_id) is None:
        raise ValueError("state-id must be 1-128 plain identifier characters")
    map_path, render_path, out_path = map(_path, (map_path, render_path, out_path))
    negative_path = _path(negative_materials_path) if negative_materials_path is not None else None
    flow_path = _path(runtime_flow_path) if runtime_flow_path is not None else None
    spawn_path = _path(spawns_path) if spawns_path is not None else None
    init_path = _path(map_initialization_path) if map_initialization_path is not None else None
    runtime_paths = []
    for entry in runtime_ebd_paths or []:
        if (not isinstance(entry, (tuple, list)) or len(entry) != 3 or
                type(entry[0]) is not int or type(entry[1]) is not int):
            raise ValueError("runtime EBD binding must be (local-area, source-area, path)")
        runtime_paths.append((entry[0], entry[1], _path(entry[2])))
    if (flow_path is None) != (not runtime_paths):
        raise ValueError("runtime flow and at least one explicit EBD binding must be supplied together")
    if out_path.suffix != ".mmlpkg":
        raise ValueError("output must have .mmlpkg suffix")
    if out_path == SOURCE_ROOT or SOURCE_ROOT in out_path.parents:
        raise ValueError("game payload output is forbidden inside the source tree")
    paths = [map_path, render_path, out_path] + ([negative_path] if negative_path is not None else [])
    paths += ([flow_path] if flow_path is not None else []) + [entry[2] for entry in runtime_paths]
    paths += ([spawn_path] if spawn_path is not None else [])
    paths += ([init_path] if init_path is not None else [])
    if len(set(paths)) != len(paths):
        raise ValueError("input/output path aliases are not allowed")
    # Reject pre-existing output before reading inputs; publication is also
    # exclusive so a concurrent file creation cannot be overwritten.
    parent_fd = _open_parent(out_path)
    try:
        try:
            os.stat(out_path.name, dir_fd=parent_fd, follow_symlinks=False)
        except FileNotFoundError:
            pass
        else:
            raise ValueError("output already exists; choose a new checkpoint path")
    finally:
        os.close(parent_fd)
    map_data, map_identity = _read_cooked(map_path, ".l3map", b"L3MP", 1,
                                         84, 76, 80, MAX_INPUT_BYTES)
    render_data, render_identity = _read_cooked(render_path, ".l3render", b"L3RG", 3,
                                               80, 60, 64, MAX_INPUT_BYTES)
    if map_identity == render_identity:
        raise ValueError("cooked inputs alias the same file")
    identities={map_identity,render_identity}
    negative_data = None
    if negative_path is not None:
        negative_data, negative_identity = _read_cooked(negative_path, ".l3nm", b"L3NM", 1,
            16, 8, 12, min(MAX_INPUT_BYTES, 16 + 256 * 32 * 8), area_count_header=True)
        if negative_identity in (map_identity, render_identity):
            raise ValueError("cooked inputs alias the same file")
        identities.add(negative_identity)
        if struct.unpack_from("<H", negative_data, 6)[0] != struct.unpack_from("<I", map_data, 12)[0]:
            raise ValueError("negative material table/map area count mismatch")
    spawn_data=None
    if spawn_path is not None:
        spawn_data,spawn_identity=_read_cooked(spawn_path,".l3spawn",b"L3SP",1,40,32,36,
            min(MAX_INPUT_BYTES,40+5*8+64*16+256*24))
        _validate_spawns(spawn_data,struct.unpack_from("<I",map_data,12)[0])
        if spawn_identity in identities:
            raise ValueError("cooked inputs alias the same file")
        identities.add(spawn_identity)
    initialization_data = None
    if init_path is not None:
        initialization_data, init_identity = _read_cooked(init_path, ".l3init", b"L3MI", 1,
            48, 40, 44, 48 + 2048 * 16)
        _validate_map_initialization(initialization_data, map_data, render_data)
        if init_identity in identities:
            raise ValueError("cooked inputs alias the same file")
        identities.add(init_identity)
    runtime_data = None
    runtime_inputs = None
    if flow_path is not None:
        area_count = struct.unpack_from("<I", map_data, 12)[0]
        if not 1 <= area_count <= 256 or len(runtime_paths) > area_count:
            raise ValueError("runtime binding/map area count invalid")
        locals_seen = set()
        for local, source, _ in runtime_paths:
            if not 0 <= local < area_count or not 0 <= source < 5 or local in locals_seen:
                raise ValueError("runtime binding area index invalid or duplicate")
            if spawn_data is not None and local != source:
                raise ValueError("ST00 spawn catalog requires identity runtime area selectors")
            locals_seen.add(local)
        flow_data, flow_identity = _read_cooked(flow_path, ".l3flow", b"L3SF", 1, 40, 32, 36, MAX_INPUT_BYTES)
        _validate_flow(flow_data)
        if flow_identity in identities:
            raise ValueError("cooked inputs alias the same file")
        identities.add(flow_identity)
        bindings = []
        runtime_size = 40 + 20 * len(runtime_paths) + len(flow_data)
        runtime_inputs = {"backend": "ST00_FLOW_V1", "flow": {"format": "L3SF", "bytes": len(flow_data),
            "sha256": hashlib.sha256(flow_data).hexdigest()}, "areas": []}
        for local, source, path in sorted(runtime_paths):
            ebd_data, identity = _read_cooked(path, ".l3ebd", b"L3EB", 2, 40, 24, 28, MAX_INPUT_BYTES)
            runtime_size += len(ebd_data)
            if runtime_size > MAX_INPUT_BYTES:
                raise ValueError("runtime bindings exceed native default section admission cap")
            _validate_ebd(ebd_data)
            if identity in identities:
                raise ValueError("cooked inputs alias the same file")
            identities.add(identity)
            bindings.append((local, source, ebd_data))
            runtime_inputs["areas"].append({"local_area": local, "source_flow_area": source,
                "format": "L3EB", "bytes": len(ebd_data), "sha256": hashlib.sha256(ebd_data).hexdigest()})
        runtime_data = _runtime_envelope(flow_data, bindings)
    metadata = {
        "schema": "explicit-cooked-area-v1",
        "state_id": state_id,
        "source_profile": PROFILE,
        "status": "SOURCE_BACKED_PARTIAL",
        "evidence": "Existing cooked bytes copied with format, size and CRC validation; no new decoding or retail parity claim.",
        "components": {
            "collision": {"section_kind": 2, "status": "SOURCE_BACKED_PARTIAL"},
            "geometry": {"section_kind": 3, "status": "SOURCE_BACKED_PARTIAL"},
        },
        "inputs": {
            "map": {"format": "L3MP", "bytes": len(map_data),
                    "sha256": hashlib.sha256(map_data).hexdigest()},
            "render": {"format": "L3RG", "bytes": len(render_data),
                       "sha256": hashlib.sha256(render_data).hexdigest()},
        },
        "unknown": ["terrain_topology", "material_state", "gameplay_semantics", "retail_parity"],
        "local_game_data": True,
        "source_release_payload": False,
    }
    sections = [(2, map_data), (3, render_data)]
    if negative_data is not None:
        metadata["components"]["negative_materials"] = {"section_kind": 32, "status": "SOURCE_BACKED_PARTIAL"}
        metadata["inputs"]["negative_materials"] = {"format": "L3NM", "bytes": len(negative_data),
            "sha256": hashlib.sha256(negative_data).hexdigest()}
        sections.append((32, negative_data))
    if runtime_data is not None:
        metadata["components"]["runtime_bindings"] = {"section_kind": RUNTIME_BINDINGS_SECTION,
            "backend": "ST00_FLOW_V1", "status": "SOURCE_BACKED_PARTIAL"}
        metadata["inputs"]["runtime_bindings"] = runtime_inputs
        sections.append((RUNTIME_BINDINGS_SECTION, runtime_data))
    if initialization_data is not None:
        metadata["components"]["map_initialization"] = {"section_kind": MAP_INITIALIZATION_SECTION,
            "profile": "STAGE0_STARTUP", "status": "SOURCE_BACKED_PARTIAL"}
        metadata["inputs"]["map_initialization"] = {"format": "L3MI", "bytes": len(initialization_data),
            "sha256": hashlib.sha256(initialization_data).hexdigest()}
        sections.append((MAP_INITIALIZATION_SECTION, initialization_data))
    if spawn_data is not None:
        metadata["components"]["spawns"]={"section_kind":SPAWNS_SECTION,"backend":"ST00_IDENTITY_V1",
            "status":"SOURCE_BACKED_PARTIAL"}
        metadata["inputs"]["spawns"]={"format":"L3SP","bytes":len(spawn_data),
            "sha256":hashlib.sha256(spawn_data).hexdigest()}
        sections.append((SPAWNS_SECTION,spawn_data))
    data = _envelope([(1, _encoded(metadata))] + sections)
    if len(data) > MAX_PACKAGE_BYTES:
        raise ValueError("package exceeds native default file admission cap")
    _publish_exclusive(out_path, data)
    return {"output": str(out_path), "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(), "metadata": metadata}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", required=True, dest="map_path")
    parser.add_argument("--render", required=True, dest="render_path")
    parser.add_argument("--state-id", required=True)
    parser.add_argument("--out", required=True, dest="out_path")
    parser.add_argument("--negative-materials", dest="negative_materials_path",
                        help="optional existing area-local L3NM table; must match map area count")
    parser.add_argument("--runtime-flow", dest="runtime_flow_path",
                        help="optional existing L3SF v1 flow; requires explicit --runtime-ebd bindings")
    parser.add_argument("--runtime-ebd", action="append", default=[], metavar="LOCAL:SOURCE:FILE",
                        help="existing L3EB v2, mapped from source flow area 0..4 to a local map area")
    parser.add_argument("--map-initialization", dest="map_initialization_path",
                        help="optional CRC-bound L3MI v1 stage-0 startup initializer")
    parser.add_argument("--spawns",dest="spawns_path",
                        help="optional existing L3SP v1 source catalog; five identity-mapped ST00 areas only")
    args = parser.parse_args(argv)
    try:
        bindings = []
        for entry in args.runtime_ebd:
            parts = entry.split(":", 2)
            if len(parts) != 3 or not re.fullmatch(r"[0-9]+", parts[0]) or not re.fullmatch(r"[0-9]+", parts[1]):
                raise ValueError("runtime EBD must be LOCAL:SOURCE:FILE")
            bindings.append((int(parts[0]), int(parts[1]), parts[2]))
        result = package_cooked_area(args.map_path, args.render_path,
                                     args.state_id, args.out_path, args.negative_materials_path,
                                     args.runtime_flow_path, bindings,args.spawns_path,args.map_initialization_path)
    except (OSError, ValueError) as error:
        print("package_cooked_area: " + str(error), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

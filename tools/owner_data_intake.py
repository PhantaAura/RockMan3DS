#!/usr/bin/env python3
"""Local, bounded intake of the supported loose owner-data extraction or ZIP.

No game executable is run. The profile is compatibility metadata, not proof of
ownership, a license, or gameplay fidelity. Disc-image and split-ZIP extraction
remain separate work: a plain ISO reader would not preserve PS1 XA data.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import tempfile
import zipfile

PROJECT_ROOT = Path(__file__).resolve().parents[1]
PROFILE_PATH = PROJECT_ROOT / "state/OWNER_DATA_PROFILE_USA.json"
ROOT_FILES = frozenset(("ROCK_NEO.EXE", "SLUS_006.03", "SYSTEM.CNF"))
GAME_DIRECTORIES = ("DAT", "XA", "STR")
MAX_MEMBERS = 4096
MAX_ARCHIVE_BYTES = 2 << 30
MAX_ENTRY_BYTES = 256 << 20
MAX_SELECTED_BYTES = 1 << 30
CHUNK_BYTES = 1 << 20


class OwnerDataIntakeError(ValueError):
    """Unsupported or malformed input; the original input is never changed."""


def _without_symlinks(path: Path) -> Path:
    path = path.absolute()
    for part in (path, *path.parents):
        if part.is_symlink():
            raise OwnerDataIntakeError(f"Symlinks are unsupported: {part}")
    return path.resolve()


def _regular_file(path: Path) -> None:
    try:
        mode = path.lstat().st_mode
    except OSError as error:
        raise OwnerDataIntakeError(f"Required regular file is missing: {path}") from error
    if not stat.S_ISREG(mode):
        raise OwnerDataIntakeError(f"Only regular files are supported: {path}")


def normalize_owner_directory(path: Path | str) -> Path:
    """Resolve an extraction root, its CDDATA directory, or its DAT directory."""
    root = _without_symlinks(Path(path))
    if not root.is_dir():
        raise OwnerDataIntakeError(f"Owner-data directory does not exist: {root}")
    if root.name.upper() == "DAT":
        root = root.parent.parent
    elif root.name.upper() == "CDDATA":
        root = root.parent
    return root


def _canonical_file(name: str) -> bool:
    if name in ROOT_FILES:
        return True
    match = re.fullmatch(r"CDDATA/(DAT|XA|STR)/[A-Z0-9_]+\.(BIN|XA|STR)", name)
    return bool(match and {"DAT": "BIN", "XA": "XA", "STR": "STR"}[match[1]] == match[2])


def _load_profile() -> tuple[dict, str]:
    _regular_file(PROFILE_PATH)
    raw = PROFILE_PATH.read_bytes()
    if len(raw) > 2 << 20:
        raise OwnerDataIntakeError("Owner-data profile exceeds its metadata budget")
    try:
        profile = json.loads(raw)
        if profile["format_version"] != 1 or not isinstance(profile["profile_id"], str):
            raise ValueError("Unsupported profile format")
        files = profile["files"]
        if not isinstance(files, list) or not 3 <= len(files) <= MAX_MEMBERS:
            raise ValueError("Invalid profile file count")
        names = set()
        counts = {name: 0 for name in GAME_DIRECTORIES}
        total = 0
        for entry in files:
            name, size, digest = entry["path"], entry["size"], entry["sha256"]
            if not isinstance(name, str) or not _canonical_file(name) or name in names:
                raise ValueError("Noncanonical or duplicate profile path")
            if type(size) is not int or not 0 <= size <= MAX_ENTRY_BYTES:
                raise ValueError("Invalid profile size")
            if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
                raise ValueError("Invalid profile fingerprint")
            names.add(name)
            total += size
            if name.startswith("CDDATA/"):
                counts[name.split("/")[1]] += 1
        if not ROOT_FILES <= names or counts != profile["counts"] or total > MAX_SELECTED_BYTES:
            raise ValueError("Incomplete or oversized profile")
    except (KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        raise OwnerDataIntakeError(f"Invalid owner-data profile: {error}") from error
    return profile, hashlib.sha256(raw).hexdigest()


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(CHUNK_BYTES), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _validate_directory(root: Path, profile: dict) -> None:
    expected = {entry["path"] for entry in profile["files"]}
    actual = set(ROOT_FILES)
    for part in (root, root / "CDDATA", *(root / "CDDATA" / name for name in GAME_DIRECTORIES)):
        if part.is_symlink() or not part.is_dir():
            raise OwnerDataIntakeError(f"Required real directory is missing: {part}")
    for name in GAME_DIRECTORIES:
        for child in (root / "CDDATA" / name).iterdir():
            _regular_file(child)
            actual.add(child.relative_to(root).as_posix())
    if actual != expected:
        missing, extra = sorted(expected - actual), sorted(actual - expected)
        raise OwnerDataIntakeError(f"Owner-data inventory mismatch; missing={missing[:5]}, extra={extra[:5]}")
    for entry in profile["files"]:
        path = root / entry["path"]
        _regular_file(path)
        if path.stat().st_size != entry["size"]:
            raise OwnerDataIntakeError(f"Owner-data size mismatch: {entry['path']}")
        if _hash_file(path) != entry["sha256"]:
            raise OwnerDataIntakeError(f"Owner-data SHA-256 mismatch: {entry['path']}")


def _safe_zip_members(archive: zipfile.ZipFile) -> list[tuple[zipfile.ZipInfo, tuple[str, ...]]]:
    infos = archive.infolist()
    if len(infos) > MAX_MEMBERS:
        raise OwnerDataIntakeError("ZIP exceeds the member-count budget")
    result = []
    seen = set()
    for info in infos:
        name = info.orig_filename
        if "\x00" in name or "\\" in name or ":" in name or name.startswith("/"):
            raise OwnerDataIntakeError("ZIP contains an unsafe path")
        parts = tuple(name.rstrip("/").split("/"))
        if not parts or any(part in ("", ".", "..") for part in parts):
            raise OwnerDataIntakeError("ZIP contains an unsafe path")
        identity = "/".join(parts).casefold()
        if identity in seen:
            raise OwnerDataIntakeError("ZIP contains duplicate or case-colliding paths")
        seen.add(identity)
        if info.flag_bits & 1:
            raise OwnerDataIntakeError("Encrypted ZIP input is unsupported; provide a supported loose extraction")
        kind = stat.S_IFMT(info.external_attr >> 16)
        if kind not in (0, stat.S_IFREG, stat.S_IFDIR):
            raise OwnerDataIntakeError("ZIP links and nonregular entries are unsupported")
        if kind == stat.S_IFDIR and not info.is_dir():
            raise OwnerDataIntakeError("ZIP directory type and path disagree")
        if kind == stat.S_IFREG and info.is_dir():
            raise OwnerDataIntakeError("ZIP regular-file type and path disagree")
        result.append((info, parts))
    return result


def _select_zip_payload(archive: zipfile.ZipFile, profile: dict) -> list[tuple[zipfile.ZipInfo, str]]:
    members = _safe_zip_members(archive)
    prefixes = {parts[:-1] for info, parts in members if not info.is_dir() and parts[-1] == "ROCK_NEO.EXE"}
    if len(prefixes) != 1:
        raise OwnerDataIntakeError("ZIP must contain exactly one supported loose extraction root; nested/split disc archives are unsupported")
    prefix = next(iter(prefixes))
    expected = {entry["path"]: entry for entry in profile["files"]}
    selected = []
    found = set()
    for info, parts in members:
        if parts[:len(prefix)] != prefix:
            continue
        relative = "/".join(parts[len(prefix):])
        if info.is_dir():
            if relative.startswith(tuple(f"CDDATA/{name}/" for name in GAME_DIRECTORIES)):
                raise OwnerDataIntakeError("Nested canonical game directories are unsupported")
            continue
        in_game_directory = relative.startswith(tuple(f"CDDATA/{name}/" for name in GAME_DIRECTORIES))
        if relative not in expected:
            if in_game_directory:
                raise OwnerDataIntakeError(f"Unexpected file in canonical game directory: {relative}")
            continue
        if info.file_size != expected[relative]["size"] or info.file_size > MAX_ENTRY_BYTES:
            raise OwnerDataIntakeError(f"ZIP member size mismatch or budget exceeded: {relative}")
        selected.append((info, relative))
        found.add(relative)
    if found != set(expected):
        raise OwnerDataIntakeError(f"ZIP extraction is incomplete; missing={sorted(set(expected)-found)[:5]}")
    if sum(info.file_size for info, _ in selected) > MAX_SELECTED_BYTES:
        raise OwnerDataIntakeError("ZIP selected payload exceeds the extraction budget")
    return selected


def _disjoint_stage(stage: Path, source: Path) -> None:
    for protected in (source, PROJECT_ROOT):
        if stage == protected or stage in protected.parents or protected in stage.parents:
            raise OwnerDataIntakeError("Intake staging must be separate from source project and original owner input")
    if stage.exists():
        raise OwnerDataIntakeError(f"Intake staging already exists; choose a new path: {stage}")


def prepare_owner_input(path: Path | str, stage: Path | str) -> tuple[Path, dict]:
    """Validate a directory or unpack a supported ZIP into a new private stage.

    Directory input is read in place. ZIP input returns stage/extracted only
    after every profile fingerprint passes; failed staging is removed.
    """
    source = _without_symlinks(Path(path))
    staging = _without_symlinks(Path(stage))
    profile, profile_hash = _load_profile()
    report = {
        "format_version": 1, "profile_id": profile["profile_id"],
        "profile_sha256": profile_hash, "verified_file_count": len(profile["files"]),
        "verified_bytes": sum(entry["size"] for entry in profile["files"]),
        "counts": profile["counts"], "input_path": str(source),
        "compatibility": "MATCHES_MEASURED_FIELDS",
        "verification_scope": "All named profile files match byte identity; compatibility does not prove ownership, permission, or gameplay fidelity.",
        "local_only": True, "executables_executed": False,
    }
    if source.is_dir():
        root = normalize_owner_directory(source)
        _disjoint_stage(staging, root)
        _validate_directory(root, profile)
        report.update({"input_kind": "directory", "root": str(root), "archive_sha256": None})
        return root, report
    _regular_file(source)
    _disjoint_stage(staging, source)
    if source.suffix.lower() != ".zip":
        raise OwnerDataIntakeError("Disc images (CHD/BIN/CUE/ISO), encrypted inputs, and split archives are unsupported in this milestone; select a supported loose extraction directory or one complete extraction ZIP")
    if source.stat().st_size > MAX_ARCHIVE_BYTES:
        raise OwnerDataIntakeError("ZIP exceeds the compressed input-size budget")
    created_stage = False
    try:
        with zipfile.ZipFile(source) as archive:
            selected = _select_zip_payload(archive, profile)
            staging.parent.mkdir(parents=True, exist_ok=True)
            _without_symlinks(staging.parent)
            staging.mkdir(mode=0o700)
            created_stage = True
            pending = Path(tempfile.mkdtemp(prefix=".pending-", dir=staging))
            for info, relative in selected:
                destination = pending / PurePosixPath(relative)
                destination.parent.mkdir(parents=True, exist_ok=True)
                written = 0
                with archive.open(info) as incoming, destination.open("xb") as outgoing:
                    for chunk in iter(lambda: incoming.read(CHUNK_BYTES), b""):
                        written += len(chunk)
                        if written > info.file_size or written > MAX_ENTRY_BYTES:
                            raise OwnerDataIntakeError(f"ZIP expanded beyond declared size: {relative}")
                        outgoing.write(chunk)
                if written != info.file_size:
                    raise OwnerDataIntakeError(f"ZIP member is truncated: {relative}")
            _validate_directory(pending, profile)
            archive_hash = _hash_file(source)
            root = staging / "extracted"
            os.rename(pending, root)
            report.update({"input_kind": "zip", "root": str(root), "archive_sha256": archive_hash})
            return root, report
    except Exception as error:
        if created_stage:
            shutil.rmtree(staging)
        if isinstance(error, OwnerDataIntakeError):
            raise
        raise OwnerDataIntakeError(f"Could not read supported extraction ZIP: {error}") from error


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--stage", type=Path, required=True)
    args = parser.parse_args()
    try:
        _, report = prepare_owner_input(args.input, args.stage)
    except OwnerDataIntakeError as error:
        parser.exit(2, f"Owner-data intake failed: {error}\n")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

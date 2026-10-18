#!/usr/bin/env python3
"""Verify the exact user-owned Mega Man Legends USA reference extraction used by Legends3DS.

This tool stores only file sizes and SHA-256 fingerprints. It does not contain or redistribute
Capcom data. Pass the extraction root that contains ROCK_NEO.EXE and CDDATA/.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

REFERENCE = {
    "ROCK_NEO.EXE": (825344, "10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"),
    "SLUS_006.03": (305152, "469670a017e9301ea6c7cd75cc5cfc1b919b839a62f73e69a979f8dafbe80cac"),
    "SYSTEM.CNF": (69, "92e2ecb9236b3b19fc899e3b2e836cfc9ec5cf44c24f9962b31acc767458450f"),
    "CDDATA/DAT/ST00.BIN": (1024000, "3f46c6bac9261ee1def37bddd8a91377343e5ee1235391b2ed3a18f2cbcc8dc0"),
    "CDDATA/DAT/ST00_00.BIN": (161792, "11016acb5b13a0699b22e9e4ca9066fe58b4be80d053860bc68454316768a92d"),
    "CDDATA/DAT/ST00_01.BIN": (225280, "f7283379a49d028623fd129f938c6a318e3c1338a62d4d9b585198748fd24779"),
    "CDDATA/DAT/ST00_02.BIN": (221184, "1fb567b6f86c1654ca9ae153ceba9e62b4501794d607b1844086591772774421"),
    "CDDATA/DAT/ST00_03.BIN": (188416, "02c7bb8bb9f2dae67e10370cee23fdf748c17d741d121b479e61236aa7e36fa8"),
    "CDDATA/DAT/ST00_04.BIN": (333824, "da52dca72a566a6a1ad10e3925c341c89dafe79cc6c384487f60caaebb9bea20"),
    "CDDATA/DAT/INIT_DAT.BIN": (204800, "ab7126f396a060c27ab7fa94df1ab31281e81af7d140dc689142e95ce3712c22"),
    "CDDATA/XA/XA37S00E.XA": (114196480, "731d89deb93e13a1e0e01b8e5873c519cf1ba109a9a1fa9f9ef4ed60abb90758"),
    "CDDATA/XA/XA37S01E.XA": (121864192, "9e46043c045c1985b8fb2738b3f7680631b1bd9227bd79ada5ec1e35441c1ce9"),
    "CDDATA/STR/CAPCOM20.STR": (2260992, "7dde5cfa9b593772711844f6baae28be11505e6ac0f6bbc8f14ce37c7ffd83dd"),
}
EXPECTED_COUNTS = {"DAT": 205, "XA": 38, "STR": 1}
PROJECT_CHD_REFERENCE = (217298243, "573c1fa958db9a84a5f57b2894bf7f7b71e51f77df70dac1cd34a5a3b66d1f94")

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", type=Path, help="Extraction root containing ROCK_NEO.EXE and CDDATA/")
    ap.add_argument("--json", action="store_true", help="Print machine-readable JSON")
    ap.add_argument("--chd", type=Path, help="Optionally verify the exact project-reference CHD as well")
    a = ap.parse_args()
    root = a.root.resolve()
    result = {"supported": True, "root": str(root), "files": [], "counts": {}, "chd": None}
    for rel, (expected_size, expected_hash) in REFERENCE.items():
        p = root / rel
        entry = {"path": rel, "expected_size": expected_size, "expected_sha256": expected_hash}
        if not p.is_file():
            entry.update({"present": False, "ok": False})
            result["supported"] = False
        else:
            actual_size = p.stat().st_size
            actual_hash = sha256(p)
            ok = actual_size == expected_size and actual_hash == expected_hash
            entry.update({"present": True, "size": actual_size, "sha256": actual_hash, "ok": ok})
            result["supported"] = result["supported"] and ok
        result["files"].append(entry)
    for name, expected in EXPECTED_COUNTS.items():
        d = root / "CDDATA" / name
        actual = len([p for p in d.iterdir() if p.is_file()]) if d.is_dir() else -1
        ok = actual == expected
        result["counts"][name] = {"expected": expected, "actual": actual, "ok": ok}
        result["supported"] = result["supported"] and ok
    if a.chd is not None:
        chd = a.chd.resolve()
        expected_size, expected_hash = PROJECT_CHD_REFERENCE
        chd_entry = {
            "path": str(chd),
            "expected_size": expected_size,
            "expected_sha256": expected_hash,
        }
        if not chd.is_file():
            chd_entry.update({"present": False, "ok": False})
            result["supported"] = False
        else:
            actual_size = chd.stat().st_size
            actual_hash = sha256(chd)
            ok = actual_size == expected_size and actual_hash == expected_hash
            chd_entry.update({"present": True, "size": actual_size, "sha256": actual_hash, "ok": ok})
            result["supported"] = result["supported"] and ok
        result["chd"] = chd_entry
    if a.json:
        print(json.dumps(result, indent=2))
    else:
        status = "SUPPORTED" if result["supported"] else "UNSUPPORTED / MISMATCH"
        print(f"Mega Man Legends USA reference: {status}")
        for e in result["files"]:
            print(f"{'OK' if e['ok'] else 'NO'}  {e['path']}")
        for name, c in result["counts"].items():
            print(f"{'OK' if c['ok'] else 'NO'}  {name} files: {c['actual']} (expected {c['expected']})")
        if result["chd"] is not None:
            c = result["chd"]
            print(f"{'OK' if c['ok'] else 'NO'}  project CHD: {c.get('size', 'missing')} bytes")
    return 0 if result["supported"] else 2

if __name__ == "__main__":
    raise SystemExit(main())

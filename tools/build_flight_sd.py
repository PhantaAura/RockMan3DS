#!/usr/bin/env python3
"""Assemble an SD-card folder from a locally cooked Legends3DS tutorial pack.

This copies only data that the user has already cooked from their own game dump.
It never contains or downloads game data itself.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import shutil


def locate_tutorial(root: Path) -> Path:
    candidates = [root / "legends3ds" / "tutorial", root / "tutorial", root]
    for c in candidates:
        if (c / "st00.l3map").is_file() and (c / "player" / "main.l3anim").is_file():
            return c
    raise SystemExit("could not find a cooked tutorial directory (expected st00.l3map + player/main.l3anim)")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("cooked_pack", type=Path, help="output root from build_tutorial_pack.py")
    ap.add_argument("sd_root", type=Path, help="directory representing the root of the SD card")
    ap.add_argument("--3dsx", dest="three_dsx", type=Path, help="optional compiled FlightProbe .3dsx to copy")
    args = ap.parse_args()

    tutorial = locate_tutorial(args.cooked_pack.resolve())
    app = args.sd_root.resolve() / "3ds" / "Legends3DS"
    dst = app / "data" / "legends3ds" / "tutorial"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(tutorial, dst)
    if args.three_dsx:
        if not args.three_dsx.is_file():
            raise SystemExit(f"3dsx not found: {args.three_dsx}")
        app.mkdir(parents=True, exist_ok=True)
        shutil.copy2(args.three_dsx, app / args.three_dsx.name)

    files = sum(1 for p in dst.rglob("*") if p.is_file())
    size = sum(p.stat().st_size for p in dst.rglob("*") if p.is_file())
    print(f"flight SD layout ready: {app}")
    print(f"cooked tutorial files: {files} bytes: {size}")
    print("original ROM/CHD/BIN is not copied")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

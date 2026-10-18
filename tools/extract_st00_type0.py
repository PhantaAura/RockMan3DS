#!/usr/bin/env python3
"""Extract only required type-0 ST00 resources from a user-supplied ST00.BIN.
No game assets are distributed with this tool. Resource headers are sector-aligned;
type-0 payload bytes start one 2048-byte sector after the resource header.
"""
from __future__ import annotations
import argparse, hashlib, struct
from pathlib import Path
WANTED={
    r"..\MAP\DATA\ST00_MAP.IDX":"ST00_MAP.IDX",
    r"..\MAP\DATA\ST00_MAP.MDT":"ST00_MAP.MDT",
    r"..\SCR\DATA\ST00.STG":"ST00.STG",
    r"..\SCR\DATA\ST00.HED":"ST00.HED",
    r"..\PROGBIN\R3_ST00.BIN":"R3_ST00.BIN",
}
def path_at(data:bytes,off:int)->str|None:
    p=off+0x40
    if p>=len(data): return None
    z=data.find(b'\0',p,min(p+256,len(data)))
    if z<0:return None
    try:s=data[p:z].decode('ascii')
    except UnicodeDecodeError:return None
    return s if s.startswith('..\\') else None
def main():
    ap=argparse.ArgumentParser();ap.add_argument('st00_bin',type=Path);ap.add_argument('out_dir',type=Path);a=ap.parse_args();d=a.st00_bin.read_bytes();a.out_dir.mkdir(parents=True,exist_ok=True);found={}
    for off in range(0,len(d)-0x800+1,0x800):
        p=path_at(d,off)
        if p not in WANTED:continue
        typ,size=struct.unpack_from('<II',d,off)
        if typ!=0: raise SystemExit(f'{p}: expected type 0, got {typ}')
        start=off+0x800; end=start+size
        if end>len(d):raise SystemExit(f'{p}: payload out of bounds')
        blob=d[start:end];dst=a.out_dir/WANTED[p];dst.write_bytes(blob);found[p]=dst
        print(f'{dst.name}: {len(blob)} bytes sha256={hashlib.sha256(blob).hexdigest()}')
    missing=[p for p in WANTED if p not in found]
    if missing:raise SystemExit('missing resources: '+', '.join(missing))
if __name__=='__main__':main()

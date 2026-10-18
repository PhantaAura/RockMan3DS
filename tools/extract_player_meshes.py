#!/usr/bin/env python3
"""Find the five baseline Mega Man P-type meshes in a user-owned extracted CDDATA/DAT directory.
The tool understands the sector-aligned MML BIN resource header used by these containers.
It never downloads or redistributes game data.
"""
from __future__ import annotations
import argparse,hashlib,struct
from pathlib import Path
WANTED={
 "PL00B00P.PBD","PL00H00P.PBD","PL00R00P.PBD","PL00L00P.PBD","PL00F00P.PBD",
}
def iter_resources(path:Path):
    d=path.read_bytes()
    for off in range(0,max(0,len(d)-0x800+1),0x800):
        ps=off+0x40; z=d.find(b'\0',ps,min(ps+256,len(d)))
        if z<0: continue
        try:s=d[ps:z].decode('ascii')
        except UnicodeDecodeError: continue
        if not s.startswith('..\\'): continue
        typ,size=struct.unpack_from('<II',d,off); start=off+0x800; end=start+size
        if end<=len(d): yield s,typ,d[start:end]
def main():
    ap=argparse.ArgumentParser();ap.add_argument('dat_dir',type=Path);ap.add_argument('out_dir',type=Path);a=ap.parse_args();a.out_dir.mkdir(parents=True,exist_ok=True);found={}
    for fp in sorted(a.dat_dir.glob('*.BIN')):
        for internal,typ,blob in iter_resources(fp):
            name=internal.rsplit('\\',1)[-1]
            if name not in WANTED or name in found: continue
            if typ!=0: raise SystemExit(f'{fp.name}:{internal}: expected raw type 0, got {typ}')
            dst=a.out_dir/name;dst.write_bytes(blob);found[name]=(fp.name,len(blob),hashlib.sha256(blob).hexdigest());print(f'{name}: {len(blob)} bytes from {fp.name} sha256={found[name][2]}')
    miss=sorted(WANTED-found.keys())
    if miss: raise SystemExit('missing baseline player meshes: '+', '.join(miss))
if __name__=='__main__':main()

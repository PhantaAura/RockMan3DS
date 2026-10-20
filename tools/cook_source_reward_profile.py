#!/usr/bin/env python3
"""Cook owner-local enemy-reward pickup tables into neutral L3RW/1 data.

Sources (USA ROCK_NEO.EXE, read by 0x80046B34, 0x8004602C state owners and
0x80046A0C): size-roll tables 0x8008A5A8, spread 0x80097EFA, scale 0x80097EE0,
launch speed 0x80097F1C, launch lift 0x80097F24, pickup radius 0x80097EEC,
pickup bounds 0x8008A618 and award amounts 0x8008A678. The output stays local.
"""
import argparse,hashlib,struct,zlib
from pathlib import Path

SUPPORTED='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
# (address, byte count) in payload order; the loader mirrors this layout.
FIELDS=(
    (0x8008A5A8,16*16),   # 16 size-roll tables x 16 entries (bytes)
    (0x80097EFA,17*2),    # spread by count 0..16 (u16)
    (0x80097EE0,5*2),     # scale by size 0..4 (u16)
    (0x80097F1C,5*2),     # launch speed by size (s16)
    (0x80097F24,5*2),     # launch lift by size (s16)
    (0x80097EEC,2*4*2),   # pickup radius [kind][size] (u16)
    (0x8008A618,2*4*6*2), # bounds [kind][size] x0,x1,z0,z1,y0,y1 (s16)
    (0x8008A678,3*8*2),   # awards [selector][money 0..3, energy 0..3] (u16)
)

def cook(image:bytes)->bytes:
    base=struct.unpack_from('<I',image,24)[0]
    payload=bytearray()
    for address,size in FIELDS:
        offset=2048+address-base
        if offset<2048 or offset+size>len(image):raise ValueError('source address outside executable load image')
        payload+=image[offset:offset+size]
    header=bytearray(32);header[:4]=b'L3RW'
    struct.pack_into('<HHIII',header,4,1,32,32+len(payload),len(payload),zlib.crc32(payload)&0xffffffff)
    return bytes(header+payload)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--exe',required=True,type=Path);parser.add_argument('--out',required=True,type=Path);args=parser.parse_args()
    image=args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest()!=SUPPORTED:raise ValueError('unsupported owner executable')
    output=cook(image)
    if args.out.exists():raise FileExistsError('refusing to overwrite cooked reward profile')
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_bytes(output)
    print('schema=L3RW/1 bytes=%d'%len(output))

if __name__=='__main__':main()

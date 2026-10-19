#!/usr/bin/env python3
"""Cook the owner executable's player0 target window into neutral L3TG data.

L3TG/2 also carries the image bytes at 0x80097D02/0x80097D03 (GP+0x49E/49F,
the per-player target mode read by 0x80034EB8). No direct store to them was
found in the EXE or ST00 overlay, so using the image value is CANDIDATE.
"""
import argparse,hashlib,struct,zlib
from pathlib import Path

SUPPORTED='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
ADDRESS=0x8008994c
MODE_ADDRESS=0x80097d02

def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
    image=a.exe.read_bytes()
    if hashlib.sha256(image).hexdigest()!=SUPPORTED:raise ValueError('unsupported owner executable')
    base=struct.unpack_from('<I',image,24)[0];at=2048+ADDRESS-base
    payload=image[at:at+12]
    if len(payload)!=12:raise ValueError('target window outside executable load image')
    maximum_depth,minimum_x,maximum_x,minimum_y,maximum_y,reserved=struct.unpack('<6h',payload)
    if maximum_depth<=0 or minimum_x>=maximum_x or minimum_y>=maximum_y or reserved!=0:
        raise ValueError('source target window failed structural validation')
    mat=2048+MODE_ADDRESS-base
    payload=payload+image[mat:mat+2]+b'\0\0'
    header=bytearray(32);header[:4]=b'L3TG';struct.pack_into('<HHIII',header,4,2,32,48,16,zlib.crc32(payload)&0xffffffff)
    out=bytes(header)+payload
    if a.out.exists():raise FileExistsError('refusing to overwrite cooked target profile')
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_bytes(out)
    print('schema=L3TG/2 bytes=48 payload_crc32=%08x source_address=%08x'%(zlib.crc32(payload)&0xffffffff,ADDRESS))

if __name__=='__main__':main()

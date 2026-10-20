#!/usr/bin/env python3
"""Cook owner-local Buster selection, weapon-level and shot tables into neutral L3BP/2 data."""
import argparse,hashlib,struct,zlib
from pathlib import Path

SUPPORTED='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
ROWS=0x80089A84
POINTERS=0x80089EAC
COUNT=16
LEVELS=8
SIZE=0x80097DCC   # shot radius by player +0x4FC (0x80043E78 +98)
SPEED=0x80097DD4  # shot speed by player +0x4FE (0x800437F8)
IMPACT=0x8008A240 # impact effect 0x82 step timers (0x80044BA0)

def cook(image:bytes)->bytes:
    base=struct.unpack_from('<I',image,24)[0]
    def offset(address:int,size:int=1)->int:
        value=2048+address-base
        if value<2048 or value+size>len(image):raise ValueError('source address outside executable load image')
        return value
    payload=bytearray()
    for selection in range(COUNT):
        row=offset(ROWS+selection*12,12)
        subtype=image[row+7]
        pointer=struct.unpack_from('<I',image,offset(POINTERS+selection*4,4))[0]
        counter=struct.unpack_from('<h',image,offset(pointer+2,2))[0]
        payload+=struct.pack('<BBh',subtype,0,counter)
    # Per-weapon level rows reached through PTR_DAT_80089EAC: +0 damage
    # (0x800375D4), +4 range (0x80037634); the other two halfwords are kept raw.
    for selection in range(COUNT):
        pointer=struct.unpack_from('<I',image,offset(POINTERS+selection*4,4))[0]
        payload+=image[offset(pointer,LEVELS*8):offset(pointer,LEVELS*8)+LEVELS*8]
    payload+=image[offset(SIZE,LEVELS):offset(SIZE,LEVELS)+LEVELS]
    payload+=image[offset(SPEED,LEVELS):offset(SPEED,LEVELS)+LEVELS]
    payload+=image[offset(IMPACT,8):offset(IMPACT,8)+8]
    header=bytearray(32);header[:4]=b'L3BP'
    struct.pack_into('<HHIIII',header,4,2,32,32+len(payload),COUNT,len(payload),zlib.crc32(payload)&0xffffffff)
    return bytes(header+payload)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--exe',required=True,type=Path);parser.add_argument('--out',required=True,type=Path);args=parser.parse_args()
    image=args.exe.read_bytes();digest=hashlib.sha256(image).hexdigest()
    if digest!=SUPPORTED:raise ValueError('unsupported owner executable')
    output=cook(image)
    if args.out.exists():raise FileExistsError('refusing to overwrite cooked Buster profile')
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_bytes(output)
    print('schema=L3BP/2 selections=16 bytes=%d source_rows=%08x source_pointers=%08x'%(len(output),ROWS,POINTERS))

if __name__=='__main__':main()

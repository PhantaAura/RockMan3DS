#!/usr/bin/env python3
"""Cook only proved detection metadata from an exact, already supplied owner EXE.

Local derived data must remain outside source releases. This never overwrites an
input or earlier output. Only the class03-supported profile4 is currently proved.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

SOURCE=Path(__file__).resolve().parents[1]
OWNER_SHA='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
PROFILE_TABLE=0x800899AC

def cook(image):
    if hashlib.sha256(image).hexdigest()!=OWNER_SHA or image[:8]!=b'PS-X EXE':
        raise ValueError('Unsupported owner executable profile')
    base,size=struct.unpack_from('<II',image,0x18)
    address=PROFILE_TABLE+4*4
    off=0x800+address-base
    if address<base or off+4>min(len(image),0x800+size):
        raise ValueError('Detection profile outside executable segment')
    payload=bytes([4])+image[off:off+3]
    header=struct.pack('<4sHHIIIIII',b'L3DP',1,32,1,1,32,36,zlib.crc32(payload)&0xffffffff,0)
    return header+payload

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe',type=Path,required=True);ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--evidence',type=Path);args=ap.parse_args()
    out=args.out.resolve()
    if out.is_relative_to(SOURCE) or out==args.exe.resolve() or out.exists():
        raise ValueError('Choose a new owner-local output outside source and original inputs')
    if args.evidence and (args.evidence.exists() or args.evidence.resolve()==out):
        raise ValueError('Choose a separate new evidence output')
    package=cook(args.exe.read_bytes());out.parent.mkdir(parents=True,exist_ok=True)
    with out.open('xb') as f:f.write(package)
    result={'schema':'actor-detection-cook/1','status':'SOURCE_BACKED_PARTIAL','owner_sha256':OWNER_SHA,
            'source_profile_address':hex(PROFILE_TABLE+16),'profile_ids':[4],
            'output':str(out),'bytes':len(package),'sha256':hashlib.sha256(package).hexdigest(),
            'retail_values_embedded_in_source':False,'limits':['Original-owned phase1 helpers/scratch still separate.','No live retail parity or original HP/AI selector claim.','Cooked payload excluded from source releases.']}
    if args.evidence:
        args.evidence.parent.mkdir(parents=True,exist_ok=True)
        with args.evidence.open('x') as f:f.write(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()

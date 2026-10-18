#!/usr/bin/env python3
"""Cook exact owner-local scalar camera table and fresh projection controls.

No original instructions, models or textures are included. Owner-derived output
is private data and is excluded from every cumulative source checkpoint.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zlib
from cook_actor_combat import OwnerExe, OWNER_SHA

SOURCE=Path(__file__).resolve().parents[1]
TABLE=0x80092348

def cook(image):
    owner=OwnerExe(image)
    # Verify the exact argument-producing caller instructions and two JALs.
    for address,register,value in ((0x80011f48,4,160),(0x80011f50,5,120),(0x80011f58,4,384)):
        word=owner.word(address)
        if (word>>26,(word>>21)&31,(word>>16)&31,word&65535)!=(9,0,register,value):
            raise ValueError('Unsupported fresh projection caller arguments')
    for address,target in ((0x80011f4c,0x8007b118),(0x80011f54,0x8007b138)):
        word=owner.word(address)
        if word>>26!=3 or (((address+4)&0xf0000000)|((word&0x3ffffff)<<2))!=target:
            raise ValueError('Unsupported fresh projection caller target')
    table=owner.read(TABLE,2050)
    values=struct.unpack('<1025h',table)
    if values[0]!=0 or values[-1]!=4096 or any(v<0 or v>4096 for v in values):
        raise ValueError('Scalar quarter-wave camera table domain')
    payload=struct.pack('<3I',160<<16,120<<16,384)+table
    header=struct.pack('<4sHH6I',b'L3SV',1,64,1,TABLE,1025,
        64+len(payload),zlib.crc32(payload)&0xffffffff,0)+bytes.fromhex(OWNER_SHA)
    return header+payload

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    args=p.parse_args();out=args.out.resolve()
    if out.exists() or out==SOURCE or SOURCE in out.parents:
        raise ValueError('Choose a NEW owner-local output directory outside source')
    package=cook(args.exe.read_bytes());out.mkdir(parents=True,exist_ok=False)
    (out/'source_scene_view.l3sv').write_bytes(package)
    print('source_scene_view.l3sv bytes='+str(len(package))+' sha256='+hashlib.sha256(package).hexdigest())
if __name__=='__main__':main()

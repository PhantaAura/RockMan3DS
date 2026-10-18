#!/usr/bin/env python3
"""Cook non-pointer runtime reference tables from the user's exact USA ROCK_NEO.EXE.

The output is local derived runtime data and must not be committed/distributed with
source releases.  It currently preserves the 4096-entry fixed-point sin/cos table
used by the original movement matrix routines.
"""
from __future__ import annotations
import argparse, hashlib, struct, zlib
from pathlib import Path

EXPECTED_SHA256="10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
TRIG_VADDR=0x80092EC4
COUNT=4096
HEADER=32

def main():
    ap=argparse.ArgumentParser();ap.add_argument('rock_neo',type=Path);ap.add_argument('out_file',type=Path);a=ap.parse_args()
    b=a.rock_neo.read_bytes(); h=hashlib.sha256(b).hexdigest()
    if h!=EXPECTED_SHA256: raise SystemExit(f'unsupported ROCK_NEO.EXE sha256={h}')
    if len(b)<0x800 or b[:8]!=b'PS-X EXE': raise SystemExit('not PS-X EXE')
    text_addr,text_size=struct.unpack_from('<II',b,0x18)
    start=0x800+(TRIG_VADDR-text_addr); end=start+COUNT*4
    if TRIG_VADDR<text_addr or end>0x800+text_size or end>len(b): raise SystemExit('reference trig table out of executable range')
    trig=b[start:end]
    # Structural anchors from the exact libgte table: 0, +90, 180, 270 degrees.
    anchors=[struct.unpack_from('<hh',trig,i*4) for i in (0,0x400,0x800,0xC00)]
    if anchors!=[(0,4096),(4096,0),(0,-4096),(-4096,0)]: raise SystemExit(f'unexpected trig anchors: {anchors}')
    payload=trig; crc=zlib.crc32(payload)&0xffffffff
    out=bytearray();out+=b'L3RF';out+=struct.pack('<HHIIIIII',1,HEADER,1,COUNT,HEADER,HEADER+len(payload),crc,0);assert len(out)==HEADER;out+=payload
    a.out_file.parent.mkdir(parents=True,exist_ok=True);a.out_file.write_bytes(out)
    print(f'{a.out_file}: {len(out)} bytes trig={COUNT} source_sha256={h} payload_crc32={crc:08x}')
if __name__=='__main__':main()

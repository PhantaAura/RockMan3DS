#!/usr/bin/env python3
"""Fingerprint the user-owned extracted XA/STR sources without copying them.

This is identity/lookup metadata only. It deliberately does not guess message AUDIO-index
routing, XA channel semantics, or codec framing before those are recovered from retail code.
"""
from __future__ import annotations
import argparse,hashlib,json,struct,zlib
from pathlib import Path

def sha256(p:Path):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1<<20),b''):h.update(b)
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser();ap.add_argument('extracted_root',type=Path);ap.add_argument('out_dir',type=Path);a=ap.parse_args()
    root=a.extracted_root.resolve();out=a.out_dir.resolve();out.mkdir(parents=True,exist_ok=True)
    entries=[]
    for kind,sub,ext in [('xa','CDDATA/XA','*.XA'),('str','CDDATA/STR','*.STR')]:
        d=root/sub
        for p in sorted(d.glob(ext)):
            n=p.stat().st_size
            entries.append({'kind':kind,'name':p.name,'bytes':n,'sha256':sha256(p),'blocks_2048':n//2048 if n%2048==0 else None,'aligned_2048':n%2048==0})
    m={'format':1,'generated':True,'source':'user-owned Mega Man Legends USA extraction','routing_proven':False,'entries':entries}
    (out/'audio_reference_catalog.json').write_text(json.dumps(m,indent=2)+'\n')
    strings=bytearray(); packed=[]
    for e in entries:
        nb=e['name'].encode('utf-8'); no=len(strings); strings.extend(nb)
        sha=bytes.fromhex(e['sha256']); kind=1 if e['kind']=='xa' else 2
        blocks=0xFFFFFFFF if e['blocks_2048'] is None else e['blocks_2048']
        packed.append(struct.pack('<BBHII32sIII',kind,1 if e['aligned_2048'] else 0,0,e['bytes'],blocks,sha,no,len(nb),0))
    header_bytes=40; entry_bytes=56; off_entries=header_bytes; off_strings=off_entries+len(packed)*entry_bytes
    payload=b''.join(packed)+bytes(strings); file_bytes=header_bytes+len(payload); crc=zlib.crc32(payload)&0xFFFFFFFF
    header=struct.pack('<4sHHIIIIIIII',b'L3AC',1,header_bytes,0,len(packed),entry_bytes,off_entries,off_strings,len(strings),file_bytes,crc)
    (out/'audio_reference_catalog.l3ac').write_bytes(header+payload)
    print(f"xa={sum(x['kind']=='xa' for x in entries)} str={sum(x['kind']=='str' for x in entries)} aligned_2048={sum(x['aligned_2048'] for x in entries)}")
if __name__=='__main__':main()

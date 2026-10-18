#!/usr/bin/env python3
"""Retain original child UV LHU tables in a bounded private owner sidecar.
Never include the cooked owner-derived output in a code-only source release.
"""
import argparse,hashlib,json,struct,zlib
from pathlib import Path
from build_stage_catalog import iter_resources
OVERLAY_SHA='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
def cook(overlay):
    if hashlib.sha256(overlay).hexdigest()!=OVERLAY_SHA:
        raise ValueError('Unsupported ST00 source overlay')
    payload=overlay[0x1c81c:0x1c834]+overlay[0x1c834:0x1c84c]
    if len(payload)!=48: raise ValueError('Truncated source UV table')
    return struct.pack('<4sHH6I32s',b'L3C6',1,64,1,0,6,112,zlib.crc32(payload),8,bytes.fromhex(OVERLAY_SHA))+payload
def cook_from_dat(dat):
    overlay=next(r['payload'] for r in iter_resources(Path(dat)/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    return cook(overlay)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dat',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args()
    if a.out.exists():raise SystemExit('Refusing to overwrite owner metadata')
    overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    b=cook(overlay);a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_bytes(b)
    print(json.dumps({'schema':'class60-06-source-profile/1','bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'status':'SOURCE_BACKED_PARTIAL','owner_payload_release_permitted':False}))

#!/usr/bin/env python3
"""Owner-local ST00 reaction timer metadata; no source-embedded retail values."""
import argparse, hashlib, json, struct, sys, zlib
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(SOURCE/'docs/research_01415'))
from tutorial_actor_identity_probe import SUPPORTED_EXE,OVERLAY_SHA,iter_resources
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path);a=p.parse_args()
    if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output outside source')
    im=a.exe.read_bytes()
    if hashlib.sha256(im).hexdigest()!=SUPPORTED_EXE:raise ValueError('Unsupported owner executable')
    ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    if hashlib.sha256(ov).hexdigest()!=OVERLAY_SHA:raise ValueError('Unsupported ST00 overlay')
    payload=ov[0x1bee0:0x1bf04]
    package=struct.pack('<4sHH6I',b'L3R7',1,64,1,0,3,100,zlib.crc32(payload)&0xffffffff,12)+bytes.fromhex(OVERLAY_SHA)+payload
    a.out.mkdir(parents=True);(a.out/'actor_reactions.l3r7').write_bytes(package)
    report={'schema':'mml-reaction-profile/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':SUPPORTED_EXE,
            'overlay_sha256':OVERLAY_SHA,'tables':['0x8011BEE0','0x8011BEEC','0x8011BEF8'],
            'table_bytes':12,'package_sha256':hashlib.sha256(package).hexdigest(),'values_exported_in_report':False,
            'limits':['Owner-local tables only. No Buster input, live packet producer, defeat lifecycle or full combat claim.']}
    (a.out/'reaction-profile-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
if __name__=='__main__':main()

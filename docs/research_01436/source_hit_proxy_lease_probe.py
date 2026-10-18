#!/usr/bin/env python3
"""Unchanged original8003545C/354E0 on authored lease masks and identities.

Only authored scalar fixtures and aggregate proof are exported. No original
instructions, owner table, game payload or return-only helper hooks are used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

SOURCE=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(SOURCE/'docs/research_01414'))
from player_contact_source_probe import CPU,SUPPORTED_EXE

MASK,TABLE,TARGET=0x80098798,0x800be478,0x1000

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose NEW private evidence outside source')
    image=args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest()!=SUPPORTED_EXE:
        raise ValueError('Unsupported original executable')
    base=struct.unpack_from('<I',image,0x18)[0]
    random_source=random.Random(1436)
    masks=[0,255,0xffffffff,0xffffff00,0xfffffeff,0x800000ff]
    masks += [(0xffffff00^(0x80000000>>i))|low
              for i in range(24) for low in (0,1,127,255)]
    masks += [random_source.getrandbits(32) for _ in range(64)]
    rows=[]; instructions=0
    for mask in masks:
        for owner,flags in ((0x3000,0),(0x4000,0x80000080),(0,0x13579bdf)):
            cpu=CPU(image,base)
            cpu.r[4],cpu.r[5]=owner,TARGET
            cpu.put(MASK,4,mask);cpu.put(TARGET+4,4,flags)
            for i in range(24):cpu.put(TABLE+i*4,4,0x5000+i*4)
            cpu.run(0x8003545c)
            returned=cpu.r[2] if cpu.r[2]<0x80000000 else cpu.r[2]-0x100000000
            rows.append([0,mask,flags,owner,0,returned,cpu.get(MASK,4),cpu.get(TARGET+4,4)]
                +[cpu.get(TABLE+i*4,4) for i in range(24)])
            instructions+=cpu.steps
    for mask in (0,255,0xffffffff,0xa5a5f00d):
        for index in tuple(range(64))+(0xffffffff,):
            cpu=CPU(image,base);cpu.r[4]=index
            cpu.put(MASK,4,mask);cpu.put(TARGET+4,4,0x13579bdf)
            for i in range(24):cpu.put(TABLE+i*4,4,0x5000+i*4)
            cpu.run(0x800354e0)
            rows.append([1,mask,0x13579bdf,0,index,-1,cpu.get(MASK,4),cpu.get(TARGET+4,4)]
                +[cpu.get(TABLE+i*4,4) for i in range(24)])
            instructions+=cpu.steps
    args.out.mkdir(parents=True)
    fixture=('\n'.join(' '.join(map(str,row)) for row in rows)+'\n').encode()
    (args.out/'source_hit_proxy_leases_01436.tsv').write_bytes(fixture)
    proof={'schema':'mml-source-hit-proxy-leases-01436/1',
        'status':'SOURCE_BACKED_PARTIAL','exe_sha256':SUPPORTED_EXE,
        'allocation_entry':'0x8003545C','release_entry':'0x800354E0',
        'allocation_cases':len(masks)*3,'release_cases':260,
        'instructions_executed':instructions,'helper_hooks':0,
        'fixture_sha256':hashlib.sha256(fixture).hexdigest(),
        'retail_payload_exported':False,'retail_parity':False,
        'limits':['Authored masks/owner identities, not a live Buster trace.',
          'Lease producer alone does not establish muzzle, shot motion, HP or defeat.',
          'Release retains owner table/target flags and uses original low5 shift index.']}
    (args.out/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))

if __name__=='__main__':main()

#!/usr/bin/env python3
"""Original8002E7F4 cold reset and80035724 producer on authored owner fields.

No helper replacements, opcode patches, retail table/payload exports, or claim
that authored muzzle/subtype/counter premises establish actual live firing.
"""
import argparse, hashlib, itertools, json, struct, sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(SOURCE/'docs/research_01414'))
from player_contact_source_probe import CPU,SUPPORTED_EXE

POOL,MASK,TARGET,PLAYER=0x800bfb20,0x800989f0,0x3000,0x800b51b0
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
    a=p.parse_args()
    if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW private output')
    image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SUPPORTED_EXE
    base=struct.unpack_from('<I',image,24)[0];rows=[];steps=0
    cases=list(itertools.product((0,1,0x10000),(-1,0),(-128,-1,0,4,7,12,13,127),(-32768,-1,0,1,32767)))
    cases+=[(i&1,0,0,100) for i in range(18)]
    for case,(stack10,selected,row,counter) in enumerate(cases):
        c=CPU(image,base);c.r[29]=0x700000
        # Source cold-reset slice initializes this actual16-slot family.
        c.run(0x8002e7f4,0x8002e840);steps+=c.steps
        count=case-len(cases)+19 if case>=len(cases)-18 else 1
        count=max(1,count)
        lease_mask=(0,0xffffff00,0xffffff00^(0x80000000>>5))[case%3]
        c.put(0x80098798,4,lease_mask);c.put(TARGET+4,4,0x12345678)
        c.put(0x800a2f50,4,TARGET);c.put(PLAYER+1,1,2);c.put(PLAYER+0x56,2,4095)
        c.put(PLAYER+0x170,1,row);c.put(PLAYER+0x171,1,row)
        subtype=(case*17+3)&255;c.put(0x80089a8b+row*12,1,subtype)
        c.put(PLAYER+0x4fa+row*8,2,counter)
        point=(123,-234,345)
        for off in (0x3c8,0x3e0):
            for i,v in enumerate(point):c.put(PLAYER+off+i*2,2,v)
        for _ in range(count):
            c.r=[0]*32;c.r[29]=0x700000;c.r[6]=PLAYER;c.r[7]=selected&0xffffffff
            c.put(c.r[29]+0x10,4,stack10);c.steps=0;c.run(0x80035724);steps+=c.steps
        allocated=c.get(MASK,4)
        last=POOL+min(count-1,15)*268
        # Fixture excludes game instructions and copied original table values.
        rows.append([count,stack10,selected,row,counter,subtype,lease_mask,
                     allocated,c.get(0x80098798,4),c.get(TARGET+4,4),
                     c.get(PLAYER+0x4fa+row*8,2)]+[c.get(last+i,1) for i in range(268)])
    a.out.mkdir(parents=True)
    fixture=('\n'.join(' '.join(map(str,r)) for r in rows)+'\n').encode()
    (a.out/'source_buster_spawn_01436.tsv').write_bytes(fixture)
    proof={'schema':'mml-source-buster-spawn-01436/1','status':'SOURCE_BACKED_PARTIAL',
       'exe_sha256':SUPPORTED_EXE,'cases':len(rows),'instructions_executed':steps,
       'helper_hooks':0,'instruction_replacements':0,
       'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'retail_payload_exported':False,
       'limits':['Authored muzzle/table/counter/target fields, not live player pose ownership.',
         'Tests original producer allocation/lease/counter/scalar stores only.',
         'Firing cadence, projectile callback/motion, damage and complete encounter remain unknown.']}
    (a.out/'proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Original gameplay camera producer 0x80037FB4, authored states.

Executes the unchanged R3000 instructions of 0x80037FB4 and its callees
(RotMatrixYXZ 0x8007B838, TransposeMatrix 0x8007B568, rsin 0x8007A060,
rcos 0x8007A12C, ApplyMatrixLV 0x8007A9B8) from the hash-guarded owner EXE.
The restricted MVMVA adapter of research_01425 supplies GTE MAC/IR results.

Both trig tables are authored, never exported: the packed RotMatrix table
(0x80092EC4, three cells per case, alias indexes share one cell) and the
quarter-wave scalar table read by rsin/rcos (0x80092348, 1025 halfwords,
one authored table for every case). Fixture rows are authored inputs plus the
original persistent outputs; no retail table value enters a fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import sys

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01425'),str(SOURCE/'docs/research_01424'),
             str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from source_camera_source_probe import Probe
from cook_actor_combat import OwnerExe

QUARTER=0x80092348
PACKED=0x80092ec4


def camera_case(image,base,quarter,spec):
    tx,ty,tz,d,yaw,pitch,shift,pad,*trig=spec
    cpu=Probe(image,base);cpu.r[29]=0x7000
    cpu.r[4:8]=[0x1000,d&0xffffffff,yaw&0xffffffff,pitch&0xffffffff]
    for axis,value in enumerate((tx,ty,tz,0)):cpu.put(0x1000+axis*4,4,value&0xffffffff)
    for i,value in enumerate(quarter):cpu.put(QUARTER+2*i,2,value&65535)
    for index,sine,cosine in zip(((-pitch)&4095,yaw&4095,0),trig[::2],trig[1::2]):
        cpu.put(PACKED+index*4,4,(sine&65535)|((cosine&65535)<<16))
    cpu.put(0x1f800074,1,shift)
    cpu.put(0x1f8000f2,2,pad)
    cpu.run(0x80037fb4)
    outputs=[cpu.get(0x800a3a10+i*2,2,True) for i in range(3)]
    outputs+=[cpu.get(0x1f800008+i*4,4) for i in range(8)]
    outputs+=[cpu.get(0x800a39f0+i*4,4,True) for i in range(3)]
    for i in range(3):assert cpu.get(0x1f800028+i*2,2,True)==outputs[-3+i]
    # The 32-byte copy is exact: 1F800008.. mirrors 1F8000E0.. after the call.
    assert [cpu.get(0x1f8000e0+i*4,4) for i in range(8)]==outputs[3:11]
    return list(spec)+outputs,cpu.steps,cpu.gte_commands


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output outside Source')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    rng=random.Random(0x01510)
    quarter=[rng.randrange(-32768,32768) for _ in range(1025)]
    extremes=(-2147483648,-2147483647,-1073741824,-65536,-32769,-32768,-4096,-1,0,1,4095,4096,32767,32768,65536,1073741824,2147483647)
    rows=[];steps=commands=0
    for i in range(640):
        if i<64:
            # Gameplay-shaped: follow target, distance 0x200..0x300, small pitch.
            target=[rng.randrange(-8192,8192),-0x96,rng.randrange(-8192,8192)]
            d=rng.randrange(0x200,0x301);yaw=rng.randrange(0,0x2000);pitch=0x30;shift=0
        else:
            pick=lambda:rng.choice(extremes) if i<256 else rng.randrange(-1<<31,1<<31)
            target=[pick() for _ in range(3)];d=pick();yaw=pick();pitch=pick()
            shift=rng.choice((0,0,1,2)) if i<512 else rng.randrange(256)
        indexes=((-pitch)&4095,yaw&4095,0)
        table={index:(rng.randrange(-32768,32768),rng.randrange(-32768,32768)) for index in indexes}
        if i%3==0:table[0]=(0,4096)
        trig=[v for index in indexes for v in table[index]]
        spec=target+[d,yaw,pitch,shift,rng.randrange(65536)]+trig
        row,n,c=camera_case(image,owner.base,quarter,spec);rows.append(row);steps+=n;commands+=c
    args.out.mkdir(parents=True)
    payloads={'gameplay-camera-quarter':' '.join(map(str,quarter))+'\n',
              'gameplay-camera-matrix':''.join(' '.join(map(str,row))+'\n' for row in rows)}
    hashes={}
    for name,text in payloads.items():
        (args.out/(name+'.tsv')).write_text(text);hashes[name]=hashlib.sha256(text.encode()).hexdigest()
    blocks=[('gameplay_camera',0x80037fb4,0x80038324),('RotMatrixYXZ',0x8007b838,0x8007bac4),
            ('transpose',0x8007b568,0x8007b5a8),('ApplyMatrixLV',0x8007a9b8,0x8007ab18),
            ('rsin',0x8007a060,0x8007a12c),('rcos',0x8007a12c,0x8007a1cc)]
    evidence={'schema':'mml-gameplay-camera-matrix/1','status':'SOURCE_BACKED_PARTIAL',
      'exe_sha256':hashlib.sha256(image).hexdigest(),'cases':len(rows),
      'instructions_executed':steps,'mvmva_commands':commands,'fixture_sha256':hashes,
      'source_blocks':[{'id':name,'start':hex(s),'end_exclusive':hex(e),
                        'sha256':hashlib.sha256(owner.read(s,e-s)).hexdigest()} for name,s,e in blocks],
      'source_instructions_modified':False,'return_only_hooks':[],'retail_payload_exported':False,
      'limits':['Authored original CPU differential with the restricted MVMVA adapter; no hardware GTE parity.',
                'Persistent outputs only (800A3A10, 1F800008..24, 1F800028, 800A39F0); the trailing 1F800030 camera-space eye is not compared.',
                'Retained pad 1F8000F2 is an authored premise here; its live value depends on earlier scratch users.']}
    (args.out/'gameplay-camera-proof.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps({k:evidence[k] for k in ('cases','instructions_executed','mvmva_commands','fixture_sha256')}))

if __name__=='__main__':main()

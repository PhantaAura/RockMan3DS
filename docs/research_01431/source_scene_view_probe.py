#!/usr/bin/env python3
"""Original scripted radial-view oracle with authored camera/trig owners.

Unchanged8001E550→80038324→80037FB4 and original scalar/packed trig helpers.
Neither original executable instructions nor owner scalar data are exported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01425'),str(SOURCE/'docs/research_01424'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from source_camera_source_probe import Probe
from cook_actor_combat import OwnerExe, OWNER_SHA

def fill(cpu,address,blob):
    for i,b in enumerate(blob):cpu.put(address+i,1,b)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();out=args.out.resolve()
    if out.exists() or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose NEW private output outside source')
    image=args.exe.read_bytes();owner=OwnerExe(image);rng=random.Random(0x01431)
    rows=[];steps=commands=0
    extremes=(-2147483648,-2147483647,-65537,-65536,-1,0,1,65535,65536,2147483647)
    short_extremes=(-32768,-32767,-4096,-2048,-1024,-1,0,1,48,1024,2048,4095,32767)
    for seed in range(384):
        cpu=Probe(image,owner.base);cpu.r[29]=0x700000;cpu.r[28]=0x80097864
        raw=bytearray((seed*37+i*17)&255 for i in range(188));raw[0]=0
        vectors=[rng.choice(extremes) for _ in range(6)] if seed<128 else [rng.randrange(-1<<31,1<<31) for _ in range(6)]
        for i in range(3):struct.pack_into('<I',raw,8+i*4,vectors[i]&0xffffffff);struct.pack_into('<I',raw,152+i*4,vectors[3+i]&0xffffffff)
        distance,yaw,pitch=[rng.choice(short_extremes) for _ in range(3)] if seed<192 else [rng.randrange(-32768,32768) for _ in range(3)]
        for off,value in ((0x42,distance),(0x3a,yaw),(0x3e,pitch)):struct.pack_into('<h',raw,off,value)
        fill(cpu,0x800bc738,raw);cpu.put(0x800bc733,1,0);cpu.put(0x1f800074,1,seed%3)
        # Authored noncanonical inputs test CPU fixed-point behavior independently
        # of retail scalar samples; endpoints remain valid package domain.
        scalar=[0]+[((i*31+seed*17)%4097) for i in range(1,1024)]+[4096]
        fill(cpu,0x80092348,struct.pack('<1025h',*scalar))
        indexes=((-pitch)&4095,yaw&4095,0)
        table={idx:(rng.randrange(-32768,32768),rng.randrange(-32768,32768)) for idx in indexes}
        if seed%3==0:table[0]=(0,4096)
        trig=[v for idx in indexes for v in table[idx]]
        for idx,(sine,cosine) in table.items():cpu.put(0x80092ec4+idx*4,4,(sine&65535)|((cosine&65535)<<16))
        cpu.put(0x1f8000f2,2,0x8123);cpu.run(0x8001e550)
        assert not cpu.return_only_hooks
        rotation=[cpu.get(0x1f800008+i*2,2,True) for i in range(9)]
        translation=[cpu.get(0x1f800008+20+i*4,4,True) for i in range(3)]
        eye=[cpu.get(0x800a39f0+i*4,4,True) for i in range(3)]
        assert bytes(cpu.get(0x800bc738+i,1) for i in range(188))==raw,'radial producer changed script camera'
        rows.append([seed]+vectors+[distance,yaw,pitch,seed%3]+trig+rotation+translation+eye)
        steps+=cpu.steps;commands+=cpu.gte_commands
    cpu=Probe(image,owner.base);cpu.r[29]=0x700000;cpu.run(0x80011f48,0x80011f5c)
    assert cpu.control2[24:27]==[160<<16,120<<16,384] and not cpu.return_only_hooks
    startup_steps=cpu.steps
    fixture=(''.join(' '.join(map(str,row))+'\n' for row in rows)).encode()
    blocks=[('script_view',0x8001e550,0x8001e6c0),('script_radial_wrapper',0x80038324,0x80038370),('radial_view',0x80037fb4,0x80038324),('scalar_sine',0x8007a060,0x8007a12c),('scalar_cosine',0x8007a12c,0x8007a1cc),('fresh_projection',0x80011f48,0x80011f5c)]
    proof={'schema':'mml-source-scene-view/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,
      'authored_radial_cases':len(rows),'unchanged_original_instructions':steps,'restricted_mvmva_commands':commands,
      'fresh_projection_original_instructions':startup_steps,'fresh_projection_controls':{'ofx':160<<16,'ofy':120<<16,'h':384},
      'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'instruction_patches':0,'helper_hooks':0,'retail_payload_exported':False,
      'retail_parity':False,'hardware_tested':False,
      'source_blocks':[{'id':name,'start':hex(start),'end_exclusive':hex(end),'sha256':hashlib.sha256(owner.read(start,end-start)).hexdigest()} for name,start,end in blocks],
      'limits':['Authored states are not live scene chronology.',
                'Inherited restricted MVMVA arithmetic is interpreted; independent hardware GTE saturation/FLAG parity is not established.',
                'Actor-tracked and target-point camera branches remain explicit holds.',
                'Float native matrix adaptation preserves original coefficient/translation outputs but not PS1 fixed-point rasterization, GPU ordering, clipping or depth.',
                'Original scratch padding and extra mirrors are not renderer inputs and remain unowned when their writer is absent.']}
    out.mkdir(parents=True,exist_ok=False);(out/'source-scene-view.tsv').write_bytes(fixture);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

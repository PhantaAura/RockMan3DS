#!/usr/bin/env python3
"""Original direct-eye camera producer and projection setters, authored states.

Reads hash-guarded owner EXE/ST00 only. Executes unchanged original CPU
instructions; restricted published MVMVA arithmetic supplies MAC/IR outputs.
No camera updater, hardware GTE parity, retail table or instruction export.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01424'),
             str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from player_proxy_gte_source_probe import Probe as BaseProbe, s16, s32
from tutorial_actor_identity_probe import OVERLAY_SHA, iter_resources
from cook_actor_combat import OwnerExe


class Probe(BaseProbe):
    def cop2(self,word):
        rs,rt,rd=word>>21&31,word>>16&31,word>>11&31
        if rs==4:
            self.data2[rd]=s16(self.r[rt])&0xffffffff if 8<=rd<=11 else self.r[rt]
        elif rs>=16:
            command=word&0x1ffffff
            assert command in (0x41e012,0x49e012),'Unexpected camera GTE command'
            matrix=[]
            for value in self.control2[:5]:matrix.extend((s16(value),s16(value>>16)))
            vector=[s16(value) for value in self.data2[9:12]]
            shift=12 if command&0x80000 else 0
            for row in range(3):
                raw=sum(matrix[row*3+col]*vector[col] for col in range(3))
                assert -(1<<43)<=raw<(1<<43)
                mac=raw>>shift
                self.data2[25+row]=mac&0xffffffff
                self.data2[9+row]=max(-32768,min(32767,mac))&0xffffffff
            # Source never consumes FLAG in this slice. It is not compared.
            self.gte_commands+=1
        else:super().cop2(word)


def camera_case(image,base,spec):
    yaw,pitch,x,y,z,pad,*trig=spec
    cpu=Probe(image,base);cpu.r[4:7]=[0x1000,yaw&0xffffffff,pitch&0xffffffff];cpu.r[29]=0x7000
    for axis,value in enumerate((x,y,z)):cpu.put(0x1000+axis*4,4,value)
    cpu.put(0x1f8000f2,2,pad)
    for index,sine,cosine in zip(((-pitch)&4095,yaw&4095,0),trig[::2],trig[1::2]):
        cpu.put(0x80092ec4+index*4,4,(sine&65535)|((cosine&65535)<<16))
    cpu.run(0x800384f0)
    outputs=[cpu.get(0x800a3a10+i*2,2,True) for i in range(3)]
    outputs += [cpu.get(0x1f800008+i*4,4) for i in range(8)]
    outputs += [cpu.get(0x800a39f0+i*4,4,True) for i in range(3)]
    for i in range(3):assert cpu.get(0x1f800028+i*2,2,True)==outputs[-3+i]
    return list(spec)+outputs,cpu.steps,cpu.gte_commands


def projection_case(image,base,kind,a,b):
    cpu=Probe(image,base);cpu.r[4:6]=[a&0xffffffff,b&0xffffffff];cpu.r[29]=0x7000
    cpu.control2=[(0x13579bdf+i*0x10101)&0xffffffff for i in range(32)]
    entries=(0x8007b118,0x8007b138,0x8007b0c0,0x8007b0cc,0x8007a1cc)
    cpu.run(entries[kind])
    return [kind,a,b]+cpu.control2[24:29],cpu.steps


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--st00',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output outside Source')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.st00)
                 if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(overlay).hexdigest()==OVERLAY_SHA
    randomizer=random.Random(0x01425)
    camera_rows=[];projection_rows=[];steps=commands=0
    extremes=(-2147483648,-2147483647,-1073741824,-65536,-32769,-32768,-1,0,1,32767,32768,65536,1073741824,2147483647)
    for i in range(768):
        yaw=randomizer.choice(extremes) if i<128 else randomizer.randrange(-1<<31,1<<31)
        pitch=randomizer.choice(extremes) if i<128 else randomizer.randrange(-1<<31,1<<31)
        eye=[randomizer.choice(extremes) for _ in range(3)] if i<256 else [randomizer.randrange(-1<<31,1<<31) for _ in range(3)]
        # Deliberately authored, often noncanonical trig; alias indexes share
        # one actual cell. No original table scalar enters an emitted fixture.
        indexes=((-pitch)&4095,yaw&4095,0)
        table={index:(randomizer.randrange(-32768,32768),randomizer.randrange(-32768,32768)) for index in indexes}
        if i%3==0:table[0]=(0,4096)
        trig=[v for index in indexes for v in table[index]]
        spec=[yaw,pitch]+eye+[randomizer.randrange(65536)]+trig
        row,n,c=camera_case(image,owner.base,spec);camera_rows.append(row);steps+=n;commands+=c
    for kind in range(5):
        for i in range(128):
            a=randomizer.choice(extremes) if i<64 else randomizer.randrange(-1<<31,1<<31)
            b=randomizer.choice(extremes) if i<64 else randomizer.randrange(-1<<31,1<<31)
            if kind==4:
                numerator=s32((-(((a&0xffffffff)<<2)+a)*64)&0xffffffff)
                if b==0 or (numerator== -2147483648 and b== -1):continue
            row,n=projection_case(image,owner.base,kind,a,b);projection_rows.append(row);steps+=n
    expected_sites=[0x8007a53c,0x8007a548,0x8007a554,0x8007a55c,0x8007a560,
                    0x8007b0c0,0x8007b0cc,0x8007b120,0x8007b124,0x8007b138]
    def ctc_sites(data,base):
        return [base+i for i in range(0,len(data)-3,4)
                if (w:=struct.unpack_from('<I',data,i)[0])>>26==18
                and (w>>21&31)==6 and 24<=((w>>11)&31)<=28]
    assert ctc_sites(owner.read(owner.base,owner.size),owner.base)==expected_sites
    assert not ctc_sites(overlay,0x80100000)
    args.out.mkdir(parents=True)
    fixture_hashes={}
    for name,rows in [('camera-direct',camera_rows),('projection-control',projection_rows)]:
        payload=''.join(' '.join(map(str,row))+'\n' for row in rows).encode()
        (args.out/(name+'.tsv')).write_bytes(payload);fixture_hashes[name]=hashlib.sha256(payload).hexdigest()
    blocks=[('camera_direct',0x800384f0,0x80038648),('RotMatrixYX',0x8007b838,0x8007bac4),
            ('transpose',0x8007b568,0x8007b5a8),('ApplyMatrixLV',0x8007a9b8,0x8007ab18),
            ('fog',0x8007a1cc,0x8007a230),('offset',0x8007b118,0x8007b130),
            ('H',0x8007b138,0x8007b144),('DQA',0x8007b0c0,0x8007b0cc),('DQB',0x8007b0cc,0x8007b0d8)]
    evidence={'schema':'mml-source-camera-producer/1','status':'SOURCE_BACKED_PARTIAL',
      'exe_sha256':hashlib.sha256(image).hexdigest(),'st00_sha256':OVERLAY_SHA,
      'camera_cases':len(camera_rows),'projection_cases':len(projection_rows),
      'instructions_executed':steps,'mvmva_commands':commands,'fixture_sha256':fixture_hashes,
      'source_blocks':[{'id':name,'start':hex(start),'end_exclusive':hex(end),
                       'sha256':hashlib.sha256(owner.read(start,end-start)).hexdigest()} for name,start,end in blocks],
      'projection_ctc_sites':[hex(value) for value in expected_sites],'st00_direct_projection_writers':0,
      'source_instructions_modified':False,'return_only_hooks':[],'retail_payload_exported':False,
      'retail_parity':False,'current_native_camera_bound':False,
      'limits':['Authored original CPU differential with documented restricted MVMVA arithmetic; no independent hardware GTE parity.',
                'Persistent camera outputs only; transient GTE FLAG/IR/control effects are outside the native producer.',
                'No original follow update inputs, obstruction, interpolation, script transition chronology or current camera binding.',
                'Retained scratch matrix pad1F8000F2 remains an explicit premise; no zero-on-reuse assumption.']}
    (args.out/'camera-proof.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps({key:evidence[key] for key in ('camera_cases','projection_cases','instructions_executed','mvmva_commands','fixture_sha256')}))

if __name__=='__main__':main()

#!/usr/bin/env python3
"""Original root pose producer and class03 proxy tail on authored states.

Source CPU rotation/scaling/copy/registration execute. The restricted GTE
implements only MVMVA(sf1,mx0,v0,cv3), with MAC and saturated IR outputs.
No original assets, table values, instruction listings or payload are exported.
"""
import argparse
import hashlib
import inspect
import itertools
import json
from pathlib import Path
import struct
import sys
import textwrap

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE/'tools'), str(SOURCE/'docs/research_01415'),
               str(SOURCE/'docs/research_01414')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
from player_contact_source_probe import CPU
from cook_actor_combat import OwnerExe
WORK, BANK, COUNT = 0x1000, 0x800A2F48, 0x800988E0


def s16(value):
    value &= 65535
    return value - 65536 if value & 32768 else value


class Probe(OverlayProbe):
    def __init__(self, image, base, overlay=b''):
        CPU.__init__(self, image, base)
        self.overlay = overlay
        self.calls, self.indirect, self.return_only_hooks = [], [], []
        self.data2, self.control2 = [0]*32, [0]*32
        self.gte_commands = 0

    def get(self, address, size, signed=False):
        if 0x80100000 <= address and address+size <= 0x80100000+len(self.overlay):
            return int.from_bytes(self.overlay[address-0x80100000:address-0x80100000+size],
                                  'little', signed=signed)
        return CPU.get(self, address, size, signed)

    def cop2(self, word):
        rs, rt, rd = word >> 21 & 31, word >> 16 & 31, word >> 11 & 31
        if rs == 6: self.control2[rd] = self.r[rt]
        elif rs == 2: self.r[rt] = self.control2[rd]
        elif rs == 4: self.data2[rd] = self.r[rt]
        elif rs == 0: self.r[rt] = self.data2[rd]
        elif rs >= 16:
            assert word & 63 == 0x12
            assert (word >> 19 & 1, word >> 17 & 3, word >> 15 & 3, word >> 13 & 3) == (1,0,0,3)
            matrix = []
            for packed in self.control2[:5]: matrix.extend((s16(packed),s16(packed >> 16)))
            vector = [s16(self.data2[0]),s16(self.data2[0] >> 16),s16(self.data2[1])]
            for i in range(3):
                raw = sum(matrix[i*3+j]*vector[j] for j in range(3))
                assert -(1 << 43) <= raw < (1 << 43)
                mac = raw >> 12
                self.data2[25+i] = mac & 0xffffffff
                self.data2[9+i] = max(-32768,min(32767,mac)) & 0xffffffff
            self.control2[31] = 0  # FLAG is never consumed by this source slice.
            self.gte_commands += 1
        else: raise ValueError('Unsupported restricted GTE transfer')


# Extend our existing interpreter at opcode dispatch, not owner instructions.
run_source = textwrap.dedent(inspect.getsource(OverlayProbe.run)).replace('10000','100000')
needle = 'elif op in (34, 38, 42, 46):'
replacement = '''elif op == 18:
    self.cop2(w)
elif op == 50:
    self.data2[rt] = self.get((a + si) & 0xffffffff, 4)
elif op == 58:
    self.put((a + si) & 0xffffffff, 4, self.data2[rt])
''' + needle
run_source = run_source.replace(needle,textwrap.indent(replacement,'        ').lstrip(),1)
namespace = {}
exec(run_source,OverlayProbe.run.__globals__,namespace)
Probe.run = namespace['run']


def pose_case(image,base,spec,return_cpu=False):
    heading,sine,cosine,zero_sine,zero_cosine,shift,flags,*rest = spec
    scale,raw,root,root_scale = [rest[i:i+3] for i in (0,3,6,9)]
    prior,parts,*rotation = rest[12:]
    cpu = Probe(image,base)
    cpu.r[4],cpu.r[29],cpu.r[31] = WORK,0x7000,0
    cpu.put(WORK+1,1,0x20); cpu.put(WORK+6,1,flags)
    cpu.put(WORK+0xf9,1,parts); cpu.put(WORK+0x56,2,heading)
    cpu.put(WORK+0x186,2,prior); cpu.put(0x1f800074,1,shift)
    for i in range(3):
        cpu.put(WORK+0x158+2*i,2,scale[i]);cpu.put(WORK+0x14+2*i,2,raw[i])
        cpu.put(0x1f800120+2*i,2,root[i]);cpu.put(0x1f800128+2*i,2,rotation[i])
        cpu.put(WORK+0x388+i,1,root_scale[i])
    for part in range(1,parts):
        cpu.put(WORK+0xf8+part*4+1,1,0);cpu.put(WORK+0xf8+part*4+2,1,0)
        for i in range(3):cpu.put(WORK+0x388+part*4+i,1,32)
    index = abs(s16(heading)) & 4095
    cpu.put(0x80092ec4+index*4,4,(sine&65535)|((cosine&65535)<<16))
    cpu.put(0x80092ec4,4,(zero_sine&65535)|((zero_cosine&65535)<<16))
    # If heading resolves index0, the zero-angle entry is the same actual table cell.
    cpu.run(0x8003346c)
    point = [cpu.get(WORK+0x180+2*i,2,True) for i in range(4)]
    matrix = [cpu.get(WORK+0x160+2*i,2,True) for i in range(9)]
    return cpu if return_cpu else (list(spec)+point+matrix,cpu.steps,cpu.gte_commands)


def tail_case(image,base,overlay,spec):
    work00,flags,source_class,producer,packet,target_flags,mask,count,*point = spec
    cpu = Probe(image,base,overlay)
    cpu.r[16],cpu.r[17],cpu.r[29],cpu.r[31] = WORK,WORK+0x328,0x7000,0
    cpu.put(WORK,1,work00);cpu.put(WORK+1,1,source_class)
    cpu.put(WORK+0x328,2,flags);cpu.put(WORK+0x330,2,producer)
    cpu.put(WORK+0x33c,4,packet);cpu.put(WORK+0x340,4,target_flags)
    cpu.put(0x80098798,4,mask);cpu.put(COUNT,4,count)
    for i in range(4):cpu.put(WORK+0x180+i*2,2,point[i]);cpu.put(WORK+0x334+i*2,2,91+i)
    for i in range(24):
        cpu.put(0x800be478+i*4,4,0x3000+i*0x100)
        cpu.put(0x3000+i*0x100+0xbc,4,0xa5a5a5a5)
        cpu.put(0x3000+i*0x100+0xc0,4,0x5a5a5a5a)
    cpu.run(0x80102370)
    after_count=cpu.get(COUNT,4)
    record=[cpu.get(BANK+count*12+off,width) if count<64 and after_count>count else 0
            for off,width in ((0,4),(4,4),(8,4))]
    after=[cpu.get(WORK+0x334+i*2,2,True) for i in range(4)]
    after += [cpu.get(WORK+0x33c,4),cpu.get(WORK+0x340,4),after_count]+record
    after += [cpu.get(0x3000+0xbc+j*4,4) for j in range(2)]
    return list(spec)+after,cpu.steps


def chain_case(image,base,overlay):
    # The second producer is authored mode1/low16=3; this is not a recovered Buster.
    spec=(0,0,4096,0,4096,0,0,512,512,512,0,0,0,16,-96,32,32,32,32,-1234,1,0,0,0)
    cpu=pose_case(image,base,spec,True);cpu.overlay=overlay
    for off,width,value in ((0,1,1),(2,1,3),(3,1,255),(0x328,2,14),(0x330,2,0),
            (0x33c,4,0x08000009),(0x6e,2,100),(0x32e,2,0)):
        cpu.put(WORK+off,width,value)
    cpu.put(COUNT,4,0);cpu.put(0x80098798,4,255)
    def tail():
        cpu.r=[0]*32;cpu.r[16],cpu.r[17],cpu.r[29]=WORK,WORK+0x328,0x7000
        cpu.put(0x7030,4,0);cpu.run(0x80102370)
    tail();assert cpu.get(WORK+0x33c,4)==0 and cpu.get(COUNT,4)==1
    cpu.r=[0]*32;cpu.r[4:8]=[0x2000,32,0,0x2000];cpu.r[29]=0x7000
    cpu.put(0x2000,4,16|((-96&65535)<<16));cpu.put(0x2004,4,32)
    for off,value in ((16,0x60),(20,0x2100),(24,0x08000003),(28,0),(32,0)):
        cpu.put(0x7000+off,4,value)
    cpu.run(0x800341bc)
    for i in range(8):cpu.put(0x80097d00+i,1,255)
    for entry in (0x80076a84,0x80034eb8,0x800358f0):cpu.hook(entry)
    cpu.r=[0]*32;cpu.r[28],cpu.r[29]=0x80097864,0x7000;cpu.run(0x8003450c)
    produced_packet=cpu.get(WORK+0x33c,4);assert produced_packet==0x08000003
    assert cpu.get(WORK+0x34a,2)==1 and cpu.get(COUNT,4)==0
    cpu.put(0x80098844,4,0x3000);cpu.put(0x300c,4,0x4000);cpu.put(0x4009,1,9)
    cpu.put(0x80098938,4,0x9000)
    cpu.r=[0]*32;cpu.r[4],cpu.r[29]=WORK,0x7000;cpu.run(0x801041b0)
    outputs={'health6e':cpu.get(WORK+0x6e,2,True),'counter332':cpu.get(WORK+0x332,2,True),
             'action09':cpu.get(WORK+9,1),'sound_cue':cpu.get(0x9002,2),
             'packet_before_tail':cpu.get(WORK+0x33c,4)}
    assert outputs=={'health6e':97,'counter332':3,'action09':6,'sound_cue':160,'packet_before_tail':0x08000003}
    tail();assert cpu.get(WORK+0x33c,4)==0 and cpu.get(COUNT,4)==1
    outputs.update({'packet_after_tail':0,'next_registration_count':1,'instructions':cpu.steps,
                    'authored_second_proxy_mode':1,'authored_second_producer':'0x08000003',
                    'real_buster_producer':False})
    return outputs


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path);parser.add_argument('--dat',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path);args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local evidence outside source')
    image=args.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SUPPORTED_EXE
    owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.dat/'ST00.BIN')
                 if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(overlay).hexdigest()==OVERLAY_SHA
    pose_rows=[];tail_rows=[];steps=0;commands=0
    for heading,trig,shift,flags in itertools.product(
            (0,1,1024,2048,4095,32767,32768,65535),
            ((0,4096),(4096,0),(-301,4084)),(0,1,2,5,15,31,32,255),(0,64)):
        spec=(heading,*trig,0,4096,shift,flags,512,512,512,-32768,17,32767,
              -2048,2047,-31,0,32,255,-1234,1,0,0,0)
        row,n,c=pose_case(image,owner.base,spec);pose_rows.append(row);steps+=n;commands+=c
    for scale,raw,root,root_scale,zero_trig,parts,rotation in itertools.product(
            ((-32768,32767,-1),(32767,-32768,512)),
            ((-32768,32767,0),(99,-100,-1)),
            ((-2048,2047,0),(11,-31,1023)),((0,0,0),(255,128,1)),
            ((0,4096),(1,4095)),(1,3,13),((0,0,0),(1024,2048,3072))):
        spec=(65535,-301,4084,*zero_trig,2,64,*scale,*raw,*root,*root_scale,32767,parts,*rotation)
        row,n,c=pose_case(image,owner.base,spec);pose_rows.append(row);steps+=n;commands+=c
    for work00,flags,producer,packet,target_flags,mask,count in itertools.product(
            (0,1,2,3),(0,4,14),(0,65535),(0,0x08000003),
            (0x000000ff,0x800000ff),(0,255,0xffffffff),(0,63,64)):
        spec=(work00,flags,0x2f,producer,packet,target_flags,mask,count,-32768,17,32767,-1234)
        row,n=tail_case(image,owner.base,overlay,spec);tail_rows.append(row);steps+=n
    ownership=[]
    for entry,expected in ((0x8003697c,0),(0x800369c0,1),(0x80036a1c,2)):
        cpu=Probe(image,owner.base);cpu.r[29]=0x7000;cpu.put(0x1f800074,1,255)
        cpu.hook(0x8007a1cc);cpu.run(entry)
        assert cpu.get(0x1f800074,1)==expected;steps+=cpu.steps
        ownership.append({'entry':hex(entry),'scratch_shift':expected,'instructions':cpu.steps,
                          'excluded_projection_consumer':'0x8007A1CC'})
    cpu=Probe(image,owner.base);cpu.r[29]=0x7000
    for slot in range(16):cpu.put(0x8009f108+slot*0x3c0+0x186,2,0xa55a)
    cpu.run(0x8002e840,0x8002e854)
    assert all(cpu.get(0x8009f108+slot*0x3c0+0x186,2)==0 for slot in range(16))
    ownership.append({'entry':'0x8002E840..8002E854','fresh_class20_pool_halfword186':0,
                      'all16_dirty_sentinels_cleared':True,'instructions':cpu.steps});steps+=cpu.steps
    cpu=Probe(image,owner.base);cpu.put(0x800987e8,4,0);cpu.put(0x8009f28e,2,0xa55a)
    cpu.run(0x80031418);assert cpu.get(0x8009f28e,2)==0xa55a
    ownership.append({'entry':'0x80031418','reused_halfword186_preserved':True,'instructions':cpu.steps});steps+=cpu.steps
    cpu=Probe(image,owner.base,overlay);cpu.r[16]=WORK
    cpu.run(0x8010244c,0x80102460)
    assert [cpu.get(WORK+0x158+i*2,2,True) for i in range(3)]==[512]*3
    ownership.append({'entry':'0x8010244C..80102460','class03_scale158_xyz':[512]*3,'instructions':cpu.steps});steps+=cpu.steps
    cpu=Probe(image,owner.base,overlay);cpu.r[17]=WORK+0x328
    for off in (0x33c,0x340,0x344,0x348):cpu.put(WORK+off,4,0xa5a5a5a5)
    cpu.run(0x801024a8,0x801024b0)
    assert cpu.get(WORK+0x33c,4)==cpu.get(WORK+0x340,4)==0
    assert cpu.get(WORK+0x344,4)==cpu.get(WORK+0x348,4)==0xa5a5a5a5
    ownership.append({'entry':'0x801024A8..801024B0','packet33c_and_flags340_cleared':True,
                      'point344_and_word34a_preserved':True,'instructions':cpu.steps});steps+=cpu.steps
    chain=chain_case(image,owner.base,overlay);steps+=chain['instructions']
    pose_data=(''.join(' '.join(map(str,row))+'\n' for row in pose_rows)).encode()
    tail_data=(''.join(' '.join(map(str,row))+'\n' for row in tail_rows)).encode()
    args.out.mkdir(parents=True)
    (args.out/'pose.tsv').write_bytes(pose_data);(args.out/'tail.tsv').write_bytes(tail_data)
    result={'schema':'mml-actor-pose-proxy-source/1','status':'SOURCE_BACKED_PARTIAL',
            'exe_sha256':SUPPORTED_EXE,'overlay_sha256':OVERLAY_SHA,
            'pose_cases':len(pose_rows),'tail_cases':len(tail_rows),'instructions_executed':steps,
            'restricted_gte_commands':commands,'pose_fixture_sha256':hashlib.sha256(pose_data).hexdigest(),
            'tail_fixture_sha256':hashlib.sha256(tail_data).hexdigest(),
            'ownership_checks':ownership,'combined_original_chain':chain,
            'pose_code_sha256':hashlib.sha256(owner.read(0x8003346c,0x5bc)).hexdigest(),
            'tail_code_sha256':hashlib.sha256(overlay[0x2370:0x2410]).hexdigest(),
            'retail_payload_exported':False,'retail_parity':False,
            'limits':['Authored inputs and restricted arithmetic GTE; no live retail combat trace.',
                      'Only root work180 collision point is translated; child render/pose transforms excluded.',
                      'Full original producer executes, including child matrix/point loops on authored parts.',
                      'Proxy tail includes actual registration and after-call packet clear; earlier outer consumers excluded.',
                      'Player/GTE80034EB8/800358F0 and real Buster producers remain pending.']}
    (args.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('pose_cases','tail_cases','instructions_executed','restricted_gte_commands','pose_fixture_sha256','tail_fixture_sha256')}))


if __name__=='__main__':main()

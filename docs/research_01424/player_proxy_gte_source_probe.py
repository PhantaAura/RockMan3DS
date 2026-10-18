#!/usr/bin/env python3
"""Unchanged80035508 lease consumer with authored GTE state and a documented
restricted RTPS adapter. The CPU slice is original; GTE arithmetic is a model,
not hardware-derived retail parity. Emits authored scalar fixtures/hashes only.
"""
import argparse
import hashlib
import inspect
import itertools
import json
from pathlib import Path
import random
import struct
import sys
import textwrap

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE
from player_contact_source_probe import CPU
from cook_actor_combat import OwnerExe

CHANGED=(8,9,10,11,12,13,14,15,16,17,18,19,24,25,26,27)
OWNER=0x3000

def s16(value):
    value&=65535
    return value-65536 if value&32768 else value

def s32(value):
    value&=0xffffffff
    return value-4294967296 if value&0x80000000 else value


def unr(h,z):
    if h>=z*2:return 0x1ffff,True
    shift=16-z.bit_length()
    n,d=h<<shift,z<<shift
    index=(d-0x7fc0)>>7
    u=max(0,((0x40000//(index+0x100)+1)//2)-0x101)+0x101
    r=(0x2000080-d*u)>>8
    r=(0x80+r*u)>>8
    return min(0x1ffff,(n*r+0x8000)>>16),False


def projection(data,control):
    data=list(data)
    flags=0
    def clamp(x,low,high,bit):
        nonlocal flags
        if x<low:flags|=1<<bit;return low
        if x>high:flags|=1<<bit;return high
        return x
    def mac0(x):
        nonlocal flags
        if x>0x7fffffff:flags|=1<<16
        if x< -0x80000000:flags|=1<<15
    matrix=[]
    for word in control[:5]:matrix.extend((s16(word),s16(word>>16)))
    point=(s16(data[0]),s16(data[0]>>16),s16(data[1]))
    raw=[]
    for row in range(3):
        accumulator=s32(control[row+5])*4096
        assert -(1<<43)<=accumulator<(1<<43),'ExcludedMACdomain'
        for col in range(3):
            accumulator+=matrix[row*3+col]*point[col]
            assert -(1<<43)<=accumulator<(1<<43),'ExcludedMACdomain'
        raw.append(accumulator>>12)
    ir=[clamp(x,-32768,32767,24-i) for i,x in enumerate(raw)]
    for i,x in enumerate(ir):data[9+i]=x&0xffffffff;data[25+i]=raw[i]&0xffffffff
    depth=clamp(raw[2],0,65535,18)
    data[16:20]=data[17:20]+[depth]
    reciprocal,overflow=unr(control[26]&65535,depth)
    if overflow:flags|=1<<17
    screen=[]
    for i in range(2):
        x=reciprocal*ir[i]+s32(control[24+i]);mac0(x)
        screen.append(clamp(x>>16,-1024,1023,14-i))
    packed=(screen[0]&65535)|((screen[1]&65535)<<16)
    data[12:15]=[data[13],data[14],packed] # raw backing15 retained; CPU read15 mirrors14
    raw0=reciprocal*s16(control[27])+s32(control[28]);mac0(raw0)
    data[24]=raw0&0xffffffff;data[8]=clamp(raw0>>12,0,4096,12)
    return data,flags| (0x80000000 if flags&0x7f87e000 else 0)


class Probe(OverlayProbe):
    def __init__(self,image,base):
        CPU.__init__(self,image,base)
        self.calls,self.indirect,self.return_only_hooks=[],[],[]
        self.data2,self.control2=[0]*32,[0]*32
        self.gte_commands=0
    def cop2(self,word):
        rs,rt,rd=word>>21&31,word>>16&31,word>>11&31
        if rs==6:
            value=self.r[rt]
            self.control2[rd]=(s16(value)&0xffffffff) if rd in (4,12,20,26,27,29,30) else value
        elif rs==2:self.r[rt]=self.control2[rd]
        elif rs==0:
            value=self.data2[14 if rd==15 else rd]
            if rd in (1,3,5,8,9,10,11):value=s16(value)&0xffffffff
            if rd in (7,16,17,18,19):value &=65535
            self.r[rt]=value
        elif rs>=16:
            assert word&0x1ffffff==0x180001,'UnexpectedGTEcommand'
            self.data2,self.control2[31]=projection(self.data2,self.control2)
            self.gte_commands+=1
        else:raise ValueError('RestrictedunsupportedGTEtransfer')
    def lwc2(self,rd,value):
        self.data2[rd]=s16(value)&0xffffffff if rd in (1,3,5,8,9,10,11) else value


# Interpreter extension only; no retail instruction replacements or helper hooks.
run_source=textwrap.dedent(inspect.getsource(OverlayProbe.run)).replace('10000','100000')
needle='elif op in (34, 38, 42, 46):'
replacement='''elif op == 18:
    self.cop2(w)
elif op == 50:
    self.lwc2(rt,self.get((a+si)&0xffffffff,4))
elif op == 58:
    self.put((a+si)&0xffffffff,4,self.data2[rt])
'''+needle
run_source=run_source.replace(needle,textwrap.indent(replacement,'        ').lstrip(),1)
namespace={};exec(run_source,OverlayProbe.run.__globals__,namespace);Probe.run=namespace['run']


def pack_matrix(matrix,translation):
    return [(matrix[i]&65535)|((matrix[i+1]&65535)<<16) for i in range(0,8,2)]+[matrix[8]&0xffffffff]+[v&0xffffffff for v in translation]


def run_case(image,base,camera,projection_control,mask,points,alias):
    cpu=Probe(image,base)
    before=[(0x13579bdf+i*0x1111111)&0xffffffff for i in range(32)]
    cpu.data2=list(before)
    cpu.control2=[(0xa5a50000+i)&0xffffffff for i in range(32)]
    cpu.control2[24:29]=[v&0xffffffff for v in projection_control]
    cpu.r[29]=0x7000
    for i,w in enumerate(camera):cpu.put(0x1f800008+i*4,4,w)
    cpu.put(0x80098798,4,mask)
    pointers=[]
    for i,point in enumerate(points):
        pointer=OWNER+(0 if alias else i)*0x100
        pointers.append(pointer);cpu.put(0x800be478+i*4,4,pointer)
        # Repeated source pointers are one actual memory owner, not24 copies.
        if not alias or i==0:
            cpu.put(pointer+0xbc,4,point[0]);cpu.put(pointer+0xc0,4,point[1])
    cpu.run(0x80035508)
    owners=[[cpu.get(pointer+0xbc,4),cpu.get(pointer+0xc0,4)] for pointer in pointers]
    result=list(camera)+[v&0xffffffff for v in projection_control]+[mask,int(alias)]
    result+=sum([list(p) for p in points],[])+[cpu.data2[i] for i in CHANGED]+[cpu.control2[31]]
    result+=sum(owners,[])+[cpu.gte_commands]
    return result,cpu.steps,cpu.gte_commands


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path);parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):raise ValueError('ChooseNEWowner-localoutputoutsideSource')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    randomizer=random.Random(0x01424)
    identity=(4096,0,0,0,4096,0,0,0,4096)
    matrices=(identity,(0,0,4096,0,4096,0,-4096,0,0),
              (3010,-1777,2200,-1789,4095,-1050,3200,1100,-2500))
    cases=[];steps=commands=0
    point_options=((-32768,32767,-32768,0xa5a5),(0,0,0,0xffff),(1024,-4096,32000,17),
                   (-1,1,1,0x8000),(32767,32767,32767,0x1234))
    for matrix,translation,control,mask,alias,point in itertools.product(matrices,
            ((0,0,0),(500,-600,2000),(-80000,80000,100000)),
            ((160<<16,120<<16,320,-98,0x1000000),(-2147483648,2147483647,65535,32767,2147483647),
             (0,0,0,-32768,-2147483648)),
            (0,255,0x80000000,0x100,0x80000100,0xffffff00,0xffffffff),(False,True),point_options):
        words=((point[0]&65535)|((point[1]&65535)<<16),(point[2]&65535)|((point[3]&65535)<<16))
        points=[words for _ in range(24)]
        row,n,c=run_case(image,owner.base,pack_matrix(matrix,translation),control,mask,points,alias)
        cases.append(row);steps+=n;commands+=c
    # Varied owner points establish source iteration/FIFO order independently
    # of the repeated-pointer alias fixtures.
    for _ in range(128):
        points=[]
        for _ in range(24):
            v=[randomizer.randrange(-32768,32768) for _ in range(4)]
            points.append(((v[0]&65535)|((v[1]&65535)<<16),(v[2]&65535)|((v[3]&65535)<<16)))
        camera=pack_matrix(randomizer.choice(matrices),[randomizer.randrange(-10000,10000) for _ in range(3)])
        control=(randomizer.randrange(-1<<31,1<<31),randomizer.randrange(-1<<31,1<<31),
                 randomizer.randrange(65536),randomizer.randrange(-32768,32768),randomizer.randrange(-1<<31,1<<31))
        row,n,c=run_case(image,owner.base,camera,control,randomizer.getrandbits(32),points,False)
        cases.append(row);steps+=n;commands+=c
    payload=''.join(' '.join(map(str,row))+'\n' for row in cases).encode()
    args.out.mkdir(parents=True);(args.out/'player-lease.tsv').write_bytes(payload)
    evidence={'schema':'mml-source-player-lease/1','status':'SOURCE_BACKED_PARTIAL',
      'exe_sha256':SUPPORTED_EXE,'source_function':'0x80035508','source_code_sha256':hashlib.sha256(owner.read(0x80035508,0xe8)).hexdigest(),
      'source_cases':len(cases),'instructions_executed':steps,'rtps_commands':commands,
      'fixture_sha256':hashlib.sha256(payload).hexdigest(),'return_only_hooks':[],
      'source_instructions_modified':False,'retail_payload_exported':False,'retail_parity':False,
      'hardware_math_reference':'https://psx-spx.consoledev.net/geometrytransformationenginegte/',
      'limits':['Authored CPU contexts and documented restricted RTPS model; not an independent hardware GTE differential.',
                'Internal44bit MAC overflow is rejected transactionally; otherGTE commands/cycle timing are not supported.',
                'Original camera/OFX/OFY/H/DQA/DQB ownership remains explicit. No native float camera substitution.',
                '80034EB8 target selection and80035724 Buster allocation/muzzle producers are not established by lease completion.']}
    (args.out/'player-lease-proof.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps({k:evidence[k] for k in ('source_cases','instructions_executed','rtps_commands','fixture_sha256')}))

if __name__=='__main__':main()

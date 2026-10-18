#!/usr/bin/env python3
"""Bounded original80034EB8 broad-target selection probe.

The routine under test is unchanged. 80035508 is a return-only prerequisite
hook and 80035724 is replaced by an argument-capture helper so this probe tests
selection without allocating retail projectiles or exporting retail tables.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01424'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from player_proxy_gte_source_probe import Probe,pack_matrix
from tutorial_actor_identity_probe import SUPPORTED_EXE

PLAYER=0x800b51b0
CAPTURE=0x3000

def encode_i(op,rs,rt,imm):return (op<<26)|(rs<<21)|(rt<<16)|(imm&0xffff)
def install_capture(cpu,address):
    # t0=CAPTURE; [0]=a1(has target), [4]=a3(index), [8]=caller stack10(lane)
    words=[encode_i(15,0,8,CAPTURE>>16),encode_i(13,8,8,CAPTURE&0xffff),
           encode_i(43,8,5,0),encode_i(43,8,7,4),encode_i(35,29,9,0x10),
           encode_i(43,8,9,8),(31<<21)|8,0]
    for i,w in enumerate(words):cpu.put(address+i*4,4,w)
    cpu.return_only_hooks.append(hex(address)+' argument-capture')

def row(cpu,index,point,shape=0,flags=0):
    at=index*12
    x,y,z=point
    cpu.put(0x800a2f48+at,2,x);cpu.put(0x800a2f4a+at,2,y)
    cpu.put(0x800a2f4c+at,2,z);cpu.put(0x800a2f4e+at,2,shape)
    target=0x4000+index*0x20
    cpu.put(0x800a2f50+at,4,target);cpu.put(target+4,4,flags)

def run_case(image,base,points,shapes=None,flags=None):
    c=Probe(image,base);c.r[28]=0x80097864;c.r[29]=0x7000;c.r[4]=0
    c.hook(0x80035508);install_capture(c,0x80035724)
    camera=pack_matrix((4096,0,0,0,4096,0,0,0,4096),(0,0,0))
    for i,w in enumerate(camera):c.put(0x1f800008+i*4,4,w)
    c.control2[24:29]=[160<<16,120<<16,320,0,0]
    c.put(PLAYER,1,1);c.put(PLAYER+0x14,2,0);c.put(PLAYER+0x16,2,0);c.put(PLAYER+0x18,2,100)
    c.put(PLAYER+0x174,1,0x20);c.put(0x80097d02,1,0x80);c.put(0x800988e0,4,len(points))
    shapes=shapes or [0]*len(points);flags=flags or [0]*len(points)
    for i,p in enumerate(points):row(c,i,p,shapes[i],flags[i])
    c.run(0x80034eb8)
    selected=struct.unpack('<i',struct.pack('<I',c.get(CAPTURE+4,4)))[0]
    return {'points':[list(p) for p in points],'shapes':shapes,'flags':flags,
            'has_target':c.get(CAPTURE,4),'selected':selected,'lane':c.get(CAPTURE+8,4),
            'steps':c.steps,'gte_commands':c.gte_commands}

def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
    image=a.exe.read_bytes();digest=hashlib.sha256(image).hexdigest()
    if digest!=SUPPORTED_EXE:raise ValueError('unsupported owner executable')
    base=struct.unpack_from('<I',image,24)[0]
    cases=[run_case(image,base,[(0,0,500)]),
           run_case(image,base,[(20,0,500),(0,0,500)]),
           run_case(image,base,[(100,0,500)]),
           run_case(image,base,[(0,0,120)]),
           run_case(image,base,[(0,0,500)],flags=[0x80]),
           run_case(image,base,[(-10,0,500),(10,0,500)],shapes=[0x200,0x200])]
    expected=[(1,0),(1,1),(0,-1),(0,-1),(0,-1),(1,1)]
    if [(r['has_target'],r['selected']) for r in cases]!=expected:raise ValueError('unexpected original selection results')
    out={'schema':'mml-source-player-buster-select-01448/1','status':'SOURCE_BACKED_PARTIAL',
         'exe_sha256':digest,'source_function':'0x80034EB8','cases':cases,
         'instructions_executed':sum(r['steps'] for r in cases),
         'gte_commands':sum(r['gte_commands'] for r in cases),
         'return_only_hooks':['0x80035508'],'argument_capture_hooks':['0x80035724'],
         'source_instructions_modified':False,'retail_payload_exported':False,
         'proved':['bit20 broad window admission','closest-to-screen-center selection','depth gate','target flag80 rejection','composite row advancement'],
         'limits':['Synthetic player/proxy/GTE contexts, not a live retail trace.',
                   'The private owner table at8008994C is neither copied nor exported; bit10 authored-window parity remains pending.',
                   'Projectile callback, cadence, motion, damage and encounter completion remain outside this probe.']}
    a.out.mkdir(parents=True,exist_ok=False);(a.out/'proof.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps({'cases':len(cases),'instructions_executed':out['instructions_executed'],'gte_commands':out['gte_commands']}))

if __name__=='__main__':main()

#!/usr/bin/env python3
"""Unchanged class40 integer pose oracle. Retail outputs remain owner-local."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[3]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01414'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01423')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from actor_pose_proxy_source_probe import Probe
WORK=0x800c3580

def fill(c,at,b):
    for i,n in enumerate(b):c.put(at+i,1,n)
def read(c,at,n):return bytes(c.get(at+i,1) for i in range(n))
def sha(b):return hashlib.sha256(b).hexdigest()
def owner_case(image,owner,overlay,ebd,shift,position=None):
    c=Probe(image,owner.base,overlay);fill(c,0x8016c000,ebd)
    for lo,hi in ((0x8002e88c,0x8002e8d8),(0x8002e924,0x8002e980),(0x8002e9cc,0x8002ea18)):
        c.r=[0]*32;c.r[29]=0x700000;c.run(lo,hi)
    cold=read(c,WORK,1444);assert all(cold[i]==(64 if i==1 else 0) for i in range(1444))
    c.r=[0]*32;c.r[29]=0x700000;c.r[4]=0x800c4c48;c.run(0x80100efc)
    c.r=[0]*32;c.r[29]=0x700000;c.r[4]=WORK;c.run(0x8010c930,0x800322a8)
    if position:
        for i,n in enumerate(position):c.put(WORK+20+i*2,2,n)
    before=read(c,WORK,1444);c.r[31]=0;c.put(0x1f800074,1,shift);c.steps=0;c.gte_commands=0
    c.run(0x800322a8);assert not c.return_only_hooks
    return [shift,before.hex(),read(c,WORK,1444).hex()],c.steps,c.gte_commands

def authored_case(image,owner,seed,shift,heading,scales):
    c=Probe(image,owner.base);c.r[4]=WORK;c.r[29]=0x700000
    c.put(WORK+1,1,0x40);c.put(WORK+2,1,5);c.put(WORK+6,1,0x23);c.put(WORK+249,1,3);c.put(WORK+156,4,0x3000)
    c.put(WORK+86,2,heading);c.put(0x1f800074,1,shift)
    # A complete authored packed sine/cosine table; no original table exports.
    table=bytearray()
    for i in range(4096):
        sine=((i*37+seed*311)&65535)-32768;cosine=((i*19+seed*137)&65535)-32768
        if i==0:sine,cosine=0,4096
        table+=struct.pack('<hh',sine,cosine)
    fill(c,0x80092ec4,table)
    raw=(-32768+seed*13,17,32767-seed*19);root=(-2048+seed*7,2047-seed*11,-31)
    rotations=((65535,1024,2048),(4095,32768,65535),(3072,17,1));translation=((0,0,0),(16,-31,48),(-32768,32767,-1))
    for i in range(3):
        c.put(WORK+344+i*2,2,scales[i]);c.put(WORK+20+i*2,2,raw[i]);c.put(0x1f800120+i*2,2,root[i])
    descriptor=bytearray(96);descriptor[1]=3
    for i in range(3):
        if i:descriptor[i*4+1]=i-1;descriptor[i*4+2]=i
        for k in range(3):c.put(0x1f800128+i*8+k*2,2,rotations[i][k]);c.put(0x3000+i*8+k*2,2,translation[i][k])
    fill(c,WORK+248,descriptor);c.steps=0;c.run(0x8003346c);assert not c.return_only_hooks
    spec=[seed,shift,heading,*scales,*raw,*root,*sum(rotations,()),*sum(translation,())]
    result=read(c,WORK+352,20).hex()+read(c,WORK+384,24).hex()+b''.join(read(c,WORK+576+i*32,20) for i in range(3)).hex()
    return spec+[result],c.steps,c.gte_commands

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','overlay','ebd','out'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();out=a.out.resolve()
    if out.exists() or out.is_relative_to(SOURCE):raise ValueError('Choose NEW private evidence directory')
    image=a.exe.read_bytes();owner=OwnerExe(image);overlay=a.overlay.read_bytes();ebd=a.ebd.read_bytes()
    if sha(overlay)!='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c' or sha(ebd)!='ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15':raise ValueError('Unsupported owners')
    rows=[];steps=gte=0
    for position in (None,(-32768,-32768,-32768),(32767,32767,32767)):
        for shift in (0,1,2):
            row,n,g=owner_case(image,owner,overlay,ebd,shift,position);rows.append(row);steps+=n;gte+=g
    owner_fixture=('shift before after\n'+''.join(' '.join(map(str,r))+'\n' for r in rows)).encode()
    authored=[];authored_steps=authored_gte=0
    for heading in (0,1,1024,4095,65535,32768):
        for scales in ((512,512,512),(-32768,32767,-1)):
            for shift in (0,1,2):
                row,n,g=authored_case(image,owner,len(authored),shift,heading,scales);authored.append(row);authored_steps+=n;authored_gte+=g
    authored_fixture=('seed shift heading scales raw root rotations translations outputs\n'+''.join(' '.join(map(str,r))+'\n' for r in authored)).encode()
    proof={'schema':'source-scene-actor40-integer-pose/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,'overlay_sha256':sha(overlay),'ebd_sha256':sha(ebd),
           'actual_first_pose_cases':3,'owner_pose_boundary_cases':6,'owner_part_bank_comparisons':len(rows)*18,'owner_original_instructions':steps,'owner_restricted_mvmva':gte,
           'authored_cases':len(authored),'authored_original_instructions':authored_steps,'authored_restricted_mvmva':authored_gte,'private_owner_fixture_sha256':sha(owner_fixture),'authored_fixture_sha256':sha(authored_fixture),
           'instruction_patches':0,'helper_hooks':0,'scratch_shift_values':[0,1,2],'scratch_shift_binding':'EXPLICIT_INPUT_NOT_A_NATIVE_CLOCK','cold_pool_padding_owner':'8002E88C+800177CC_ACTUAL_FULL_SLOT_CLEAR',
           'opaque_stack_copy_lanes':'work174..17F invalidated, never filled from an invented zero','hardware_tested':False,'retail_parity':False,
           'limits':['Restricted GTE MVMVA arithmetic model; timing/unused flags are excluded.','Shift selection remains the outer original projection-consumer owner.','Source08, overlay/player wrapper globals, later requests/records and scene chronology are separate consumers.','Retail pose/table/bank outputs stay private.']}
    out.mkdir(parents=True);(out/'owner-pose.tsv').write_bytes(owner_fixture);(out/'authored-pose.tsv').write_bytes(authored_fixture);(out/'reference-trig.bin').write_bytes(owner.read(0x80092ec4,16384));(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

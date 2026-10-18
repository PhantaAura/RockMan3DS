#!/usr/bin/env python3
"""Unchanged phase5 constructor oracle with authored pools/spawn/resources.

Only data is authored. No instruction patches, helper hooks or original asset
payloads are exported. Cold-clear and camera initializer prefixes are measured
separately; authored full-constructor work is not a live retail trace.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources

WORK=0x800c4c48
POOLS=((0x800c3580,4,1444,0x40),(0x800a3b40,64,72,0x80),(0x800bf118,32,80,0xe0))
def sha(data):return hashlib.sha256(data).hexdigest()
def authored(seed,index):return bytes((seed*37+i*17+index*11)&255 for i in range(index))
def fill(cpu,address,data):
    for i,value in enumerate(data):cpu.put(address+i,1,value)
def read(cpu,address,size):return bytes(cpu.get(address+i,1) for i in range(size))
def record(seed,i):
    raw=bytearray((seed*13+i*19+j*23)&255 for j in range(20))
    raw[0]=i;raw[1]=5 if i==0 else 3;raw[3]=0x40 if i==0 else 0xe0
    return bytes(raw)
class Probe(OverlayProbe):
    def __init__(self,*args):super().__init__(*args);self.lookup_misses=0
    def put(self,address,size,value):
        if address==0x87654321:self.lookup_misses+=1
        return super().put(address,size,value)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','st00','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists():raise ValueError('Choose a NEW evidence directory')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay)!=OVERLAY_SHA:raise ValueError('Unsupported overlay identity')
    clear_rows=[];clear_steps=0
    for seed in range(4):
        cpu=Probe(image,owner.base,overlay);cpu.r[29]=0x700000
        for base,count,width,family in POOLS:fill(cpu,base,authored(seed,count*width))
        for lo,hi in ((0x8002e88c,0x8002e8d8),(0x8002e924,0x8002e980),(0x8002e9cc,0x8002ea18)):
            cpu.steps=0;cpu.run(lo,hi);clear_steps+=cpu.steps
        for base,count,width,family in POOLS:
            expected=bytearray(count*width)
            for i in range(count):expected[i*width+1]=family|i
            assert read(cpu,base,count*width)==expected,'Original inclusive word clearing/tag mismatch'
        assert all(cpu.get(a,4)==0 for a in (0x80098b20,0x80098838,0x8009883c,0x800989cc))
        assert not cpu.return_only_hooks
        clear_rows.append(seed)
    camera_rows=[];camera_steps=0
    for seed in range(24):
        cpu=Probe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[16]=0x800a3958
        position=bytes((seed*17+i*31)&255 for i in range(6));fill(cpu,0x800b51c4,position)
        cpu.run(0x80038674,0x800387bc);camera_steps+=cpu.steps
        eye=[cpu.get(0x800a39a0+i*4,4) for i in range(3)]
        rotation=[cpu.get(0x800a3964+i*4,4) for i in range(3)]
        assert eye==[v&0xffffffff for v in struct.unpack('<3h',position)] and rotation==[512,0,48]
        assert cpu.get(0x800a395c,1)==1 and not cpu.return_only_hooks
        camera_rows.append([seed,position.hex(),*eye,*rotation])
    rows=[];steps=0;maximum=0
    for seed in range(24):
        cpu=Probe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[4]=WORK
        # Explicit captured/native owners: retained bytes are deliberately not
        # zero, so a port must preserve lanes not written by the constructor.
        before=bytearray(authored(seed,168));before[0]=1;before[1]=1;before[4]=5
        fill(cpu,WORK,before);fill(cpu,0x800bc738,authored(seed,188))
        for base,count,width,family in POOLS:
            values=bytearray(authored(seed,count*width))
            for i in range(count):values[i*width+1]=family|i
            fill(cpu,base,values)
        used40=(0,0x80000000,0xc0000000,0xe0000000)[seed%4]
        used_e0=(0,0x80000000,0xf0000000,0xffff0000)[seed%4]
        used80=(0,0x80000000,0xffffffff)[seed%3]
        for address,value in ((0x80098b20,used40),(0x800989cc,used_e0),(0x80098838,used80),(0x8009883c,0)):
            cpu.put(address,4,value)
        eye=[(seed*271+i*0x10203456)&0xffffffff for i in range(3)]
        rotation=[(seed*137+i*0x13579)&0xffffffff for i in range(3)]
        for i in range(3):cpu.put(0x800a39a0+i*4,4,eye[i]);cpu.put(0x800a3964+i*4,4,rotation[i])
        # An authored four-record EBD directory and authored resource tokens.
        # The original lookup scans it in source order without helper hooks.
        cpu.put(0x8016c000,4,4)
        for variant in range(4):
            entry=bytes((0xe0,3,variant,0))+struct.pack('<III',0x80180000+variant*128,0,0)
            fill(cpu,0x8016c004+variant*16,entry)
        for i in range(16):
            fill(cpu,0x8011ae0c+i*20,record(seed,i));cpu.put(0x8011b377+i,1,(i-1)%4 if i else 0)
        cpu.run(0x801010c4)
        assert cpu.lookup_misses==0 and not cpu.return_only_hooks
        assert cpu.get(WORK+4,1)==6
        row=[seed,read(cpu,WORK,168).hex(),read(cpu,0x800bc738,188).hex()]
        row += [read(cpu,b,c*w).hex() for b,c,w,f in POOLS]
        row += [cpu.get(a,4) for a in (0x80098b20,0x80098838,0x8009883c,0x800989cc)]
        row += [cpu.get(0x800a395c,1),cpu.get(0x80122428,1)]
        rows.append(row);steps+=cpu.steps;maximum=max(maximum,cpu.steps)
    fixture=('seed\twork\tcamera\tpool40\tpool80\tpool_e0\tused40\tused80low\tused80high\tused_e0\tmode\tbyte80122428\n'+
             ''.join('\t'.join(map(str,row))+'\n' for row in rows)).encode()
    camera_fixture=('seed\tposition\tx\ty\tz\trx\try\trz\n'+''.join('\t'.join(map(str,row))+'\n' for row in camera_rows)).encode()
    proof={'schema':'mml-source-event-phase5-probe/1','status':'SOURCE_BACKED_PARTIAL',
           'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,
           'constructor_cases':len(rows),'constructor_original_instructions':steps,'maximum_constructor_steps':maximum,
           'cold_pool_reset_cases':len(clear_rows),'cold_pool_reset_original_instructions':clear_steps,
           'camera_prefix_cases':len(camera_rows),'camera_prefix_original_instructions':camera_steps,
           'instruction_patches':0,'helper_hooks':0,'retail_parity':False,'hardware_tested':False,
           'fixture_sha256':sha(fixture),'camera_fixture_sha256':sha(camera_fixture),
           'constructor_sha256':sha(overlay[0xefc:0x102c]),
           'limits':['Inputs and resource tables are authored; these are original-instruction differential cases, not live retail traces.',
                     'Native profile cooking separately proves original ST00_00 selected resource identities.',
                     'Initial camera prefix proves field ownership only; complete original gameplay camera update remains unported.',
                     'Phase6 callback80100738/8001EC0C/8001F070 remains an explicit next consumer.']}
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'source-event-phase5.tsv').write_bytes(fixture)
    (args.out/'source-event-phase5-camera.tsv').write_bytes(camera_fixture)
    (args.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Unchanged camera8001EC0C/timeline8001F070 over authored neutral programs."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01425'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from source_camera_source_probe import Probe
from cook_actor_combat import OwnerExe,OWNER_SHA
def value(seed,i):return (seed*0x1020304+i*0x11bb77ee)&0xffffffff
def authored(seed,n):return bytes((seed*37+i*17+n*11)&255 for i in range(n))
def fill(cpu,address,data):
    for i,v in enumerate(data):cpu.put(address+i,1,v)
def read(cpu,address,n):return bytes(cpu.get(address+i,1) for i in range(n))
def program(seed):
    out=[0x03000000,0x04000000,0x05000000,0x06000000]
    for op in range(16,23):out.extend([(op<<24)|(3<<16)]+[value(seed,op*3+i) for i in range(3)])
    out.append((32+seed%3)<<24)
    op=seed%3
    out.append(seed+1 if op==0 else ((op<<24)|((1 if seed%2==0 else 2)<<16)|seed))
    return out+[0xffffffff]*(231-len(out))
def event(seed):
    out=bytearray(authored(seed,168));out[0]=1;out[1]=1;out[4]=6;out[8]=1;out[9]=1
    for offset in (0x14,0x18,0x1c):struct.pack_into('<I',out,offset,seed)
    struct.pack_into('<I',out,0xa4,0x8011b2e8+(seed%16)*8)
    return out
def timeline(seed):
    out=[]
    for i in range(18):out.append(((seed+i)%4|(((seed+i)%3)<<8)|(1<<16),0x80100738 if i==0 else 0))
    index=seed%16;deadlines=(0,1,32767,32768,65534,65535)
    out[index]=(out[index][0]&65535)|(deadlines[seed%6]<<16),out[index][1]
    out[-1]=(255,0)
    return out
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists():raise ValueError('Choose a NEW evidence directory')
    image=args.exe.read_bytes();owner=OwnerExe(image);rows=[];timelines=[];steps=timeline_steps=0
    for seed in range(64):
        cpu=Probe(image,owner.base);cpu.r[29]=0x700000
        fill(cpu,0x800c4c48,event(seed));fill(cpu,0x800bc738,authored(seed,188))
        words=program(seed);fill(cpu,0x8011af4c,struct.pack('<231I',*words));cpu.put(0x800bc7f0,4,0x8011af4c)
        distance=(0,1,3276,3277,6553,6554,13106,13107,32768,65535)[seed%10]
        cpu.put(0x800bc81c,2,distance);cpu.put(0x800bc733,1,seed)
        cpu.put(0x1f800074,1,seed);cpu.put(0x1f800006,2,seed+100);cpu.put(0x1f800076,2,seed+200)
        cpu.control2[24:29]=[value(seed,i) for i in range(5)]
        cpu.run(0x8001ec0c);steps+=cpu.steps;assert not cpu.return_only_hooks
        rows.append([seed,distance,read(cpu,0x800bc738,188).hex(),cpu.get(0x800bc733,1),cpu.get(0x1f800074,1),
                     cpu.get(0x1f800006,2),cpu.get(0x1f800076,2),*cpu.control2[24:29]])
        cpu=Probe(image,owner.base);cpu.r[29]=0x700000;work=event(seed)
        # Authored pre-counter pairs exercise exact signed16 deadlines,
        # selector handoff/reset and low32 rollover.
        deadline=(0,1,32767,32768,65534,65535)[seed%6]
        before=(deadline-1)&0xffffffff if seed%2==0 else value(seed,91)
        struct.pack_into('<I',work,0x10,before)
        fill(cpu,0x800c4c48,work);fill(cpu,0x8011b2e8,b''.join(struct.pack('<II',*record) for record in timeline(seed)))
        cpu.run(0x8001f070);timeline_steps+=cpu.steps;assert not cpu.return_only_hooks
        timelines.append([seed,before,read(cpu,0x800c4c48,168).hex()])
    args.out.mkdir(parents=True,exist_ok=False)
    hashes={}
    for name,records,header in [('source-event-camera.tsv',rows,'seed distance camera byte733 shift half006 half076 ofx ofy h dqa dqb'),
                                ('source-event-timeline.tsv',timelines,'seed before work')]:
        data=(header+'\n'+''.join(' '.join(map(str,row))+'\n' for row in records)).encode();(args.out/name).write_bytes(data)
        hashes[name]=hashlib.sha256(data).hexdigest()
    proof={'schema':'mml-source-event-camera-timeline/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,
           'camera_cases':64,'camera_original_instructions':steps,'timeline_cases':64,'timeline_original_instructions':timeline_steps,
           'instruction_patches':0,'helper_hooks':0,'fixture_sha256':hashes,'retail_parity':False,'hardware_tested':False,
           'limits':['Authored programs and owners are differential tests, not live retail scene traces.',
                     'GTE control transfers are interpreted; no hardware GPU/camera cadence parity.',
                     'Original caller, color tail, completion predicate, later callback and actual camera application remain separate owners.']}
    (args.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

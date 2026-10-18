#!/usr/bin/env python3
"""Unchanged8010102C/80100738 oracle with authored retained owners.

No instruction patches or helper hooks. This proves one callee and its first
callback, not a live scene update or8001EC0C/8001F070 execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources

WORK=0x800c4c48
def authored(seed,n):return bytes((seed*37+i*17+n*11)&255 for i in range(n))
def fill(cpu,address,data):
    for i,value in enumerate(data):cpu.put(address+i,1,value)
def read(cpu,address,size):return bytes(cpu.get(address+i,1) for i in range(size))
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','st00','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists():raise ValueError('Choose a NEW evidence directory')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    if hashlib.sha256(overlay).hexdigest()!=OVERLAY_SHA:raise ValueError('Unsupported overlay identity')
    assert overlay[0x22424]==0,'Cold source overlay control premise changed'
    rows=[];instructions=0
    frames=(0,1,424,425,426,464,465,466,515)
    for seed in range(54):
        cpu=OverlayProbe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[4]=WORK
        frame=frames[seed%len(frames)];control=seed//9%3;input_word=(0,2,65535)[seed//3%3]
        enabled=seed<45
        selector=(0,1,2,254)[seed%4] if enabled else 255
        work=bytearray(authored(seed,168));work[0]=1;work[1]=1;work[4]=6;work[8]=selector
        fill(cpu,WORK,work);cpu.put(WORK+0x10,4,frame);cpu.put(WORK+0xa4,4,0x8011b2e8)
        slot40=seed%4;slot_e0=seed%32;actor40=0x800c3580+slot40*1444;actor_e0=0x800bf118+slot_e0*80
        cpu.put(WORK+0x24,4,actor40);cpu.put(WORK+0x30,4,actor_e0)
        fill(cpu,actor40,authored(seed,1444));fill(cpu,actor_e0,authored(seed,80))
        cpu.put(actor40+1,1,0x40|slot40);cpu.put(actor_e0+1,1,0xe0|slot_e0)
        fill(cpu,0x800bc738,authored(seed,188));fill(cpu,0x80122424,authored(seed,21))
        cpu.put(0x80122424,1,control);cpu.put(0x1f800002,2,input_word)
        before=read(cpu,WORK,168);cpu.run(0x8010102c)
        assert before==read(cpu,WORK,168),'Prefix unexpectedly wrote source event work'
        assert not cpu.return_only_hooks
        if enabled:assert any(call['target']=='0x80100738' for call in cpu.indirect)
        instructions+=cpu.steps
        rows.append([seed,frame,control,input_word,selector,read(cpu,actor40,1444).hex(),
                     read(cpu,actor_e0,80).hex(),read(cpu,0x800bc738,188).hex(),read(cpu,0x80122424,21).hex()])
    fixture=('seed\tframe\tcontrol24\tinput\tselector08\tactor40\tactorE0\tcamera\tcontrols\n'+
             ''.join('\t'.join(map(str,row))+'\n' for row in rows)).encode()
    proof={'schema':'mml-source-event-phase6-prefix/1','status':'SOURCE_BACKED_PARTIAL',
           'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'cases':len(rows),
           'unchanged_original_instructions':instructions,'instruction_patches':0,'helper_hooks':0,
           'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'retail_parity':False,'hardware_tested':False,
           'cold_overlay_control_80122424':0,'first_callback':'80100738','next_unconsumed_owner':'8001EC0C',
           'callback_selector08_cases':[0,1,2,254,255],
           'limits':['Authored inputs are not live retail traces.',
                     'No camera command consumer, scene-counter chronology, progression or scene completion is acknowledged.',
                     'Other table callbacks remain unsupported; raw action/pose IDs keep original offset labels.']}
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'source-event-phase6-prefix.tsv').write_bytes(fixture)
    (args.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

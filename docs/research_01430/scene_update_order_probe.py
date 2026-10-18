#!/usr/bin/env python3
"""Prove bounded source caller order and the main-phase0 busy gate.

Calls are stopped at their actual source entries. No helper is replaced with a
return, no full frame completion is inferred, and no game payload is exported.
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

MAIN=0x80200000
def fill(cpu,address,seed,n):
    for i in range(n):cpu.put(address+i,1,(seed*29+i*17+11)&255)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('exe','st00','out'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists() or SOURCE==args.out.resolve() or SOURCE in args.out.resolve().parents:
        raise ValueError('Choose a new evidence directory outside source')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(overlay).hexdigest()==OVERLAY_SHA
    total=0;rows=[]
    for seed in range(12):
        cpu=OverlayProbe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[16]=MAIN;cpu.r[17]=0x800c3558
        fill(cpu,MAIN,seed,256);cpu.run(0x800158c8,0x800158d8)
        assert cpu.get(MAIN,1)==3 and cpu.get(MAIN+1,1)==0
        total+=cpu.steps
        # Exact main update prefix reaches event processing before dispatching
        # the main phase. This does not acknowledge event execution itself.
        cpu=OverlayProbe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[4]=MAIN
        cpu.run(0x800158f0,0x8001d990)
        assert cpu.calls[-1]=={'site':'0x80015904','target':'0x8001d990'}
        total+=cpu.steps
        cpu=OverlayProbe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[16]=MAIN;cpu.r[17]=0x800b51b0;cpu.r[18]=0x800c3558
        fill(cpu,MAIN,seed,256);cpu.put(MAIN+1,1,0);cpu.put(0x800c3570,1,255)
        busy=(0,1,0x8000,0xffff)[seed%4];cpu.put(0x80098a72,2,busy)
        cpu.run(0x8001594c,0x80015db4 if busy==0 else None)
        assert cpu.get(MAIN+1,1)==(2 if busy==0 else 0)
        assert any(call['target']=='0x80015db4' for call in cpu.calls)==(busy==0)
        assert not cpu.return_only_hooks
        rows.append({'seed':seed,'busy_bits':busy,'world_update_reached':busy==0,'phase1_after':cpu.get(MAIN+1,1)})
        total+=cpu.steps
        # The exact world helper call site reaches the original E0 sweep. All
        # preceding world helpers remain outside this bounded call-site proof.
        cpu=OverlayProbe(image,owner.base,overlay);cpu.run(0x80015e0c,0x80017528)
        assert cpu.calls==[{'site':'0x80015e0c','target':'0x80017528'}]
        total+=cpu.steps
    proof={'schema':'mml-source-event-world-order/1','status':'SOURCE_BACKED_PARTIAL',
           'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'cases':len(rows),
           'unchanged_original_instructions':total,'instruction_patches':0,'helper_hooks':0,
           'fresh_main_phase_owner':'800158C8..800158D8 stores main+00=3, main+01=0',
           'event_call':{'caller':'800158F0','site':'80015904','target':'8001D990'},
           'e0_call':{'caller':'80015DB4','site':'80015E0C','target':'80017528'},
           'main_phase0_gate':rows,'native_diagnostic_service_chronology':'CANDIDATE',
           'limits':['Main phase0 with nonzero busy does not reach the world sweep.',
                     'Busy0 samples use authored transition byte800C3570=255 to avoid an unowned fade call.',
                     'Calls stop at actual consumer entries; they are not acknowledged or hooked.',
                     'All earlier world helpers and full native main-phase ownership remain unproved.',
                     'This proves source ordering, not retail-vs-native frame timing.']}
    args.out.mkdir(parents=True);(args.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

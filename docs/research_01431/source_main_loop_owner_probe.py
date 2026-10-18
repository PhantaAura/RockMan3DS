#!/usr/bin/env python3
"""Unchanged C0reset and shared scratch countertail oracle; no retail payload export."""
import argparse,hashlib,json,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources
from source_event_callbacks_probe import authored,fill,read

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','st00','out'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args()
 if a.out.exists():raise ValueError('Choose new private evidence directory')
 image=a.exe.read_bytes();owner=OwnerExe(image)
 overlay=next(r['payload'] for r in iter_resources(a.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
 if hashlib.sha256(overlay).hexdigest()!=OVERLAY_SHA:raise ValueError('Unsupported owner')
 clear_steps=tail_steps=0;rows=[]
 for seed in range(8):
  c=OverlayProbe(image,owner.base,overlay);c.r[29]=0x700000
  fill(c,0x800a3b40,authored(seed,64*72));fill(c,0x800a2d08,authored(seed,16*36))
  c.put(0x800987ec,4,0xabcdef01);c.put(0x80098818,4,0xabcdef02)
  c.run(0x8002e924)
  expected=bytearray(16*36)
  for slot in range(16):expected[slot*36+1]=0xc0|slot
  assert read(c,0x800a2d08,16*36)==expected and c.get(0x800987ec,4)==0 and c.get(0x80098818,4)==0
  assert not c.return_only_hooks;clear_steps+=c.steps
 for before in [0,1,2,3,7,8,255,256,32767,32768,65533,65534,65535]:
  c=OverlayProbe(image,owner.base,overlay);c.r[29]=0x700000;c.put(0x1f800002,2,before)
  c.run(0x80011ecc,0x80011ce8);after=c.get(0x1f800002,2)
  assert after==(before+1)&65535 and not c.return_only_hooks
  rows.append([before,after]);tail_steps+=c.steps
 fixture=('before after\n'+''.join(f'{a} {b}\n' for a,b in rows)).encode()
 proof={'schema':'source-main-loop-owner/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,
        'overlay_sha256':OVERLAY_SHA,'c0_reset_cases':8,'c0_reset_original_instructions':clear_steps,
        'scratch_counter_tail_cases':len(rows),'scratch_counter_tail_original_instructions':tail_steps,
        'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'helper_hooks':0,'instruction_patches':0,'retail_parity':False,'hardware_tested':False,
        'scratch_half_1f800002_initial_owner':'UNKNOWN',
        'limits':['The counter tail is proved; its initial value is not fabricated from BSS, native frame count or buttons.',
                  'Original main loop invokes events/objects/render then this increment; native scheduler parity remains CANDIDATE.',
                  'The C0 reset is a cold/reset lifecycle action; it is not a warm-load fallback.']}
 a.out.mkdir(parents=True,exist_ok=False);(a.out/'source-main-loop-tail.tsv').write_bytes(fixture);(a.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

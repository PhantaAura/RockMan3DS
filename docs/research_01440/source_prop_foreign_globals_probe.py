#!/usr/bin/env python3
"""Bounded foreign-global initial-word/capped-writer proof; no owner export."""
import argparse,hashlib,json,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('exe','overlay','out'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();assert not a.out.exists() and SOURCE not in a.out.resolve().parents
    image=a.exe.read_bytes();owner=OwnerExe(image);overlay=a.overlay.read_bytes();assert sha(overlay)==OVERLAY_SHA
    assert owner.word(0x800988fc)==0 and owner.word(0x8009898c)==0
    cases=[];steps=0
    for function,advance in [(0x8002d464,40),(0x8002d4b8,32)]:
        for count in (0,1,254,255,256,257,0x80000000,0xffffffff):
            c=OverlayProbe(image,owner.base,overlay);c.r[29]=0x700000
            c.put(0x800988fc,4,count);c.put(0x1f800070,4,0x80160000)
            c.run(function);expected=count+1 if count<256 else count
            assert c.get(0x800988fc,4)==expected and c.r[2]==(0x80160000 if count<256 else 0)
            assert c.get(0x1f800070,4)==(0x80160000+advance if count<256 else 0x80160000)
            assert not c.return_only_hooks;steps+=c.steps;cases.append({'function':hex(function),'before':count,'after':expected})
    for start,end in [(0x8002cfe4,0x8002cfec),(0x8002d574,0x8002d57c),(0x8002d9b4,0x8002d9bc)]:
        c=OverlayProbe(image,owner.base,overlay);c.put(0x800988fc,4,0xa5a5a5a5);c.run(start,end)
        assert c.get(0x800988fc,4)==0 and not c.return_only_hooks;steps+=c.steps
    proof={'schema':'mml-source-foreign-prop-globals/1','status':'SOURCE_BACKED_PARTIAL',
        'exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'initial_loaded_words':{'800988FC':0,'8009898C':0},
        'capped_enqueuer_cases':len(cases),'reset_slices':3,'unchanged_original_instructions':steps,
        'instruction_patches':0,'helper_hooks':0,'capped_writer_limit':256,
        'unchecked_tutorial_writer':'80112648 in E0/subtype1 callback8011216C',
        'selected_bound_premise':'Fresh supportedEXEload, all encountered occupiedE0 roster entries known and subtype1 excluded; unknown/presence permanentlyrevokes bound.',
        'used_counter_known_zero_mask':'FFFFFE00','used_counter_exact_value_supplied':False,
        'pending_after_selected_aux':'(0|08000000)&count0..256 =0',
        'generic_E0_free':False,'native_hardware_tested':False,'retail_parity':False,
        'limits':['Only supportedresidentEXE/tutorial codeprofile is audited; otheroverlays/warmrestore/untrackedwriters mustinvalidate ownership.',
            'Cap preserves originalvalidrange; it doesnot normalize arbitrarypriorout-of-rangevalues.',
            'Exactqueuecount and packetdraw remain unknown; this supplies onlyprovedbits and the resultingoriginalAND sideeffect.',
            'Loadedwordzeros are originalsourceimage facts, not a forced scene snapshot or per-area reset.',
            'No code, model, texture, ownerpayload, originalsave or newreferencecapture exported.']}
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({k:proof[k] for k in ('capped_enqueuer_cases','reset_slices','unchanged_original_instructions','instruction_patches','helper_hooks')}))
if __name__=='__main__':main()

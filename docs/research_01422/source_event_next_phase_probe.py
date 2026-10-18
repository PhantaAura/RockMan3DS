#!/usr/bin/env python3
"""Bounded unchanged phase3 predicate source differential, authored raw status.

No camera/command completion is inferred. It measures only8001D878 and the
event phase3 stores when explicit raw80098AB8 bit20 is present.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'dat', 'fixture', 'proof'):
        ap.add_argument('--' + name, type=Path, required=True)
    a = ap.parse_args()
    if a.fixture.exists() or a.proof.exists(): raise SystemExit('Refusing to overwrite phase3 evidence')
    image=a.exe.read_bytes();base=struct.unpack_from('<I',image,24)[0]
    sha=lambda blob:hashlib.sha256(blob).hexdigest()
    assert sha(image)==SUPPORTED_EXE
    overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay)==OVERLAY_SHA
    rows=[];steps=0
    for status in range(256):
        c=OverlayProbe(image,base,overlay);c.r[29]=0x700000;c.r[4]=0x800C4C48
        before=bytearray((i*37+status*11)&255 for i in range(168));before[0]=1;before[1]=1;before[4]=3
        for i,v in enumerate(before): c.put(0x800C4C48+i,1,v)
        c.put(0x80098AB8,1,status);c.run(0x801010C4)
        assert not c.return_only_hooks
        after=bytes(c.get(0x800C4C48+i,1) for i in range(168))
        expected=before[:]
        if status&32:expected[4]=4;struct.pack_into('<I',expected,12,55)
        assert after==expected
        rows.append([status,after.hex()]);steps+=c.steps
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text('raw80098ab8\tactual_work\n'+''.join('\t'.join(map(str,r))+'\n' for r in rows))
    proof={'schema':'mml-source-event-phase3/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),'overlay_sha256':sha(overlay),
           'cases':len(rows),'original_instructions_executed':steps,'accepted_cases_helper_hooks':[],
           'fixture_sha256':sha(a.fixture.read_bytes()),
           'predicate_region':{'start':'0x8001D878','end_exclusive':'0x8001D888','sha256':sha(image[2048+0x8001D878-base:2048+0x8001D888-base])},
           'semantics':'Explicit raw80098AB8 bit20 gates phase4 and word+0C=55; bitclear retains work.',
           'limits':['Status byte is authored; camera/command/message completion and native writers are not established.',
                     'Phase2 consumer8001B33C and packet producer8001D58C are not skipped.',
                     'No whole scene, native progression, audible playback or hardware parity claim.'],
           'retail_parity':False,'hardware_tested':False}
    a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({'cases':len(rows),'original_instructions_executed':steps}))


if __name__=='__main__':main()

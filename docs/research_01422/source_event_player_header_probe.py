#!/usr/bin/env python3
"""Original fresh NewGame byte store and player header; authored input only."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','fixture','proof'):ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args()
    if a.fixture.exists() or a.proof.exists():raise SystemExit('Refusing to overwrite fresh header evidence')
    image=a.exe.read_bytes();sha=lambda b:hashlib.sha256(b).hexdigest();assert sha(image)==SUPPORTED_EXE
    base=struct.unpack_from('<I',image,24)[0];rows=[];steps=0;player=0x800B51B0
    for seed in range(24):
        c=OverlayProbe(image,base,b'');c.r[29]=0x700000;c.r[4]=player;c.r[5]=0
        before=bytes((i*37+seed*11)&255 for i in range(0x5F4))
        for i,v in enumerate(before):c.put(player+i,1,v)
        c.put(0x800C1B92,1,(seed*17+128)&255)
        c.run(0x80013C78,0x80013C80)
        assert c.get(0x800C1B92,1)==0
        c.run(0x8003BE6C,0x8003BED4)
        assert not c.return_only_hooks
        after=bytes(c.get(player+i,1) for i in range(0x5F4));expected=bytearray(before)
        expected[0]=expected[6]=3;expected[2:6]=bytes(4);expected[8:16]=bytes(8)
        assert after==expected;rows.append([seed,after.hex()]);steps+=c.steps
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('seed\tactual_player\n'+''.join('\t'.join(map(str,r))+'\n' for r in rows))
    proof={'schema':'mml-source-event-fresh-player-header/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),
           'cases':len(rows),'original_instructions_executed':steps,'accepted_cases_helper_hooks':[],
           'fixture_sha256':sha(a.fixture.read_bytes()),'fresh_byte_writer':'NewGame80013C7C writes raw800C1B92=0 before callback800156A4.',
           'regions':[{'start':hex(lo),'end_exclusive':hex(hi),'sha256':sha(image[2048+lo-base:2048+hi-base])} for lo,hi in ((0x80013C78,0x80013C80),(0x8003BE6C,0x8003BED4))],
           'limits':['Two explicit original prefixes execute; no claim of complete NewGame or original callback scheduling.',
                     'Continuation/statusbit80 branch is not admitted as fresh state.',
                     'Player01/07 and every field outside header stores remain unknown unless separately initialized.'],
           'retail_parity':False,'hardware_tested':False}
    a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({'cases':len(rows),'original_instructions_executed':steps}))


if __name__=='__main__':main()

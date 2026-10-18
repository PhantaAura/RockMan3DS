#!/usr/bin/env python3
"""Always-taken player initializer zero-store prefix; authored work only."""
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
    if a.fixture.exists() or a.proof.exists():raise SystemExit('Refusing to overwrite player initializer evidence')
    image=a.exe.read_bytes();sha=lambda b:hashlib.sha256(b).hexdigest();assert sha(image)==SUPPORTED_EXE
    base=struct.unpack_from('<I',image,24)[0];rows=[];steps=0;player=0x800B51B0
    for seed in range(16):
        c=OverlayProbe(image,base,b'');c.r[29]=0x700000;c.r[16]=player;c.r[2]=player
        before=bytes((i*37+seed*11)&255 for i in range(0x5F4))
        for i,v in enumerate(before):c.put(player+i,1,v)
        c.run(0x8003BEE8,0x8003BF10)
        assert not c.return_only_hooks
        after=bytes(c.get(player+i,1) for i in range(0x5F4));expected=bytearray(before)
        for i in (0xBD,0x74,0x75,0x76,0x77,0x184,0x185,0x186,0x187,0x110,0x111,0x449):expected[i]=0
        assert after==expected;rows.append([seed,after.hex()]);steps+=c.steps
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('seed\tactual_player\n'+''.join('\t'.join(map(str,r))+'\n' for r in rows))
    proof={'schema':'mml-source-event-player-initializer/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),
           'cases':len(rows),'original_instructions_executed':steps,'accepted_cases_helper_hooks':[],
           'fixture_sha256':sha(a.fixture.read_bytes()),'source_owner':'Player initializer8003BE6C always reaches8003BEE8 before fresh/continued branch8003BF38.',
           'prefix_region':{'start':'0x8003BEE8','end_exclusive':'0x8003BF10','sha256':sha(image[2048+0x8003BEE8-base:2048+0x8003BF10-base])},
           'limits':['Only source player zero-store prefix is admitted; native must explicitly invoke it during player initialization.',
                     'Remaining player work, later pose/render/control helpers and whole NewGame ordering are not acknowledged.'],
           'retail_parity':False,'hardware_tested':False}
    a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({'cases':len(rows),'original_instructions_executed':steps}))


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Read-only source E0 first-initializer oracle using authored80-byte work.

No original instructions/assets are exported. The differential fixture contains
only authored work. Separate owner-derived geometry is tested locally by the
native decoder; this does not claim full frame chronology or GTE parity.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources

def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ['exe','st00','out']:parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists() or SOURCE in args.out.resolve().parents:raise ValueError('Choose a NEW evidence directory outside source')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay)!=OVERLAY_SHA:raise ValueError('Unsupported owner overlay')
    fixtures=[];steps=0
    for seed in range(4):
        for variant in range(8):
            before=bytearray((seed*37+i*19+variant*13)&255 for i in range(80))
            before[0]=1;before[1]=0xe0;before[2]=3;before[3]=variant;before[6]=0x23;before[8]=0
            struct.pack_into('<I',before,20,0x8016c100)
            cpu=OverlayProbe(image,owner.base,overlay);cpu.r[4]=0x5000;cpu.r[29]=0x700000
            for i,v in enumerate(before):cpu.put(0x5000+i,1,v)
            cpu.run(0x80112bdc);steps+=cpu.steps
            after=bytes(cpu.get(0x5000+i,1) for i in range(80))
            assert after[0]==(15 if variant==0 else 7) and after[8]==1
            assert struct.unpack_from('<8I',after,24)==(4096,0,4096,0,4096,0,0,0)
            assert not cpu.return_only_hooks
            fixtures.append(f'{seed}\t{variant}\t{before.hex()}\t{after.hex()}\n')
    fixture=('seed\tvariant\tbefore\tafter\n'+''.join(fixtures)).encode()
    proof={'schema':'mml-source-scene-e0-owner/1','status':'SOURCE_BACKED_PARTIAL',
           'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,
           'authored_initializer_cases':len(fixtures),'unchanged_original_instructions':steps,
           'initializer_slice_sha256':sha(overlay[0x12bdc:0x12c70]),
           'instruction_patches':0,'helper_hooks':0,'fixture_sha256':sha(fixture),
           'retail_parity':False,'hardware_tested':False,
           'limits':['Only the first initialization owner is exercised; later variant callbacks are explicit holds.',
                     'Authored work is not a live reference scene or proof of complete actor update chronology.',
                     'GTE draw, depth shading, dynamic part selection and blend order are not proved by this oracle.']}
    args.out.mkdir(parents=True)
    (args.out/'scene-e0-initializer.tsv').write_bytes(fixture)
    (args.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

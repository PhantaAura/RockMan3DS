#!/usr/bin/env python3
"""Unchanged E0 scalar packet construction, no GTE/raster claim or retail output."""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','out'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();assert not a.out.exists() and SOURCE not in a.out.resolve().parents
    image=a.exe.read_bytes();owner=OwnerExe(image);rng=random.Random(0x1439);rows=[];steps=0
    for case in range(256):
        modulation=rng.getrandbits(32);material=rng.getrandbits(32)
        if case<8:modulation=(0,0x00808080,0x02808080,0xffffffff,0x01808080,0x10808080,0x40808080,0xfd806040)[case]
        cpu=OverlayProbe(image,owner.base,b'');cpu.r[29]=0x700000;cpu.r[8]=0x1000
        cpu.put(0x1000+44,4,modulation);cpu.put(0x1f8000e8,4,0x2000)
        cpu.run(0x8002a6b8,0x8002a6d0);assert cpu.r[5]==modulation&0x02ffffff and cpu.r[4]==0x2000
        cpu.put(0x2000+12,4,material)
        cpu.run(0x8002a6e8,0x8002a738);triangle=cpu.get(cpu.r[29],4)
        cpu.run(0x8002a908,0x8002a920);quad=cpu.get(cpu.r[29],4)
        assert triangle==(modulation&0x02ffffff)|0x24000000
        assert quad==(modulation&0x02ffffff)|0x2c000000
        # Real UV material inserts. OriginalCLUT also ORs scratch1F8000E0;
        # retain this explicit authored input rather than claim a livezero.
        uv0,uv1,uv2=[rng.randrange(65536) for _ in range(3)];scratch=rng.getrandbits(32)
        cpu.r[4]=0x4000;cpu.r[9]=0x3000;cpu.r[7]=0x3004
        cpu.r[17]=material&0xffff0000;cpu.r[16]=(material<<16)&0xffffffff
        cpu.put(0x3000,2,uv0);cpu.put(0x3002,2,uv1);cpu.put(0x3004,2,uv2);cpu.put(0x1f8000e0,4,scratch)
        cpu.run(0x8002a8c8,0x8002a8fc)
        assert cpu.get(0x400c,4)==(uv0|(material&0xffff0000)|scratch)
        tpage=cpu.get(0x4014,4);assert tpage==(uv1|((material<<16)&0xffffffff))
        assert cpu.get(0x401c,4)==uv2
        assert not cpu.return_only_hooks
        rows.append((modulation,material,triangle,quad,tpage));steps+=cpu.steps
    ordering_cases=0
    for depth in (0,1,2,3,48,700,1800,65535):
        for length in (7,9):
            cpu=OverlayProbe(image,owner.base,b'');cpu.r[29]=0x700000;cpu.r[18]=0x1000
            cpu.put(0x1000,4,0x2000);cpu.put(0x700008,4,depth)
            bucket=0x2000+(depth>>1)*4;old=rng.getrandbits(24);cpu.put(bucket,4,old)
            cpu.r[24]=length<<24;cpu.r[25]=0xffffff
            for packet in (0x80140000,0x80140040):
                cpu.r[4]=packet;cpu.run(0x8002a880,0x8002a8ac)
                assert cpu.get(packet,4)==(old|(length<<24))
                assert cpu.get(bucket,4)==(packet&0xffffff)
                old=packet&0xffffff
            assert not cpu.return_only_hooks;steps+=cpu.steps;ordering_cases+=1
    fixture=('modulation material triangle quad tpage_uv\n'+''.join(' '.join(map(str,r))+'\n' for r in rows)).encode()
    proof={'schema':'mml-source-e0-material/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,
        'cases':len(rows),'ordering_scalar_cases':ordering_cases,'unchanged_original_instructions':steps,'instruction_patches':0,'helper_hooks':0,
        'fixture_sha256':sha(fixture),'fixture_authored_only':True,
        'source_slices':['8002A6B8..8002A6D0','8002A6E8..8002A738','8002A908..8002A920','8002A8C8..8002A8FC','8002A880..8002A8AC'],
        'ordering_insertion':'bucket=(authored_source_depth>>1), prepend packet to existing24bitlink; samebucketLIFO',
        'gte_executed':False,'gpu_executed':False,'hardware_tested':False,'retail_parity':False,
        'limits':['Authored inputs prove exact originalscalar packet construction/material inserts, not complete renderer execution.',
            'CLUTword also ORs current source scratch1F8000E0; no live owner/value is established.',
            'GTE projection/culling/depthshade/OTinsertion/command submission are excluded.',
            'PICAblend and8bitconstantfactor quantization remain CANDIDATE, not PS1 rasterparity.',
            'Only authored fixture fields, aggregate scalar counts and source hashes are exported; no owner bytes/assets.']}
    a.out.mkdir(parents=True);(a.out/'source-e0-material.tsv').write_bytes(fixture)
    (a.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({k:proof[k] for k in ('cases','unchanged_original_instructions','instruction_patches','helper_hooks')}))
if __name__=='__main__':main()

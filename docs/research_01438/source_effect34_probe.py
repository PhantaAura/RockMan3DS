#!/usr/bin/env python3
"""Unchanged owner callbacks3/4 + selected BIOS oracle, outputs owner-local."""
import argparse, hashlib, json, struct, sys, zlib
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA
BIOS_SHA='713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
BASE,RAND,BIOS_RAND,STATE=0x800a3b40,0x8007fe50,0xbfc060e4,0x8448
def sha(b):return hashlib.sha256(b).hexdigest()
def fill(c,a,b):
    for i,v in enumerate(b):c.put(a+i,1,v)
def read(c,a,n):return bytes(c.get(a+i,1) for i in range(n))
def authored(seed,n):return bytes((seed*37+i*17+11)&255 for i in range(n))
class Effect34Probe(OverlayProbe):
    # Original4's24 full-pool attempts exceed inherited per-run10K budget.
    # Reset only the diagnostic counter at the unchanged allocator entry;
    # record its accumulated total and retain a100K whole-callback bound.
    # No fetched instruction, helper result or emulated register is altered.
    accumulated_steps=0
    def get(self,address,count,signed=False):
        if address==0x80031618 and count==4:
            self.accumulated_steps+=self.steps;self.steps=0
            if self.accumulated_steps>=100000:raise ValueError('Whole callback instruction budget')
        return super().get(address,count,signed)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','overlay','bios','out'):p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args();assert not a.out.exists(),'Refusing to overwrite oracle'
    image,overlay,bios=a.exe.read_bytes(),a.overlay.read_bytes(),a.bios.read_bytes()
    owner=OwnerExe(image);assert sha(overlay)==OVERLAY_SHA and sha(bios)==BIOS_SHA
    specs=[]
    # Parent creation/idle/clock wrap/full capacity; low/high bank and sparse free.
    patterns=[(0x80000000,0),(0xffffffff,0),(0xffffffff,0xffffffff),
              (0xfffffffe,0xffffffff),(0xfffeffff,0xfffffff7),(0x80000000,0xfffffff8)]
    for subtype in (3,4):
        for clock in (0,1,3,4,5,252,255):
            for used0,used1 in patterns:
                for tag in (0,1,255):specs.append((subtype,0,0,0,clock,0,tag,0,used0,used1))
    # Child sequence progression/release, modulo byte/halfword edge values.
    for phase in range(10):
        for clock in (0,1,255):specs.append((3,1,0,0,clock,phase,0,0,0x80000000,0))
    for phase in range(4):
        for clock in (0,1,2,255):
            for direction in (0,7,128,255):specs.append((4,1,0,0,clock,phase,0,direction,0x80000000,0))
    for subtype in (3,4):
        for mode in (1,128,255):specs.append((subtype,mode,0,1,0,0,0,0,0x80000000,0))
    for parameter in (1,2):specs.append((3,0,parameter,0,0,0,0,0,0x80000000,0))
    rows=[];instructions=0;services=0;maximum=0
    for case,spec in enumerate(specs):
        subtype,mode,param,state,clock,phase,tag,direction,used0,used1=spec
        c=Effect34Probe(image,owner.base,overlay);c.r[29]=0x700000;c.r[4]=BASE
        c.mem.update({0xbfc00000+i:v for i,v in enumerate(bios)})
        pool=bytearray(authored(case,64*72))
        for slot in range(64):pool[slot*72+1]=0x80|slot
        pool[0]=3;pool[2]=subtype;pool[3]=param;pool[6]=3;pool[8]=state;pool[9]=clock;pool[10]=phase;pool[13]=mode;pool[14]=direction;pool[15]=tag
        fill(c,BASE,pool);c.put(0x80098838,4,used0);c.put(0x8009883c,4,used1)
        c.put(0x800988c8,4,0);c.put(0x800988cc,4,0)
        seed=(case*0x1234567)&0xffffffff;c.put(STATE,4,seed)
        calls=[];pc=0x801172fc if subtype==3 else 0x80117558
        while True:
            c.run(pc,RAND)
            if c.r[31]==0:break
            ra=c.r[31];child=c.r[16]
            assert BASE<=child<BASE+4608 and (child-BASE)%72==0
            before=c.get(STATE,4);c.run(RAND,0xa0);assert c.r[9]==47
            c.run(BIOS_RAND,ra)
            calls.append((ra,c.r[2],child,1001+len(calls)));pc=ra
        assert not c.return_only_hooks
        instructions+=c.accumulated_steps+c.steps;services+=len(calls);maximum=max(maximum,len(calls))
        result=read(c,BASE,4608)
        changes=','.join(f'{i}:{v}' for i,(old,v) in enumerate(zip(pool,result)) if old!=v) or '-'
        receipts=','.join(':'.join(map(str,r)) for r in calls) or '-'
        rows.append((case,*spec,seed,receipts,changes,c.get(0x80098838,4),c.get(0x8009883c,4),c.get(0x800988c8,4),c.get(0x800988cc,4),zlib.crc32(result)))
    fixture=('case subtype mode param state clock phase tag direction used0 used1 seed receipts changes out0 out1 pending0 pending1 crc\n'+''.join(' '.join(map(str,r))+'\n' for r in rows)).encode()
    controls=overlay[0x1cda8:0x1cdbe]+overlay[0x1cdcc:0x1cde0]
    proof={'schema':'mml-source-effects34-oracle/1','status':'SOURCE_BACKED_PARTIAL',
        'exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'selected_bios_sha256':BIOS_SHA,
        'callbacks':['801172FC','80117558'],'cases':len(rows),'unchanged_original_instructions':instructions,
        'selected_bios_calls':services,'maximum_rng_per_callback':maximum,'instruction_patches':0,'helper_hooks':0,
        'controls_sha256':sha(controls),'private_fixture_sha256':sha(fixture),'fixture_owner_local':True,
        'native_hardware':False,'retail_parity':False,
        'limits':['All initial pool records, masks and selected-BIOS seeds are authored original-instruction differential inputs.',
            'Original callback, allocator, release and selected-BIOS rand execute unchanged at declared A0 service boundaries; outer scheduler/BIOS dispatcher excluded.',
            'Neither passive original parent context nor source-global competing-consumer RNG chronology is established.',
            'Rendering, original primitive submission and physicalOld3DS remain untested.']}
    a.out.mkdir(parents=True);(a.out/'owner-controls.bin').write_bytes(controls)
    (a.out/'effects34.tsv').write_bytes(fixture);(a.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({k:proof[k] for k in ('cases','unchanged_original_instructions','selected_bios_calls','maximum_rng_per_callback','instruction_patches','helper_hooks')}))
if __name__=='__main__':main()

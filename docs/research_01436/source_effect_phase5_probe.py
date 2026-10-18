#!/usr/bin/env python3
"""Unchanged family80 sweep/read lanes/caller cleanup on authored scalar owners.

No code patches or helper hooks. Fixtures contain authored bytes and source
scalar results only; private disassembly/table/owner payloads are not exported.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01422'),str(SOURCE/'docs/research_01417')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from class60_06_source_probe import FastProbe
FX,ALLOC,PENDING,GP=0x800a3b40,0x80098838,0x800988c8,0x80097864

class Probe(FastProbe):
    def __init__(self,image,base):
        super().__init__(image,base,b'');self.r[29]=0x700000;self.r[28]=GP
        self.reads=set();self.writes=set();self.track=False;self.visits=[]
    def get(self,address,count,signed=False):
        if self.track and FX<=address<FX+72:self.reads.update(range(address-FX,address-FX+count))
        if address==0x80017434:self.visits.append((self.r[4]-FX)//72)
        return super().get(address,count,signed)
    def put(self,address,count,value):
        if self.track and FX<=address<FX+72:self.writes.update(range(address-FX,address-FX+count))
        return super().put(address,count,value)

def blob(cpu,address,count):return bytes(cpu.get(address+i,1) for i in range(count))
def fill(cpu,address,data):
    for i,value in enumerate(data):cpu.put(address+i,1,value)
def mask(lanes):return ''.join('1' if i in lanes else '0' for i in range(72))
def authored(seed):return bytearray((seed*13+i*17+11)&255 for i in range(72))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
    out=a.out.resolve()
    if out.exists() or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose NEW private output directory')
    image=a.exe.read_bytes();owner=OwnerExe(image);steps=0;rows=[];lane_cases=[]
    assert owner.word(0x80089a64)==0x80044c58
    # Execute each unchanged JAL and delay slot to its real callee. Opaque
    # helpers are separate external boundaries, not hooked/acknowledged calls.
    boundaries=[]
    for site,target in [(0x80015e0c,0x80017528),(0x80015e14,0x80051be4),
        (0x80015e1c,0x8003c2d8),(0x80015e24,0x80017154),
        (0x80015e2c,0x800173b4),(0x80015e34,0x80038894)]:
        c=Probe(image,owner.base);c.run(site,target);assert c.calls==[{'site':hex(site),'target':hex(target)}]
        assert c.r[31]==site+8 and not c.return_only_hooks
        steps+=c.steps;boundaries.append({'site':hex(site),'target':hex(target),'resume':hex(c.r[31])})
    c=Probe(image,owner.base);c.r[16]=0x800c1b10;c.r[17]=0x80082114;c.put(c.r[16],1,3)
    c.run(0x800155d4,0x800158f0);assert c.indirect==[{'site':'0x800155ec','target':'0x800158f0'}] and c.r[31]==0x800155f4
    steps+=c.steps;c.steps=0;c.run(0x800155f4,0x80031aa4)
    assert c.calls[-1]=={'site':'0x800155f4','target':'0x80031aa4'} and c.r[31]==0x800155fc;steps+=c.steps
    # Preserve exact byte read/write coverage for every admitted scalar state.
    for state in range(5):
        for parameter in range(3) if 0<state<4 else (0,1,2,3,255):
            for timer in (0,1,3,255):
                case=len(rows);c=Probe(image,owner.base);w=authored(case)
                for offset,value in [(0,3),(1,128),(2,131),(3,parameter),(6,3),(8,state),(11,timer)]:w[offset]=value
                fill(c,FX,w);c.put(ALLOC,4,0x80000000);c.put(ALLOC+4,4,0);c.put(PENDING,4,0);c.put(PENDING+4,4,0)
                c.r[4]=FX;c.track=True;c.run(0x80044c58);c.track=False
                after=blob(c,FX,72);rows.append([w.hex(),after.hex(),mask(c.reads),mask(c.writes),c.get(PENDING,4)])
                lane_cases.append({'state':state,'parameter':parameter,'timer':timer,'reads':sorted(c.reads),'writes':sorted(c.writes)})
                steps+=c.steps;assert not c.return_only_hooks
    # Three fades at separated records, subtype2 and inactive131 coexist.
    c=Probe(image,owner.base);pool=bytearray(64*72)
    for slot in range(64):pool[slot*72+1]=128|slot
    pool[0]=1;pool[2]=2;pool[6]=3
    for slot in (1,31,63):
        w=authored(slot);w[0]=1;w[1]=128|slot;w[2]=131;w[3]=0;w[6]=3;w[8]=0
        pool[slot*72:(slot+1)*72]=w
    pool[32*72+2]=131;pool[32*72+6]=3
    fill(c,FX,pool);used=(0xc0000001,1)
    for bank in range(2):c.put(ALLOC+4*bank,4,used[bank]);c.put(PENDING+4*bank,4,0)
    sweeps=[]
    for invocation in range(1,14):
        c.steps=0;c.r[31]=0;c.visits=[];c.run(0x800173b4)
        assert c.visits==[1,31,63] and blob(c,FX,72)==pool[:72]
        before_cleanup=[c.get(ALLOC+4*b,4) for b in range(2)]
        pending=[c.get(PENDING+4*b,4) for b in range(2)]
        assert before_cleanup==list(used)
        assert pending==([0,0] if invocation<13 else [0x40000001,1]);steps+=c.steps
        c.steps=0;c.r[31]=0;c.run(0x80031aa4);steps+=c.steps
        used=tuple(c.get(ALLOC+4*b,4) for b in range(2));assert [c.get(PENDING+4*b,4) for b in range(2)]==[0,0]
        sweeps.append({'invocation':invocation,'visited':c.visits,'pending_before_cleanup':pending,'allocated_after_cleanup':list(used)})
    assert used==(0x80000000,0)
    fixture=('before after reads writes pending\n'+''.join(' '.join(map(str,row))+'\n' for row in rows)).encode()
    proof={'schema':'mml-source-phase5-effect131/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,
        'callback_cases':len(rows),'unchanged_original_instructions':steps,'instruction_patches':0,'helper_hooks':0,
        'world_call_boundaries':boundaries,'phase_dispatch':{'site':'0x800155ec','target':'0x800158f0','resume':'0x800155f4'},
        'cleanup_boundary':{'site':'0x800155f4','target':'0x80031aa4','resume':'0x800155fc'},
        'sweeps':sweeps,'read_write_lanes':lane_cases,'fixture_sha256':hashlib.sha256(fixture).hexdigest(),
        'limits':['Authored scalar records, not live retail scene observations.','Opaque call boundaries prove order only; earlier/later world helpers and cooperative yielding are unexecuted.',
            'Thirteen original callback invocations are not thirteen native frames.','Only131 is admitted; active subtypes>=128 other than131 remain native holds.',
            'No projection/material/draw, complete native scheduler, retail parity or hardware claim.','No original instructions, tables or assets exported.']}
    out.mkdir(parents=True);(out/'callback-known-lanes.tsv').write_bytes(fixture);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({key:proof[key] for key in ('callback_cases','unchanged_original_instructions','instruction_patches','helper_hooks','world_call_boundaries','cleanup_boundary')}))
if __name__=='__main__':main()

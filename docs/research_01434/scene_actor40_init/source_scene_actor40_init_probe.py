#!/usr/bin/env python3
"""Complete unchanged class40 fresh wrapper oracle; retail outputs stay private."""
import argparse,hashlib,json,struct,sys,zlib
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[3]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01414'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01423')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from actor_pose_proxy_source_probe import Probe
from player_contact_source_probe import CPU
WORK=0x800c3580;PLAYER=0x800b51b0;PLAYER_BYTES=0x5f4

def fill(c,a,b):
    for i,v in enumerate(b):c.put(a+i,1,v)
def read(c,a,n):return bytes(c.get(a+i,1) for i in range(n))
def sha(b):return hashlib.sha256(b).hexdigest()
def authored(seed,n):return bytes((seed*37+i*17)&255 for i in range(n))
class InitProbe(Probe):
    # The old readonly overlay adapter returns immutable initial global data.
    # Admit these proved mutable data globals through CPU memory instead; code
    # fetch is unchanged and no instruction/helper is replaced.
    def get(self,a,n,signed=False):
        if 0x80122470<=a and a+n<=0x80122478:return CPU.get(self,a,n,signed)
        return super().get(a,n,signed)
def owner_case(image,owner,overlay,ebd,seed,shift,position):
    c=InitProbe(image,owner.base,overlay);fill(c,0x8016c000,ebd)
    for lo,hi in ((0x8002e88c,0x8002e8d8),(0x8002e924,0x8002e980),(0x8002e9cc,0x8002ea18)):
        c.r=[0]*32;c.r[29]=0x700000;c.run(lo,hi)
    c.r=[0]*32;c.r[29]=0x700000;c.r[4]=0x800c4c48;c.run(0x80100efc)
    if position:
        for i,n in enumerate(position):c.put(WORK+20+i*2,2,n)
    before=read(c,WORK,1444);player=authored(seed,PLAYER_BYTES);fill(c,PLAYER,player)
    global70=(seed*0x1234567+0x87654321)&0xffffffff;global74=(seed*0x2345678+0x12345678)&0xffffffff
    c.put(0x80122470,4,global70);c.put(0x80122474,4,global74)
    c.r=[0]*32;c.r[29]=0x700000;c.r[4]=WORK;c.put(0x1f800074,1,shift);c.steps=0;c.gte_commands=0
    c.run(0x8010d824);assert not c.return_only_hooks
    assert c.get(WORK+8,1)==1 and c.get(0x80122470,4)==0x8011c338 and c.get(0x80122474,4)==0x8011c3a4
    return [seed,shift,global70,global74,before.hex(),read(c,WORK,1444).hex(),player.hex(),read(c,PLAYER,PLAYER_BYTES).hex()],c.steps,c.gte_commands

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('exe','overlay','ebd','out'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();out=a.out.resolve()
    if out.exists() or out.is_relative_to(SOURCE):raise ValueError('Choose NEW private evidence directory')
    image=a.exe.read_bytes();owner=OwnerExe(image);overlay=a.overlay.read_bytes();ebd=a.ebd.read_bytes()
    if sha(overlay)!='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c' or sha(ebd)!='ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15':raise ValueError('Unsupported owners')
    rows=[];steps=gte=0
    for position in (None,(-32768,-32768,-32768),(32767,32767,32767),(-1,0,1)):
        for shift in (0,1,2):
            row,n,g=owner_case(image,owner,overlay,ebd,len(rows),shift,position);rows.append(row);steps+=n;gte+=g
    fixture=('seed shift prior70 prior74 before after player_before player_after\n'+''.join(' '.join(map(str,r))+'\n' for r in rows)).encode()
    public=[];mirror_steps=0
    for seed in range(32):
        before=tuple((seed*0x1234+i*0x4567)&65535 for i in range(3));after=tuple((seed*0x4321+i*0x7654)&65535 for i in range(3))
        c=InitProbe(image,owner.base,overlay);c.r[16]=WORK;player=authored(seed,PLAYER_BYTES);fill(c,PLAYER,player)
        for i,n in enumerate(before):c.put(WORK+20+i*2,2,n)
        c.run(0x8010d834,0x8010d858)
        for i,n in enumerate(after):c.put(WORK+20+i*2,2,n)
        c.run(0x8010d888,0x8010d8ac);assert not c.return_only_hooks;mirror_steps+=c.steps
        public.append([seed,*before,*after,zlib.crc32(read(c,PLAYER,PLAYER_BYTES))&0xffffffff])
    public_fixture=('seed previous2c previous2e previous30 primary14 primary16 primary18 expected_player_crc32\n'+''.join(' '.join(map(str,r))+'\n' for r in public)).encode()
    gate=CPU(image,owner.base);gate_steps=0
    for raw00 in range(256):
        for raw06 in range(256):
            gate.r=[0]*32;gate.r[29]=0x700000;gate.steps=0
            gate.put(PLAYER,1,raw00);gate.put(PLAYER+6,1,raw06)
            gate.run(0x8003c07c,0x8003c098)
            assert gate.r[2]==(raw00&raw06&1)
            gate_steps+=gate.steps
    proof={'schema':'source-scene-actor40-complete-initializer/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,'overlay_sha256':sha(overlay),'ebd_sha256':sha(ebd),
           'player_dispatch_gate_cases':65536,'player_dispatch_gate_instructions':gate_steps,'player_dispatch_gate_code_sha256':sha(owner.read(0x8003c07c,32)),
           'player_before_class40_call_order':['0x80015DD4->0x8003C07C','0x80015DEC->0x80017094'],'player_class40_order_code_sha256':sha(owner.read(0x80015db4,64)),
           'complete_wrapper_cases':len(rows),'original_wrapper_instructions':steps,'restricted_mvmva_commands':gte,'authored_halfword_copy_cases':len(public),'original_halfword_copy_instructions':mirror_steps,
           'actual_initialized_byte08':1,'initializer_global_identity_addresses':['0x80122470','0x80122474'],'initializer_global_tokens':['0x8011C338','0x8011C3A4'],
           'player_halfword_offsets':[20,22,24,44,46,48],'fixed_point_heading_state_inference':False,'opaque_stack_lanes_invalidated':12,'helper_hooks':0,'instruction_patches':0,
           'private_owner_fixture_sha256':sha(fixture),'authored_fixture_sha256':sha(public_fixture),'initializer_code_sha256':sha(overlay[0xc930:0xc9c8]),'wrapper_code_sha256':sha(overlay[0xd824:0xd8c0]),'hardware_tested':False,'retail_parity':False,
           'limits':['Only fresh08=0 actualdispatch1/class40/subtype5 initializer and wrapper halfword copies.','Later8010D7A4/8010C820 movement/animation callbacks remain held.','Global tokens are source identities only; later overlay table consumers remain separate.','PoseGTE timing/unused flags and12 unowned temporary stack lanes remain outside exactknown-field proof.','Owner work/model/texture outputs remain private.']}
    out.mkdir(parents=True);(out/'owner-wrapper.tsv').write_bytes(fixture);(out/'authored-player-mirrors.tsv').write_bytes(public_fixture);(out/'reference-trig.bin').write_bytes(owner.read(0x80092ec4,16384));(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

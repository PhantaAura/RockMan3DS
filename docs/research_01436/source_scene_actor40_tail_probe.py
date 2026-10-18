#!/usr/bin/env python3
"""Unchanged MIPS tail write tracing. Owner-derived snapshots stay private."""
import argparse, itertools, json, struct, sys, zlib
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(SOURCE/'docs/research_01436'))
import source_scene_actor40_state_probe as states
m=states.m
BASE=0x8016c000

class TailProbe(m.InitProbe):
    def observe_branch(self,pc,word):
        op=word>>26;a=self.r[word>>21&31];b=self.r[word>>16&31]
        if op in (4,5):taken=(a==b)==(op==4)
        elif op in (6,7):taken=(a-0x100000000 if a&0x80000000 else a)<=0;taken=taken==(op==6)
        elif op==1 and (word>>16&31) in (0,1):taken=bool(a&0x80000000)==((word>>16&31)==0)
        else:return
        self.branches.add((pc,taken))
    def get(self,a,n,signed=False):
        # These are mutable overlay DATA; instruction fetch remains original.
        if 0x80122460<=a and a+n<=0x80122478:return m.CPU.get(self,a,n,signed)
        return super().get(a,n,signed)
    def put(self,a,n,v):
        super().put(a,n,v)
        if hasattr(self,'writes'):self.writes.append((self.current_pc,a,n))

# Extend interpreter observation only. No owner instruction/helper is patched.
import actor_pose_proxy_source_probe as pose
run_source=pose.run_source
run_source=run_source.replace('w = self.get(pc, 4)',
    'self.current_pc=pc\n        self.visited.add(pc)\n        w = self.get(pc, 4)',1)
run_source=run_source.replace('w = self.get(pc, 4)','w = self.get(pc, 4)\n        self.observe_branch(pc,w)',1)
namespace={};exec(run_source,m.InitProbe.run.__globals__,namespace)
TailProbe.run=namespace['run']

def reset(c):
    c.r=[0]*32;c.r[29]=0x700000;c.steps=0
    c.writes=[];c.visited=set();c.branches=set();c.current_pc=0;c.calls=[];c.indirect=[]

def execute(c,pc,args):
    reset(c);c.r[4:4+len(args)]=args;c.run(pc)
    assert not c.return_only_hooks
    return c.steps

def setup(image,owner,overlay,ebd):
    c=states.setup(image,owner,overlay,ebd);c.__class__=TailProbe
    reset(c);return c

def assert_actor_preserved(c,before):
    assert m.read(c,m.WORK,1444)==before
    assert not any(a<m.WORK+1444 and a+n>m.WORK for _,a,n in c.writes)

def authored_material(seed,count):
    payload=bytearray(65536)
    struct.pack_into('<I',payload,0x100+112,BASE+0x200)
    for pair,part in enumerate((16,18)):
        dest=0x200+part*20+20;source=dest+20
        struct.pack_into('<I',payload,dest,count|(count<<8))
        for kind in range(2):
            lane=pair*4+kind*2
            to=0x800+lane*4096;origin=to+4096
            struct.pack_into('<I',payload,dest+4+kind*4,BASE+to)
            struct.pack_into('<I',payload,source+4+kind*4,BASE+origin)
            for i in range(count*12):
                payload[to+i]=(seed*37+i*17)&255
                payload[origin+i]=(seed*13+i*19)&255
    return payload

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','overlay','ebd','out'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();out=a.out.resolve()
    if out.exists()or out.is_relative_to(SOURCE):raise ValueError('Choose NEW private evidence directory')
    image=a.exe.read_bytes();owner=m.OwnerExe(image)
    overlay=a.overlay.read_bytes();ebd=a.ebd.read_bytes()
    assert m.sha(image)==m.OWNER_SHA
    assert m.sha(overlay)=='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
    assert m.sha(ebd)=='ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15'
    original_steps=0;shadow_cases=cue_cases=material_cases=0;visited=set();writes=set();branches=set()
    owner_rows=[];public_rows=[]
    for position,work76,count in itertools.product(
            ((0,0,0),(-32768,-32768,-32768),(32767,32767,32767),(-1,0,1)),
            (0,1,255),(0,63,64,0xffffffff)):
        c=setup(image,owner,overlay,ebd)
        for axis,value in enumerate(position):c.put(m.WORK+20+axis*2,2,value)
        c.put(m.WORK+0x76,1,work76);c.put(0x800987d8,4,count)
        before=m.read(c,m.WORK,1444)
        original_steps+=execute(c,0x8002c8cc,[m.WORK,50,683,0])
        assert_actor_preserved(c,before);visited.update(c.visited);writes.update(c.writes);branches.update(c.branches)
        expected=count if count>=64 else count+1
        assert c.get(0x800987d8,4)==expected
        owner_rows.append(['shadow',shadow_cases,before.hex(),m.read(c,m.WORK,1444).hex()])
        shadow_cases+=1
    # Both original fourteen-entry cue tables; normal tick, duration equality,
    # and their subsequent loop/control records execute the real UV helpers.
    for index in range(14):
        c=setup(image,owner,overlay,ebd);before=m.read(c,m.WORK,1444)
        for pc in (0x8010c5d0,0x8010c644):
            original_steps+=execute(c,pc,[m.WORK,index])
            assert_actor_preserved(c,before);visited.update(c.visited);writes.update(c.writes);branches.update(c.branches);cue_cases+=1
        for tick in range(12):
            for pointer,timer in ((0x80122460,0x80122464),(0x80122468,0x8012246c)):
                duration=c.get(c.get(pointer,4)+2,2,True)
                if duration>0:c.put(timer,2,duration-1 if tick%2 else 0)
            original_steps+=execute(c,0x8010c6b8,[m.WORK])
            assert_actor_preserved(c,before);visited.update(c.visited);writes.update(c.writes);branches.update(c.branches);cue_cases+=1
        owner_rows.append(['cues',index,before.hex(),m.read(c,m.WORK,1444).hex()])
    # Actual outer cue script match, miss, and terminator branches.
    for first_match,second_match,terminated in itertools.product((False,True),repeat=3):
        c=setup(image,owner,overlay,ebd);before=m.read(c,m.WORK,1444)
        for pointer,match in ((0x80122470,first_match),(0x80122474,second_match)):
            script=c.get(pointer,4)
            if terminated:
                while c.get(script,1)!=255:script+=6
                c.put(pointer,4,script)
            elif match:
                c.put(0x800c4c50,2,c.get(script,2));c.put(0x800c4c64,4,c.get(script+2,2))
        # Both script starts use the same first event phase/time.
        if not first_match and not second_match and not terminated:c.put(0x800c4c50,2,0xffff)
        original_steps+=execute(c,0x8010c820,[m.WORK])
        assert_actor_preserved(c,before);visited.update(c.visited);writes.update(c.writes);branches.update(c.branches);cue_cases+=1
    # Authored outer event rows distinguish phase miss from time miss for both
    # original conditional cue handlers, including the sentinel fast path.
    for mode0,mode1 in itertools.product(('match','phase_miss','time_miss','sentinel'),repeat=2):
        c=setup(image,owner,overlay,ebd);before=m.read(c,m.WORK,1444)
        c.put(0x800c4c50,2,1);c.put(0x800c4c64,4,2)
        for pointer,address,mode in ((0x80122470,0x4000,mode0),(0x80122474,0x4010,mode1)):
            c.put(pointer,4,address);c.put(address,2,255 if mode=='sentinel' else 3 if mode=='phase_miss' else 1)
            c.put(address+2,2,4 if mode=='time_miss' else 2);c.put(address+4,2,0)
        original_steps+=execute(c,0x8010c820,[m.WORK]);assert_actor_preserved(c,before)
        visited.update(c.visited);writes.update(c.writes);branches.update(c.branches);cue_cases+=1
    # Authored primitive counts at both part pairs. The oracle itself emits the
    # final player halfwords, then C++ checks the admitted receipt against CRC.
    for seed,count in enumerate((0,1,2,3,63,254,255)):
        c=TailProbe(image,owner.base,overlay);payload=authored_material(seed,count)
        m.fill(c,BASE,payload);m.fill(c,m.WORK,m.authored(seed,1444))
        c.put(m.WORK+136,4,BASE+0x100)
        previous=tuple((seed*0x1234+i*0x4567)&65535 for i in range(3))
        primary=tuple((seed*0x4321+i*0x7654)&65535 for i in range(3))
        player=m.authored(seed,m.PLAYER_BYTES);m.fill(c,m.PLAYER,player)
        for axis,value in enumerate(primary):c.put(m.WORK+20+axis*2,2,value)
        before=m.read(c,m.WORK,1444)
        for part in (16,18):
            original_steps+=execute(c,0x8002d320,[BASE+0x100,((part+1)<<16)|part,0x12345678])
            assert_actor_preserved(c,before);visited.update(c.visited);writes.update(c.writes);branches.update(c.branches);material_cases+=1
        # Exactly the original final mirror block, independent previous copy.
        for axis,value in enumerate(previous):c.put(m.PLAYER+44+axis*2,2,value)
        reset(c);c.r[16]=m.WORK;c.run(0x8010d888,0x8010d8ac);original_steps+=c.steps
        assert_actor_preserved(c,before)
        public_rows.append([seed,count,*previous,*primary,zlib.crc32(m.read(c,m.PLAYER,m.PLAYER_BYTES))&0xffffffff])
    # Demonstrate that actor-aliasing primitive pointers are a real hazard:
    # the unchanged helper overwrites actor20/24; the native guard rejects it.
    c=TailProbe(image,owner.base,overlay);payload=authored_material(91,1)
    struct.pack_into('<I',payload,0x200+16*20+24,m.WORK+20)
    m.fill(c,BASE,payload);m.fill(c,m.WORK,m.authored(91,1444));before=m.read(c,m.WORK,1444)
    original_steps+=execute(c,0x8002d320,[BASE+0x100,0x00110010,0])
    assert m.read(c,m.WORK,1444)!=before
    fixture=('seed count previous2c previous2e previous30 primary14 primary16 primary18 expected_player_crc32\n'+
             ''.join(' '.join(map(str,row))+'\n'for row in public_rows)).encode()
    owner_fixture=('kind case before after\n'+''.join(' '.join(map(str,row))+'\n'for row in owner_rows)).encode()
    # Whole-function opcode audit covers unexecuted branch stores too. The
    # floor classifiers receive caller stack bounds and fixed scratch buffers;
    # no actor-work pointer is used as their write destination.
    static=[]
    for name,lo,hi,allowed in (
        ('shadow_queue',0x8002c8cc,0x8002ca10,{1,29}),
        ('floor_wrapper',0x80048e6c,0x8004920c,{1,29,6}),
        ('candidate_cells',0x8004b1e0,0x8004b3c0,{23,29}),
        ('floor_bounds',0x8004b3c0,0x8004b414,{1}),
        ('floor_cell_lookup',0x80048bec,0x80048e6c,{29}),
        ('floor_y_classifier',0x8004a060,0x8004ac58,{14}),
        ('floor_xz_classifiers',0x8004e304,0x8004f44c,{1,4,6,8}),
        ('material_primitive_words',0x8002d320,0x8002d404,{4}),
        ('material_offset',0x8002d404,0x8002d464,set())):
        code=owner.read(lo,hi-lo);stores=[]
        for offset in range(0,len(code),4):
            word=struct.unpack_from('<I',code,offset)[0]
            if word>>26 in (40,41,42,43,46,58):stores.append((lo+offset,word>>21&31))
        assert all(reg in allowed for _,reg in stores)
        static.append({'name':name,'start':hex(lo),'end_exclusive':hex(hi),'sha256':m.sha(code),
            'store_instructions':len(stores),'store_base_registers':sorted(set(reg for _,reg in stores))})
    tail_branch_sites=(0x8002c904,0x8002c930,0x8002c958,0x8010c6d8,0x8010c72c,
        0x8010c77c,0x8010c7d0,0x8010c84c,0x8010c864,0x8010c87c,
        0x8010c8bc,0x8010c8d0,0x8010c8e4,0x8002d384,0x8002d3bc,
        0x8002d3b4,0x8002d3f0)
    missing=[hex(pc) for pc in tail_branch_sites if not all((pc,taken) in branches for taken in (False,True))]
    # The original floor max selector can be covered with explicit terrain;
    # its store destinations are proved by the complete static helper audit.
    assert missing==['0x8002c958'],missing
    proof={'schema' :'source-scene-actor40-tail/1','status':'SOURCE_BACKED_PARTIAL',
        'owner_exe_sha256':m.OWNER_SHA,'overlay_sha256':m.sha(overlay),'ebd_sha256':m.sha(ebd),
        'shadow_cases':shadow_cases,'cue_cases':cue_cases,'authored_material_cases':material_cases,
        'authored_player_mirror_cases':len(public_rows),'original_instructions':original_steps,
        'observed_instruction_addresses':len(visited),'both_outcomes_tail_branch_sites':len(tail_branch_sites)-len(missing),
        'one_outcome_floor_max_branch':missing,'static_store_destination_audit':static,'observed_store_events':len(writes),
        'actor_work_1444_preserved_in_all_admitted_cases':True,
        'deliberate_actor_alias_originally_corrupts_actor':True,
        'material_part_pairs':[[16,17],[18,19]],'primitive_counts':[0,1,2,3,63,254,255],
        'material_store_bytes_per_primitive':8,'material_stride':12,
        'wrapper_tail_code_sha256':m.sha(overlay[0xd800:0xd8ac]),
        'cue_code_sha256':m.sha(overlay[0xc5d0:0xc930]),
        'material_code_sha256':m.sha(owner.read(0x8002d320,0x144)),
        'shadow_code_sha256':m.sha(owner.read(0x8002c8cc,0x144)),
        'authored_fixture_sha256':m.sha(fixture),'private_owner_fixture_sha256':m.sha(owner_fixture),
        'helper_hooks':0,'instruction_patches':0,'retail_parity':False,'hardware_tested':False,
        'limits':['Shadow queue/floor and cue clocks/material UV presentation remain pending in native tail.',
            'Invariance is admitted only after resident material spans exclude the complete original class40 pool.',
            'Shadow floor functions write caller stack and fixed scratch; map geometry ownership remains separate.',
            'Original owner snapshots and MIPS write traces remain private; no owner asset/code bytes ship.']}
    out.mkdir(parents=True);(out/'authored-player-mirrors.tsv').write_bytes(fixture)
    (out/'owner-tail.tsv').write_bytes(owner_fixture)
    (out/'write-trace.json').write_text(json.dumps(sorted(writes))+'\n')
    (out/'branch-trace.json').write_text(json.dumps(sorted(branches))+'\n')
    (out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))

if __name__=='__main__':main()

#!/usr/bin/env python3
"""Unchanged source fade callback/packet proof on authored state, no BIOS hooks.

The owner EXE stays local and read only. Output contains authored bytes, hashes,
addresses and aggregate facts only; never source instructions or owner tables.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE/'tools'), str(SOURCE/'docs/research_01415'),
               str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe, OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe

REGIONS = ((0x80098A68,16), (0x80098A28,32), (0x80098A10,24))
YIELD = 0x80012E98
TERMINATE = 0x80012ECC


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def make_cpu(image, owner, seed):
    cpu = OverlayProbe(image, owner.base, b'')
    offset = 0
    for address, size in REGIONS:
        for i in range(size):
            cpu.put(address+i, 1, (seed*37+offset*17)&255)
            offset += 1
    cpu.r[29] = 0x7000
    cpu.r[31] = 0
    return cpu


def context(cpu):
    return bytes(cpu.get(address+i,1) for address,size in REGIONS for i in range(size))


def boundary(cpu, start):
    # Select the only possible next boundary from the source state, without
    # changing instructions. phase2/step0 calls termination before the yield.
    phase = cpu.get(0x80098A69,1)
    target = TERMINATE if start==0x80017F3C and phase==2 and cpu.get(0x80098A74,2)==0 else YIELD
    cpu.run(start,target)
    if target==YIELD:
        require(cpu.r[4]==1, 'Source yield argument changed')
        return 0
    return 1


def prove(image, out):
    owner = OwnerExe(image)
    require(owner.word(0x80082140)==0x80017ED0, 'Unsupported fade dispatch')
    callback_rows, present_rows = [], []
    original_steps = 0
    steps = (0,1,7,255,256,32767,32768,65535)
    starts = [(0, seed, busy, 0, 0, step, hold)
              for seed,busy in enumerate((0,1,2,32767,32768,65535),1)
              for step in steps for hold in (0,1,2,255)]
    updates = [(1, 100+phase, busy, phase, intensity, step, 2)
               for phase in (0,1,2,3,255) for intensity in (0,1,127,254,255,256,32767,32768,65535)
               for step in steps for busy in (0,1,2,65535)]
    for kind,seed,busy,phase,intensity,step,hold in starts+updates:
        cpu = make_cpu(image,owner,seed)
        cpu.put(0x80098A68,1,0)
        cpu.put(0x80098A69,1,phase)
        cpu.put(0x80098A72,2,busy)
        cpu.put(0x80098A74,2,step)
        cpu.put(0x80098A76,2,intensity)
        cpu.put(0x1F800001,1,hold)
        # Authored resumed registers match the callback's original entry setup.
        if kind:
            cpu.r[16],cpu.r[17],cpu.r[18]=0x80098A68,2,1
            status=boundary(cpu,0x80017F3C)
        else:
            status=boundary(cpu,0x80017E94)
        require(not cpu.return_only_hooks, 'Helper hooks forbidden')
        original_steps+=cpu.steps
        callback_rows.append('\t'.join(str(v) for v in
            (kind,seed,busy,phase,intensity,step,hold,status,context(cpu).hex())))
    for seed in (1,3,97):
        for active in (0,1,32768,65535):
            for intensity in (0,1,127,254,255,256,32768,65535):
                for buffer in (0,1):
                    cpu=make_cpu(image,owner,seed)
                    cpu.put(0x80098A70,2,active)
                    cpu.put(0x80098A76,2,intensity)
                    cpu.put(0x1F800000,1,buffer)
                    cpu.put(0x80098934,4,0x5000)
                    tag=(0xCA000000 | ((seed*8191+intensity)&0xFFFFFF))
                    cpu.put(0x5000+116,4,tag)
                    cpu.run(0x80017B24)
                    require(not cpu.return_only_hooks, 'Presentation hooks forbidden')
                    original_steps+=cpu.steps
                    present_rows.append('\t'.join(str(v) for v in
                        (seed,active,intensity,buffer,tag,cpu.get(0x5000+116,4),context(cpu).hex())))
    # Run the original yield's publication up to BIOS ChangeThread; do not
    # provide a fake BIOS return, handle, SP/GP or thread completion receipt.
    cpu=make_cpu(image,owner,1)
    cpu.put(0x801F8300,4,0x6000);cpu.put(0x6000,4,0xABCD1234)
    cpu.r[4]=1
    cpu.run(YIELD,0x8007FF30)
    require(cpu.get(0x6000,2)==1 and cpu.get(0x6002,2)==1,'Scheduler wait publication changed')
    original_steps+=cpu.steps
    # Original scheduler state1 decrements the published halfword; this authored
    # one-pass case reaches ChangeThread when it becomes zero.
    cpu=make_cpu(image,owner,1)
    cpu.put(0x801F8300,4,0x6000);cpu.put(0x6000,2,1);cpu.put(0x6002,2,1)
    cpu.put(0x6008,4,0xABC);cpu.put(0x6070,1,0)
    cpu.r[16]=0x801F8300;cpu.r[17]=3
    cpu.run(0x80012CAC,0x8007FF30)
    require(cpu.get(0x6002,2)==0 and cpu.get(0x6000,2)==3 and cpu.r[4]==0xABC,
            'One scheduler pass wait/resume changed')
    original_steps+=cpu.steps
    budget_writers=[]
    for start,end,expected in ((0x80012004,0x80012020,2),
                               (0x80013430,0x80013444,2),
                               (0x800155B4,0x800155D0,2),
                               (0x800167D4,0x800167E0,4),
                               (0x8001683C,0x80016848,2),
                               (0x800169E8,0x800169F4,4),
                               (0x80016A50,0x80016A5C,2)):
        cpu=make_cpu(image,owner,1)
        cpu.put(0x1F800001,1,0xAB)
        cpu.run(start,end)
        require(cpu.get(0x1F800001,1)==expected,'Source frame-budget writer changed')
        original_steps+=cpu.steps
        budget_writers.append({'start':hex(start),'stop_before':hex(end),
                               'written_byte':expected})
    callback_data=('\n'.join(callback_rows)+'\n').encode()
    present_data=('\n'.join(present_rows)+'\n').encode()
    result={'schema':'mml-source-fade-callback-probe/1','status':'SOURCE_BACKED_PARTIAL',
            'owner_exe_sha256':OWNER_SHA,'authored_callback_cases':len(callback_rows),
            'authored_presentation_cases':len(present_rows),
            'executed_original_instructions':original_steps,'instruction_patches':0,'helper_hooks':0,
            'callback_entry':'80017E94','type2_dispatch':'80017ED0','resume_iteration':'80017F3C',
            'yield_boundary':'80012E98(1) before BIOS ChangeThread8007FF30',
            'termination_boundary':'80012ECC before BIOS close/delete/change operations',
            'presentation_entry':'80017B24','packet_order':'DR_MODE then TILE then previous OT+116 tail',
            'required_external_owners':['endpoint hold byte1F800001','selected buffer byte1F800000',
                                        'current ordering-table tag+116 for exact packet ownership',
                                        'native explicit scheduling pass; no PS1 BIOS handle is provided'],
            'frame_budget_writer_probes':budget_writers,
            'frame_budget_admission_limit':'Fresh initialization proves2 only at its own boundary; control writers prove4/2 and prevent a universal constant fallback.',
            'source_block_sha256':{'dispatch':hashlib.sha256(owner.read(0x80017E94,0x40)).hexdigest(),
                                   'callback':hashlib.sha256(owner.read(0x80017ED0,0x180)).hexdigest(),
                                   'presentation':hashlib.sha256(owner.read(0x80017B24,0x10C)).hexdigest(),
                                   'yield':hashlib.sha256(owner.read(YIELD,0x34)).hexdigest()},
            'fixture_sha256':{'callback':hashlib.sha256(callback_data).hexdigest(),
                              'presentation':hashlib.sha256(present_data).hexdigest()},
            'limits':['Authored state is not live retail chronology or whole-fade parity.',
                      'CPU source instructions execute unchanged; BIOS and GPU boundaries are never hooked or executed.',
                      'Native scheduling acknowledgment is not a PS1 BIOS thread creation/completion receipt.',
                      'Subtract-mode command semantics do not prove 3DS framebuffer quantization or blend parity.',
                      'No commercial instruction bytes, resource tables, textures or audio are emitted.']}
    out.mkdir(parents=True,exist_ok=False)
    (out/'source-fade-callback.tsv').write_bytes(callback_data)
    (out/'source-fade-presentation.tsv').write_bytes(present_data)
    (out/'source-proof.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    require(not args.out.resolve().is_relative_to(SOURCE),'Keep probe output outside source')
    print(json.dumps(prove(args.exe.read_bytes(),args.out),indent=2))

if __name__=='__main__':main()

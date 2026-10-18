#!/usr/bin/env python3
"""Unchanged phase4/fade startup with authored data; stop before BIOS scheduler.

No code patches or helper hooks. Emits authored packet values and scalar/hash
evidence only; original instructions and owner-derived tables are not emitted.
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
from build_stage_catalog import iter_resources
from tutorial_actor_identity_probe import OverlayProbe, OVERLAY_SHA

REGIONS = ((0x80098A68,16), (0x80098A28,32), (0x80098A10,24))
WORK = 0x3000

def require(condition, reason):
    if not condition:
        raise ValueError(reason)

def authored(seed, offset):
    return bytes((seed*37 + i*17 + offset*11) & 255 for i in range(72))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--st00', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    image = args.exe.read_bytes()
    owner = OwnerExe(image)
    overlay = next(r['payload'] for r in iter_resources(args.st00)
                   if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    require(hashlib.sha256(overlay).hexdigest()==OVERLAY_SHA, 'Unsupported ST00 identity')
    require(not args.out.exists(), 'Output must be a NEW directory')
    rows, total_steps, maximum_steps, phase_before_helper, noops = [], 0, 0, 0, 0
    cases = [(gpu+1, duration, step, 0, gpu)
             for duration, step in ((3,7),(65535,32768)) for gpu in range(256)]
    cases += [(600+i,3,7,busy,99) for i,busy in enumerate((1,32767,32768,65535,42))]
    for seed,duration,step,busy,gpu in cases:
        cpu = OverlayProbe(image, owner.base, overlay)
        initial = authored(seed,0)
        initial = initial[:10] + struct.pack('<H',busy) + initial[12:]
        position = 0
        for address,size in REGIONS:
            for i in range(size): cpu.put(address+i,1,initial[position+i])
            position += size
        cpu.put(0x80097A44,2,duration)
        cpu.put(0x80097A4C,2,step)
        cpu.put(0x80097718,1,gpu)
        cpu.put(WORK,1,1);cpu.put(WORK+1,1,1);cpu.put(WORK+4,1,4);cpu.put(WORK+12,4,1)
        cpu.r[4]=WORK;cpu.r[29]=0x7000;cpu.r[31]=0
        # The source call delay-slot phase store must have executed at helper
        # entry. This proves the caller does not wait for fade completion.
        cpu.run(0x801010C4, 0x80017C30)
        require(cpu.get(WORK+4,1)==5 and cpu.get(WORK+12,4)==0,
                'Source caller did not publish phase5/count0 before helper')
        phase_before_helper += 1
        cpu.run(0x80017C30, 0x80012E10 if busy==0 else None)
        if busy==0:
            require(cpu.r[4]==2 and cpu.r[5]==0x80017E94, 'Scheduler ownership boundary changed')
        else:
            require(cpu.get(WORK+4,1)==5 and cpu.get(WORK+12,4)==0, 'Busy no-op caller state')
            noops += 1
        require(not cpu.return_only_hooks, 'Original instruction hooks are forbidden')
        result = bytes(cpu.get(a+i,1) for a,n in REGIONS for i in range(n))
        if busy: require(result==initial, 'Busy source helper must preserve all context/packets')
        rows.append(f'{seed}\t{duration}\t{step}\t{busy}\t{gpu}\t{result.hex()}')
        total_steps += cpu.steps; maximum_steps=max(maximum_steps,cpu.steps)
    fixture = ('\n'.join(rows)+'\n').encode()
    evidence = {'schema':'mml-source-fade-startup-probe/1', 'owner_exe_sha256':OWNER_SHA,
                'overlay_sha256':OVERLAY_SHA, 'authored_cases':len(rows),
                'phase_increment_before_helper_cases':phase_before_helper,
                'busy_noop_cases':noops, 'gpu_type_domain':256,
                'executed_original_instructions':total_steps,'maximum_case_steps':maximum_steps,
                'instruction_patches':0,'helper_hooks':0,'retail_parity':False,
                'stop_before':'80012E10(slot2,callback80017E94)',
                'fixture_sha256':hashlib.sha256(fixture).hexdigest(),
                'source_block_sha256':hashlib.sha256(owner.read(0x80017C30,0x264)).hexdigest(),
                'source_handler_sha256':hashlib.sha256(overlay[0x1184:0x11B4]).hexdigest(),
                'limits':['Authored state is not a live retail trace.',
                          'BIOS thread SP/GP/handle, yielding, fade presentation and completion remain unbound.']}
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'source-fade-startup.tsv').write_bytes(fixture)
    (args.out/'source-proof.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps(evidence,indent=2))

if __name__=='__main__':main()

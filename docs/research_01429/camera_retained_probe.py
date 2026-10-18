#!/usr/bin/env python3
"""Exact cold camera-vector ownership; no copied game bytes in evidence.

Runs the unchanged executable BSS clear loop with an expanded instrumentation
step bound. Fresh camera initialization is checked separately against nonzero
authored retained scene state. Cold ownership is never a warm/load fallback.
"""
import argparse
import hashlib
import inspect
import json
from pathlib import Path
import struct
import sys
import textwrap

SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources
from player_contact_source_probe import s16,s32

# This changes the instrumentation limit only, not any original instruction or
# helper. The unchanged startup loop needs232064instructions per execution.
namespace={'s16':s16,'s32':s32}
exec(textwrap.dedent(inspect.getsource(OverlayProbe.run)).replace('10000','1000000'),namespace)
class Probe(OverlayProbe):
    run=namespace['run']

def sha(value):return hashlib.sha256(value).hexdigest()
def fill(cpu,address,value):
    for i,b in enumerate(value):cpu.put(address+i,1,b)
def read(cpu,address,size):return bytes(cpu.get(address+i,1) for i in range(size))
LOW,HIGH,CAMERA=0x80098158,0x800c5688,0x800bc738

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','st00','area','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();output=args.out.resolve()
    if output.exists() or SOURCE in output.parents or output==SOURCE:raise ValueError('Choose NEW output outside source')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(args.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay)!=OVERLAY_SHA:raise ValueError('Unsupported owner overlay')
    area=next(r['payload'] for r in iter_resources(args.area) if r['load']==0x8016c000)
    if sha(area)!='ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15':raise ValueError('Unsupported owner EBD')
    rows=[];total=0
    for seed in range(4):
        cpu=Probe(image,owner.base,overlay);authored=bytes(((seed*37+i*17)%255)+1 for i in range(HIGH-LOW))
        fill(cpu,LOW,authored);cpu.put(LOW-4,4,0x13579bdf);cpu.put(HIGH,4,0x2468ace0)
        cpu.run(0x80068000,0x80068024)
        assert read(cpu,LOW,HIGH-LOW)==bytes(HIGH-LOW),'Original cold clear failed'
        assert cpu.get(LOW-4,4)==0x13579bdf and cpu.get(HIGH,4)==0x2468ace0,'Clear range changed'
        assert cpu.r[2]==HIGH and cpu.r[3]==HIGH and not cpu.return_only_hooks
        assert read(cpu,CAMERA,188)==bytes(188)
        rows.append({'seed':seed,'original_instructions':cpu.steps,'whole_range_zero':True,'guard_words_preserved':True})
        total+=cpu.steps
    retained=[];prefix_steps=0
    for seed in range(24):
        cpu=Probe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[28]=0x80097864
        before=bytes((seed*37+i*17+11)&255 for i in range(188));fill(cpu,CAMERA,before)
        position=bytes((seed*17+i*31)&255 for i in range(6));fill(cpu,0x800b51c4,position)
        cpu.run(0x80038648,0x80038860)
        assert read(cpu,CAMERA,188)==before,'Fresh controller prefix secretly changed scene camera'
        assert not cpu.return_only_hooks
        retained.append(seed);prefix_steps+=cpu.steps
    # Exact cold constructor sequence: no scene bytes are filled after the
    # original clear loop. Actual owner resource/overlay bytes stay private.
    cpu=Probe(image,owner.base,overlay);cpu.r[29]=0x700000;cpu.r[28]=0x80097864
    cpu.run(0x80068000,0x80068024);cold_steps=cpu.steps
    position=struct.pack('<3h',-1234,567,3456);fill(cpu,0x800b51c4,position)
    cpu.steps=0;cpu.run(0x80038648,0x80038860);cold_prefix_steps=cpu.steps
    fill(cpu,0x8016c000,area)
    for lo,hi in ((0x8002e88c,0x8002e8d8),(0x8002e924,0x8002e980),(0x8002e9cc,0x8002ea18)):
        cpu.steps=0;cpu.run(lo,hi)
    cpu.put(0x800c4c48,1,1);cpu.put(0x800c4c49,1,1);cpu.put(0x800c4c4c,1,5)
    cpu.r[4]=0x800c4c48;cpu.steps=0;cpu.run(0x801010c4)
    assert cpu.get(0x800c4c4c,1)==6 and not cpu.return_only_hooks
    assert read(cpu,CAMERA+0x68,12)==bytes(12) and read(cpu,CAMERA+0x78,12)==bytes(12)
    constructor_steps=cpu.steps
    # Explicit first callback counter/input premises. This is not an assertion
    # that a live retail menu reached those fields through the same chronology.
    cpu.put(0x800c4c50,1,0);cpu.put(0x800c4c58,4,0);cpu.put(0x1f800002,2,0)
    cpu.r[4]=0x800c4c48;cpu.r[31]=0;cpu.steps=0;cpu.run(0x8010102c)
    callback_steps=cpu.steps
    assert read(cpu,CAMERA+0x68,12)==bytes(12) and read(cpu,CAMERA+0x78,12)==bytes(12)
    assert not cpu.return_only_hooks
    proof={'schema':'mml-camera-retained-cold-owner/1','status':'SOURCE_BACKED_PARTIAL',
           'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,
           'clear_source_block_sha256':sha(owner.read(0x80068000,0x24)),
           'clear_entry':'80068000','clear_stop_before':'80068024','clear_low':hex(LOW),'clear_end_exclusive':hex(HIGH),
           'source_camera_work':'800BC738','camera_work_bytes':188,
           'proved_retained_vector_offsets':['68','6C','70'],'proved_zero_velocity_offsets':['78','7C','80'],
           'cold_clear_cases':rows,'cold_clear_original_instructions':total,
           'fresh_controller_prefix_cases':len(retained),'fresh_controller_prefix_original_instructions':prefix_steps,
           'fresh_prefix_preserves_retained_scene_camera':True,
           'fresh_prefix_source_block_sha256':sha(owner.read(0x80038648,0x80038860-0x80038648)),
           'cold_boot_then_supported_owner_constructor':{'cold_instructions':cold_steps,'prefix_instructions':cold_prefix_steps,
               'constructor_instructions':constructor_steps,'retained_vector_zero':True,'velocity_vector_zero':True,'phase':6},
           'first_source_callback_prefix':{'entry':'8010102C','callback':'80100738','authored_counter':0,'authored_input':0,
               'original_instructions':callback_steps,'retained_vector_zero':True,'velocity_vector_zero':True},
           'instruction_patches':0,'helper_hooks':0,'retail_parity':False,'hardware_tested':False,
           'original_payloads_exported':False,'instrumentation_step_limit':1000000,
           'limits':['Cold executable bootstrap establishes explicit initial ownership, not a warm area/save/captured-state fallback.',
                     'The controller initializer prefix preserves existing scene camera; it cannot independently justify zeros.',
                     'Complete renderer camera update/GTE calls beyond80038860 are not ported by this proof.',
                     'This does not prove complete intro timeline, camera command execution, scene completion or hardware behavior.']}
    output.mkdir(parents=True,exist_ok=False);(output/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

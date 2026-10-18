#!/usr/bin/env python3
"""Bounded owner-local palette correction, E0 spawn gate and event metadata.

Outputs scalar behavior/hashes only. Synthetic probes do not run a retail game,
mutate saves, or establish a complete cutscene/message/transition execution.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE/'tools'), str(SOURCE/'docs/research_01415')]
from build_stage_catalog import iter_resources
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA

def sha(data): return hashlib.sha256(data).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path); p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path); a=p.parse_args()
    image=a.exe.read_bytes(); assert sha(image)==SUPPORTED_EXE
    base=struct.unpack_from('<I',image,24)[0]
    overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN')
        if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay)==OVERLAY_SHA
    def cpu():
        c=OverlayProbe(image,base,overlay); c.r[29]=0x8000; return c
    def block(name,start,end):
        data=(overlay[start-0x80100000:end-0x80100000] if start>=0x80100000
              else image[start-base+2048:end-base+2048])
        assert len(data)==end-start
        return {'id':name,'start':hex(start),'end_exclusive':hex(end),'sha256':sha(data)}
    gate_cases=0
    for current in (-1,0,1):
      for x in (-1537,-1536,-1535,-1,0,1023,1024):
       for z in (-8193,-8192,-8191,-7169,-7168):
        c=cpu(); c.r[4]=0x1000; c.put(0x1008,1,1); c.put(0x1014,4,0x3000);c.put(0x101c,1,current)
        row=struct.pack('<hhbbBBI',2,1,-3,-16,5,2,0x4000)
        for i,v in enumerate(row):c.put(0x3000+i,1,v)
        c.put(0x300c,2,65535); c.put(0x801224ed,1,255)
        c.put(0x800b51c4,2,x);c.put(0x800b51c8,2,z)
        c.run(0x80111810)
        expected=current!=1 and 0<=x//512+3<5 and 0<=z//512+16<2
        assert (c.get(0x1008,1)==2)==expected
        assert c.get(0x1018,4)==(0x3000 if expected else 0)
        gate_cases+=1
    palette_cases=0
    output_index, mode=struct.unpack_from('<HH',overlay,0x1cf38); assert mode==1
    for frame in range(5):
        c=cpu();c.r[4]=0x1000;c.put(0x1014,4,0x3000);c.put(0x1018,4,0x4000)
        c.put(0x100c,1,4);c.put(0x100d,1,frame)
        for i in range(16):c.put(0x3000+2*i,2,0);c.put(0x4000+2*i,2,31|(15<<5)|(7<<10))
        c.run(0x80119580)
        expected=0x8000|(31*frame//4)|((15*frame//4)<<5)|((7*frame//4)<<10)
        assert all(c.get(0x800ad168+2*(output_index+i),2)==expected for i in range(16))
        palette_cases+=1
    flag_cases=0
    for predicate in (0,1,7,8,1599,1600,1601,2047):
      for enabled in (False,True):
        c=cpu();c.r[4]=predicate;c.put(0x800be378+(predicate>>3),1,(128>>(predicate&7)) if enabled else 0)
        c.run(0x8001da58);assert c.r[2]==int(enabled);flag_cases+=1
    # Only the final state7 transition branch. Three prior update consumers and
    # two cleanup calls return through synthetic hooks; completion is a premise.
    c=cpu();c.r[16]=0x1000;c.put(0x1000,1,1);c.put(0x800a8a72,2,0)
    for address in (0x8010102c,0x8001ec0c,0x8001f070,0x80042314,0x8001d494):c.hook(address)
    c.put(0x800be378+(1540>>3),1,128>>(1540&7))
    c.run(0x80101248,0x80101310)
    assert [c.get(0x800c3560+2*i,2,True) for i in range(3)]==[2822,-1,-4833]
    assert c.get(0x800c356e,1)==0 and c.get(0x800c356f,1)==1 and c.get(0x800c356c,1)==2
    assert c.get(0x1000,1)==0 and not c.get(0x800be378+(1540>>3),1)&(128>>(1540&7))
    result={'schema':'worldalive-event-source/1','status':'SOURCE_BACKED_PARTIAL',
      'exe_sha256':sha(image),'overlay_sha256':sha(overlay),
      'correction':{'supersedes':['docs/validation_01413/object-initializer-chain.json:first_imported_record.selected_script',
        'docs/validation_01413/object-initializer-chain.json:first_imported_record.command_base'],
        'record':'area0 classC0/subtype03','consumer':'0x80119398',
        'role':'RGB555 palette timeline','storage':'0x800AD168 color halfword buffer',
        'fields':'four-byte palette u16 index/duration/signed control records',
        'proof':'0x80119580 interpolates R/G/B five-bit channels in sixteen halfwords, sets bit15',
        'runtime_audit':'No C0/03 inferred-command execution is wired; p2 event VM is an independent synthetic instruction API'},
      'checks':{'source_spatial_gate_cases':gate_cases,'source_palette_interpolation_cases':palette_cases,
        'source_flag_get_cases':flag_cases,'conditional_transition_branch_cases':1},
      'controller':{'static_record':'area1 C0/subtype0x21 index5','callback':'0x80119BE8',
        'fixed_allocator':'0x800317AC explicit E0 slot allocator',
        'allocation_order':'try slot31 then descend on failure; slot31 only on fresh available pool','created_class':'E0','created_subtype':0,
        'area_descriptor_table':'0x8011A46C','gate':'0x80111810','creator':'0x80111CD0',
        'phase_order':'state1 selects work+18, writes state2; state2 creates on next dispatcher tick',
        'predicate':'u32 record+20; 0x8001DA58 true skips allocation',
        'commit':'state2 stores qualifier lowbyte towork+1C, id lowbyte towork+1D, state1 towork+08',
        'cleanup_scope':'source tracks only its conditional allocations in pairs at0x801224EC; original group switching/removed-object flag side effects are distinct from other static actors'},
      'actual_event_path':{'poll':'0x8001D990','per_stage_poll_table':'0x80088FA8',
        'ST00_poll':'0x80100598','ST00_flag_table':'0x8011ADF8',
        'initial_cutscene_trigger':{'flag':1540,'event_id':1},'event_initialize':'0x8001DDE4',
        'handler_table':'0x80089024','event1_handler':'0x801010C4',
        'event1_state_table':'0x80100018','per_actor_command_interpreter':'0x8001EC0C (highbyte opcode)',
        'phase_timing_consumer':'0x8001F070 (eight-byte records)',
        'state7_transition_if_busy_zero':{'target_stage':0,'target_area':1,'position':[2822,-1,-4833],
          'yaw':0,'transition_status':2,'clear_flag':1540,'event_active':False},
        'message_queue':'0x80063BC8 -> 0x80063C30 -> 0x80053A08; source message payload0x80153000',
        'limits':['No complete event1 cutscene or message sequence execution asserted',
          'Final transition branch tested under completion premise, with update/cleanup hooks',
          'No C0 event-to-message-to-transition chain is fabricated; C0/03 is palette behavior',
          'Undecoded commands remain UNKNOWN; EBD is a resource directory, not interpreter storage']},
      'code_regions':[block('palette_timeline',0x80119398,0x80119800),
        block('C0_21_controller_seed',0x80119be8,0x80119c68),
        block('E0_spatial_gate',0x80111810,0x80111cd0),block('E0_creator',0x80111cd0,0x80112148),
        block('story_flag_get',0x8001da58,0x8001da8c),block('event_poll',0x80100598,0x80100614),
        block('event1_handler',0x801010c4,0x80101364),block('actor_command_interpreter',0x8001ec0c,0x8001f070)],
      'payload_policy':'Scalar behavior, selected addresses and hashes only; no owner bytes or original tables'}
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result['checks']))
if __name__=='__main__':main()

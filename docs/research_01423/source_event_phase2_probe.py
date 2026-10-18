#!/usr/bin/env python3
"""Unchanged phase2 sound/command instructions, authored safe owners only.

No original code/table/audio/asset bytes are exported. Fixtures contain authored
inputs and measured byte changes. Cold LOAD data are checked, but not emitted.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
WORK,VOICES,COMMANDS=0x800C4C48,0x800A33E0,0x800A3A40
DESCRIPTORS,WRITE,CURRENT=0x80098AF8,0x800989A4,0x80098A84
PENDING,ACTIVE=0x800C5680,0x8009AE40

def pattern(size,seed):return bytes((i*37+seed*11)&255 for i in range(size))
def sha(blob):return hashlib.sha256(blob).hexdigest()
def diff(before,after):return ','.join(f'{i}:{v}' for i,v in enumerate(after) if v!=before[i]) or '-'
def put(c,address,blob):
    for i,v in enumerate(blob):c.put(address+i,1,v)
def get(c,address,size):return bytes(c.get(address+i,1) for i in range(size))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','dat','fixture','proof'):ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args()
    if a.fixture.exists() or a.proof.exists():raise SystemExit('Refusing to overwrite phase2 evidence')
    image=a.exe.read_bytes();base=struct.unpack_from('<I',image,24)[0]
    assert sha(image)==SUPPORTED_EXE
    overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN')
                 if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay)==OVERLAY_SHA
    rows=[];steps=0;sound_calls=0;stops=0;aliases=0
    cases=list(itertools.product(range(8),(0,1,7,14),(0,1,7,14)))
    for seed,(kind,count,current) in enumerate(cases):
        c=OverlayProbe(image,base,overlay);c.r[29]=0x700000;c.r[4]=WORK
        work=bytearray(pattern(168,seed+1));work[0]=work[1]=1;work[4]=2;put(c,WORK,work)
        descriptors=bytearray(pattern(32,seed+2));keys=[]
        for i in range(4):
            # Cases include all skip, scalar, pointed, duplicate owners, and
            # signed-ID/selector skip independent of retained descriptor tail.
            identity=i if kind!=5 else 0
            selector=(255 if kind==0 else 0 if kind in (1,5) else 1 if kind==2 else
                      (0,1,255,128)[i] if kind in (3,7) else 255 if kind==4 else 0)
            if kind==6 and i%2==0:identity=-1
            program=0 if selector==0 else (i*61+seed)%256
            struct.pack_into('<h',descriptors,i*8,identity);descriptors[i*8+2]=selector;descriptors[i*8+4]=program
            keys.append((identity,program if selector else 0))
        put(c,DESCRIPTORS,descriptors)
        # Four keyed owners; duplicate descriptor keys share the same source
        # memory and native owner. No pointer identity is invented at runtime.
        sequence_addresses=[];sequence_before=[]
        for i,(identity,program) in enumerate(keys):
            address=0x200000+max(0,identity)*0x10000+program*176
            sequence_addresses.append(address)
            for bank in range(4):c.put(0x800C4CF0+bank*4,4,0x200000+bank*0x10000)
            blob=pattern(176,seed+10+i)
            if address not in sequence_addresses[:-1]:put(c,address,blob)
        sequence_before=[get(c,address,176) for address in sequence_addresses]
        voice_count=(0,1,7,24)[seed%4];c.put(0x800BC820,1,voice_count)
        voices=bytearray(pattern(24*54,seed+20))
        for voice in range(24):
            identity,program=keys[voice%4]
            key=(identity&65535)|program<<8
            struct.pack_into('<H',voices,voice*54+16,key&65535)
        put(c,VOICES,voices);c.put(0x800C0BFA,2,0x1234)
        masks=tuple(((seed*(17+i*13))^(0xA55A>>i))&65535 for i in range(4))
        for address,values in ((PENDING,masks[:2]),(ACTIVE,masks[2:])):
            for i,value in enumerate(values):c.put(address+i*2,2,value)
        c.put(0x800BC050,4,1) # direct742C8 ignores phase1 wrapper lock
        c.put(0x800B5168,2,0x4567)
        queue=bytearray(pattern(256,seed+30))
        for i in range(16):struct.pack_into('<I',queue,i*16,2 if i<count else 0)
        put(c,COMMANDS,queue);c.put(WRITE,4,COMMANDS+count*16);c.put(CURRENT,4,COMMANDS+current*16)
        lookup_id=(seed*0x12345+0x54321)&0xffffffff
        pair0=(seed*0x7654321+0x03000000)&0xffffffff;pair1=(seed*0x812345+0x4567)&0xffffffff
        # Only authored pointer/table data are replaced. Instructions execute
        # unchanged, including packet loads after writes for current/write alias.
        c.put(0x80088F9C+8,4,0x10000);c.put(0x10000,4,lookup_id)
        c.put(0x80088EF8+8,4,0x10020);c.put(0x10020,4,0x10040)
        c.put(0x10040+24,4,pair0);c.put(0x10040+28,4,pair1)
        c.run(0x801010C4)
        assert not c.return_only_hooks
        expected_sound=sum(struct.unpack_from('<h',descriptors,i*8)[0]!=-1 and descriptors[i*8+2]!=255 for i in range(4))
        targets=[call['target'] for call in c.calls]
        assert targets.count('0x8006ff90')==expected_sound
        assert targets.count('0x8001d58c')==1 and c.get(WORK+4,1)==3
        actual_stops=targets.count('0x800742c8')
        actual_masks=[c.get(address+i*2,2) for address in (PENDING,ACTIVE) for i in range(2)]
        rows.append([seed,voice_count,count,current,lookup_id,pair0,pair1,descriptors.hex(),*masks,
                     expected_sound,actual_stops,c.get(0x800C0BFA,2),*actual_masks,c.get(0x800B5168,2),
                     c.get(0x800989BC,4),c.get(0x800989C0,4),diff(work,get(c,WORK,168)),
                     diff(queue,get(c,COMMANDS,256)),diff(voices,get(c,VOICES,24*54)),
                     *(diff(before,get(c,address,176)) for before,address in zip(sequence_before,sequence_addresses))])
        sound_calls+=expected_sound;stops+=actual_stops;aliases+=current==count;steps+=c.steps
    init_rows=[]
    for seed in range(16):
        c=OverlayProbe(image,base,overlay);c.r[29]=0x700000
        descriptors=pattern(32,seed+2);queue=pattern(256,seed+30)
        put(c,DESCRIPTORS,descriptors);put(c,COMMANDS,queue)
        c.run(0x80019F0C,0x80019F40);c.run(0x8001B44C,0x8001B4B4)
        assert not c.return_only_hooks
        init_rows.append([seed,diff(descriptors,get(c,DESCRIPTORS,32)),diff(queue,get(c,COMMANDS,256)),
                          c.get(WRITE,4)-COMMANDS,c.get(CURRENT,4)-COMMANDS])
        steps+=c.steps
    boot=[]
    for address,size in ((DESCRIPTORS,32),(COMMANDS,256)):
        data=image[2048+address-base:2048+address-base+size]
        assert len(data)==size and not any(data)
        boot.append({'address':hex(address),'bytes':size,'all_zero':True,'sha256':sha(data),
                     'scope':'cold executable LOAD only; source resets retain unmodified fields'})
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text('seed voicecount count current lookupid pair0 pair1 descriptors masks0 masks1 masks2 masks3 soundcalls stopcalls aftervoice afterpending0 afterpending1 afteractive0 afteractive1 afterhalfword capture8 captureC workdiff queuediff voicediff seq0diff seq1diff seq2diff seq3diff\n'+
                         ''.join('\t'.join(map(str,row))+'\n' for row in rows)+
                         '#initializer\n'+''.join('\t'.join(map(str,row))+'\n' for row in init_rows))
    def region(lo,hi):
        blob=overlay[lo-0x80100000:hi-0x80100000] if lo>=0x80100000 else image[2048+lo-base:2048+hi-base]
        return {'start':hex(lo),'end_exclusive':hex(hi),'sha256':sha(blob)}
    proof={'schema':'mml-source-event-phase2/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),'overlay_sha256':sha(overlay),
           'cases':len(rows),'initializer_cases':len(init_rows),'sound_calls':sound_calls,'voice_stops':stops,
           'current_write_alias_cases':aliases,'original_instructions_executed':steps,'accepted_cases_helper_hooks':[],
           'fixture_sha256':sha(a.fixture.read_bytes()),'boot_regions':boot,
           'regions':[region(lo,hi) for lo,hi in ((0x8001B33C,0x8001B3E4),(0x80070114,0x80070168),
                      (0x8006FF90,0x80070114),(0x80075314,0x800753B0),(0x800725BC,0x800725CC),
                      (0x8001D58C,0x8001D648),(0x80019F0C,0x80019F40),(0x8001B44C,0x8001B4B4))],
           'limits':['Authored descriptor/sequence/voice/table/queue inputs; no live retail state or chronology claim.',
                     'Source masks and retained byte changes only; no SPU/sample/audio timing claim.',
                     'Type4 dispatch8001B89C still requires CD/control callbacks; no completionbit20 writer admitted.',
                     'Native queue reserves a source type0 sentinel; overflow/missing owners hold atomically.',
                     'No original code/table/assets/saves/sample bytes exported.'],
           'retail_parity':False,'PHYSICS_1TO1':False,'hardware_tested':False}
    a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({key:proof[key] for key in ('cases','initializer_cases','sound_calls','voice_stops','current_write_alias_cases','original_instructions_executed')}))
if __name__=='__main__':main()

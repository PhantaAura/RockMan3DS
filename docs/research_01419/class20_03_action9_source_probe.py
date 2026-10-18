#!/usr/bin/env python3
"""Bounded action9 evidence on authored work; original owner inputs read only.

No original code/listings/tables are exported. Angle, motion, wall, floor and
cue consumers are explicit authored premises. No native implementation here.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

WORK = 0x1000
ENTRY = 0x80103B58
END = 0x80103DF8
HELPERS = (0x80031198, 0x8002FEA4, 0x8004B414, 0x80048E6C, 0x8001A050)
FIELDS = [(9,1,False),(10,1,False),(11,1,False),(14,1,False),(172,1,False),
          (808,2,False),(812,2,True),(816,2,True),(86,2,False),(876,2,True),(878,2,False)]
for offset,width,signed in ((20,2,True),(28,4,True),(44,2,True),(52,4,True)):
    FIELDS += [(offset+i*width,width,signed) for i in range(4)]


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Action9Probe(OverlayProbe):
    def __init__(self,image,base,overlay):
        super().__init__(image,base,overlay)
        self.active_pc = None
        self.stores = []
        self.arguments = []

    def get(self,address,size,signed=False):
        if size == 4 and address in HELPERS:
            self.arguments.append({'entry':hex(address),'arguments':self.r[4:8]})
        if size == 4 and ENTRY <= address < END:
            self.active_pc = address
        return super().get(address,size,signed)

    def put(self,address,size,value):
        if self.active_pc is not None and WORK <= address < WORK+0x3C0:
            self.stores.append({'site':hex(self.active_pc),'work_offset':hex(address-WORK),
                                'bytes':size,'authored_context_value':value & ((1 << (size*8))-1)})
        super().put(address,size,value)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--dat',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose NEW owner-local evidence outside source')
    image=args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise ValueError('Unsupported owner executable')
    base=struct.unpack_from('<I',image,24)[0]
    overlay=next(r['payload'] for r in iter_resources(args.dat/'ST00.BIN')
                 if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise ValueError('Unsupported owner overlay')
    cases=[]
    specs=[(phase,flags,distance,768,137,-7,0,0,0,0) for phase,flags,distance in itertools.product(
        (0,1,2,255),(0,16),(0,0x11FFF,0x12000,0x11FFFF,0x120000,0x480000,0x480001,0xFFFFFFFF))]
    specs += [(1,16,0x480001,target,heading,-7,0,0,0,0) for target,heading in
              ((0,4095),(4095,0),(0,65535),(65535,0))]
    specs += [(1,16,0x12000,768,137,floor,contact,cue,timer35c,timer35e)
              for floor,contact,cue,timer35c,timer35e in
              ((25,0,0,0,0),(26,0,0,0,0),(-39,0,0,0,0),(-40,0,0,0,0),
               (-7,0x100,0,0,0),(-7,0x1000,0,0,0),(-7,0,1,0,0),
               (-7,0,-1,0,0),(-7,0,0,1,0),(-7,0,0,0,1))]
    instructions=0
    for phase,flags,distance,target,player_heading,floor,contact,cue,timer35c,timer35e in specs:
        cpu=Action9Probe(image,base,overlay)
        cpu.r[4],cpu.r[29],cpu.r[31]=WORK,0x8000,0
        for offset,size,value in ((0,1,1),(1,1,0x20),(2,1,3),(9,1,9),(10,1,phase),
            (11,1,19),(14,1,4),(86,2,4095),(128,4,distance),(166,1,cue),(172,1,7),
            (808,2,flags),(812,2,5),(816,2,22),(860,2,timer35c),(862,2,timer35e)):
            cpu.put(WORK+offset,size,value)
        for offset,width,values in ((20,2,(101,-7,-202,1234)),(28,4,(101<<16,-7<<16,-202<<16,1234567)),
            (44,2,(101,-7,-202,4321)),(52,4,(101<<16,-7<<16,-202<<16,7654321))):
            for i,value in enumerate(values):cpu.put(WORK+offset+i*width,width,value)
        cpu.put(0x800B5206,2,player_heading)
        for address in HELPERS:
            result=target if address==0x80031198 else floor if address==0x80048E6C else contact if address==0x8004B414 else 0
            cpu.hook(address,result)
        before=[cpu.get(WORK+off,size,signed) for off,size,signed in FIELDS]
        cpu.run(ENTRY)
        assert cpu.get(WORK+878,2)==((target-player_heading)&0xFFFF)
        calls=[call['target'] for call in cpu.calls]
        assert '0x8007fe50' not in calls and '0x801041b0' not in calls
        if phase in (0,1) and distance>0x480000 and flags&16:
            assert calls==['0x80031198']
            assert cpu.get(WORK+9,1)==3 and cpu.get(WORK+10,2)==0
            if phase==0:
                assert cpu.get(WORK+172,1)==1 and cpu.get(WORK+812,2)==90 and cpu.get(WORK+816,2)==0
        if phase not in (0,1):
            assert calls==['0x80031198'] and cpu.get(WORK+9,1)==9
        instructions+=cpu.steps
        cases.append({'authored_input':{'phase':phase,'flags328':flags,'work80':distance,
            'target_angle':target,'player_heading800B5206':player_heading,'floor':floor,
            'contact':contact,'cueA6':cue,'timer35C':timer35c,'timer35E':timer35e},
            'before':before,'after':[cpu.get(WORK+off,size,signed) for off,size,signed in FIELDS],
            'instructions':cpu.steps,'original_calls':cpu.calls,'helper_arguments':cpu.arguments,
            'source_field_stores':cpu.stores,'return_only_hooks':cpu.return_only_hooks})
    result={'schema':'mml-class03-action9-source/1','status':'SOURCE_BACKED_PARTIAL',
        'exe_sha256':sha(image),'overlay_sha256':sha(overlay),'entry':hex(ENTRY),
        'code_end_exclusive':hex(END),'code_region_sha256':sha(overlay[ENTRY-0x80100000:END-0x80100000]),
        'case_count':len(cases),'instructions':instructions,'field_layout':FIELDS,'cases':cases,
        'native_reducer_implemented':False,'retail_parity':False,'hardware_tested':False,
        'limits':['All work and angle/floor/contact/cue inputs are authored scalar premises.',
                  'Original turn80031BEC executes; target-angle getter is an authored return hook.',
                  'Original8002FEA4 motion producer is not exercised; its hook makes no movement.',
                  'Full callback BIOS sample, detection producer, hit delivery and common tail are excluded.']}
    args.out.mkdir(parents=True,exist_ok=False)
    path=args.out/'source-evidence.json'
    with path.open('x') as file:file.write(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'cases':len(cases),'instructions':instructions,'status':result['status'],
                      'evidence_sha256':sha(path.read_bytes()),'native_implementation':False}))


if __name__=='__main__':main()

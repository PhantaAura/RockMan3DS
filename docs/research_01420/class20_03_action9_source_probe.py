#!/usr/bin/env python3
"""Original action9 reducer and independent forward52 contact evidence.

Owner inputs are read only. Export authored scalar fixtures and hashes, never
original instruction listings, tables, resources or owner positions/trig.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE/'docs/research_01415'), str(SOURCE/'docs/research_01417'),
               str(SOURCE/'docs/research_01418')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
from class20_03_callback_source_probe import authored_store_hook, signed16, signed32
from contact_source_probe import synthetic_stage

WORK, BOUNDS, ENTRY = 0x1000, 0x2000, 0x80103b58
AUTHORED_BOUNDS = [-60,60,-60,60,-128,0]
SCALAR = [(3,1,False),(9,1,False),(10,1,False),(11,1,False),(14,1,False),
          (172,1,False),(808,2,False),(812,2,True),(816,2,True),(86,2,False),
          (102,2,True),(876,2,True),(878,2,False),(860,2,True),(862,2,True),(880,4,False)]
FIELDS = SCALAR + [(off+i*width,width,signed) for off,width,signed in
                   [(20,2,True),(28,4,True),(44,2,True),(52,4,True),(84,2,False)] for i in range(4)]


def sha(data): return hashlib.sha256(data).hexdigest()


class Probe(OverlayProbe):
    def __init__(self,image,base,overlay,player_to_actor,actor_to_player):
        super().__init__(image,base,overlay)
        self.angles = [player_to_actor,actor_to_player]
        self.angle_index = 0
        self.helper_arguments = []

    def get(self,address,size,signed=False):
        if size==4 and address==0x80031198:
            authored_store_hook(self,address,[],self.angles[min(self.angle_index,1)])
            self.angle_index += 1
        if size==4 and address in (0x80031198,0x8002fea4,0x8004b414,0x80048e6c,0x8001a050):
            self.helper_arguments.append((address,self.r[4:8]))
        return super().get(address,size,signed)


def state(c): return [c.get(WORK+off,width,signed) for off,width,signed in FIELDS]


def seed(c,phase,flags,distance,heading,cue,timer35c,timer35e):
    for off,width,value in [(0,1,3),(1,1,0x20),(2,1,3),(3,1,0x5a),(8,1,1),(9,1,9),
                            (10,1,phase),(11,1,19),(14,1,7),(172,1,22),(808,2,flags),
                            (812,2,-7),(816,2,43),(86,2,heading),(102,2,-23),
                            (876,2,-5),(878,2,0x1234),(860,2,timer35c),(862,2,timer35e),
                            (880,4,0x12345678),(128,4,distance),(166,1,cue),(116,1,1),(120,4,BOUNDS)]:
        c.put(WORK+off,width,value)
    for off,width,values in [(20,2,[101,-7,-202,1234]),(28,4,[101<<16,-7<<16,-202<<16,1234567]),
                             (44,2,[81,-7,-182,4321]),(52,4,[81<<16,-7<<16,-182<<16,7654321]),
                             (84,2,[100,heading,300,400]),(68,2,[321,-654,987])]:
        for i,value in enumerate(values):c.put(WORK+off+i*width,width,value)
    for i,value in enumerate(AUTHORED_BOUNDS):c.put(BOUNDS+i*2,2,value)
    c.put(0x80098938,4,0x9000)
    c.r[4],c.r[29],c.r[31]=WORK,0x700000,0


def reducer_cases(image,base,overlay):
    cases=[]
    distances=(0,0x11fff,0x12000,0x11ffff,0x120000,0x480000,0x480001,0xffffffff)
    angles=((768,768,137,4095),(0,0,4095,0),(4095,4095,0,4095),
            (65535,16,65535,0),(1024,15,0,0),(1024,16,0,0))
    specs=[(phase,flags,distance,*angle,-7,0,0,*timers) for phase,flags,distance,angle,timers in
           itertools.product((0,1,2,255),(0,16,17),distances,angles,((0,0),(1,0),(0,1)))]
    specs += [(phase,flags,0x12000,768,0,137,0,floor,contact,cue,0,1)
              for phase,flags,floor,contact,cue in itertools.product(
                  (0,1),(0,16),(-40,-39,-7,25,26,32767,32768,65535),
                  (0,1,0x100,0xf00,0x1000,0x80000000),(0,1,-1,2))]
    instructions=0
    for phase,flags,distance,player_angle,turn_angle,player_heading,heading,floor,contact,cue,t35c,t35e in specs:
        c=Probe(image,base,overlay,player_angle,turn_angle)
        seed(c,phase,flags,distance,heading,cue,t35c,t35e)
        c.put(0x800b5206,2,player_heading)
        raw=[111,-7,-212,1234];fixed=[111<<16,-7<<16,-212<<16,1234567]
        authored_store_hook(c,0x8002fea4,[(4,20+i*2,2,v) for i,v in enumerate(raw)]+
                            [(4,28+i*4,4,v) for i,v in enumerate(fixed)])
        authored_store_hook(c,0x8004b414,[],contact)
        authored_store_hook(c,0x80048e6c,[],floor)
        before=state(c);c.run(ENTRY);after=state(c)
        calls=[entry['target'] for entry in c.calls]
        sound=176 if '0x8001a050' in calls else -1
        words=[c.get(0x9004,4),c.get(0x9008,4)] if sound>=0 else [0,0]
        moving=phase<=1 and not(distance>0x480000 and flags&16)
        if moving:
            assert c.angle_index==2
            assert next(args for address,args in c.helper_arguments if address==0x8002fea4)[1:]==[52,0,0]
            assert next(args for address,args in c.helper_arguments if address==0x80031198)[:2]==[0x800b51c4,WORK+20]
            if sound>=0:
                assert words==[(raw[0]&65535)|((raw[1]&65535)<<16),(raw[2]&65535)|((raw[3]&65535)<<16)]
        else:
            assert c.angle_index==1
        moved_heading=c.get(WORK+86,2)
        # Preparation carries heading before body floor/contact transitions.
        rows=[distance,player_angle,player_heading,turn_angle,cue,1,*raw,*fixed,floor,contact,moved_heading,
              *before,*after,sound,*words]
        cases.append(rows);instructions+=c.steps
    return cases,instructions


def motion_state(c):
    return [c.get(WORK+20+i*2,2,True) for i in range(4)]+\
           [c.get(WORK+28+i*4,4,True) for i in range(4)]+\
           [c.get(WORK+68+i*2,2,True) for i in range(3)]+[c.get(WORK+116,1)]


def preparation(c,raw,fixed,phase,flags,distance,heading,target,cue,trig,bounds):
    seed(c,phase,flags,distance,heading,cue,1,1)
    for off,width,values in [(20,2,raw),(28,4,fixed),(44,2,raw),(52,4,fixed)]:
        for i,value in enumerate(values):c.put(WORK+off+i*width,width,value)
    for i,value in enumerate(bounds):c.put(BOUNDS+i*2,2,value)
    c.put(0x800b5206,2,137)
    delta=[-trig[0]*52,0,-trig[1]*52]
    authored_store_hook(c,0x80030058,[(4,i*4,4,value) for i,value in enumerate(delta)])
    c.run(ENTRY,0x80103c60)
    assert next(args for address,args in c.helper_arguments if address==0x8002fea4)[1:]==[52,0,0]
    sound=176 if any(entry['target']=='0x8001a050' for entry in c.calls) else -1
    if sound>=0:
        live=motion_state(c)[:4]
        assert c.get(0x9004,4)==((live[0]&65535)|((live[1]&65535)<<16))
        assert c.get(0x9008,4)==((live[2]&65535)|((live[3]&65535)<<16))
    return [c.get(WORK+880,4),signed32(c.r[2]),*motion_state(c),
            c.get(WORK+86,2),c.get(WORK+876,2,True),sound,c.get(0x9004,4),c.get(0x9008,4)]


def contact_cases(image,base,overlay,resources):
    rows,private,steps=[],[],0
    points=[(-180,-60,0),(180,-60,0),(0,-60,-180),(0,-60,180),
            (-120,-60,0),(0,-60,0),(0,-128,0),(0,256,0)]
    for phase,trig,point,code,cue in itertools.product((0,1),((0,4096),(4096,0),(-3072,2048)),points,(1,3,8),(0,1)):
        c=Probe(image,base,overlay,768,2048);synthetic_stage(c,1,code)
        raw=list(point)+[1234];fixed=[v*65536+(12345 if i!=1 else 0) for i,v in enumerate(raw)]
        result=preparation(c,raw,fixed,phase,16,0x12000,4095,2048,cue,trig,AUTHORED_BOUNDS)
        rows.append([phase,16,0x12000,4095,2048,cue,*trig,code,0,0,*AUTHORED_BOUNDS,*raw,*fixed,321,-654,987,1,*result]);steps+=c.steps
    selected={}
    for name,load in [('idx',0x8015c000),('mdt',0x80164000),('stg',0x80194000),('hed',0x8013a000)]:
        selected[name]=next(r for r in resources if r['type']==0 and r['load']==load)
    idx,mdt,stg,hed=[selected[name]['payload'] for name in ('idx','mdt','stg','hed')]
    pointers=[]
    for slot in range(512):
        ptr=struct.unpack_from('<I',hed,slot*8+4)[0]
        if not ptr:break
        pointers.append(ptr&0xffffff)
    admitted=[]
    for identity in range(len(idx)//12):
        ptr=struct.unpack_from('<I',idx,identity*12+8)[0]
        if not ptr or ptr&0xffffff not in pointers:continue
        header=struct.unpack_from('<I',stg,ptr-0x80194000+24)[0]
        admitted.append((identity,struct.unpack_from('<H',stg,header-0x80194000+2)[0]))
    assert len(admitted)==994
    sx,sz,width,height,cells=struct.unpack_from('<4BI',mdt,12)
    bounds=list(struct.unpack_from('<6h',overlay,0x1bed4))
    for record,phase,target,cue in itertools.product((0x8011a274,0x8011a28c,0x8011a2a4),(0,1),(0,2048),(0,1)):
        c=Probe(image,base,overlay,768,target)
        for resource in selected.values():
            for off,value in enumerate(resource['payload']):
                c.put(resource['load']+off,1,value);c.put((resource['load']&0xffffff)+off,1,value)
        for identity,seed in admitted:
            c.put(0x8015c006+identity*12,2,seed);c.put(0x15c006+identity*12,2,seed)
        for address,size,value in [(0x1f800040,1,sx),(0x1f800041,1,sz),(0x1f800044,2,width),
            (0x1f800046,2,height),(0x1f800048,4,cells),(0x1f80004c,4,0x8015c000)]:c.put(address,size,value)
        raw=list(struct.unpack_from('<4h',overlay,record-0x80100000+12))
        fixed=[v*65536+(8192 if i!=1 else 0) for i,v in enumerate(raw)]
        heading=(struct.unpack_from('<H',overlay,record-0x80100000+18)[0]<<4)&65535
        t=Probe(image,base,overlay,target,target);t.r[4:8]=[WORK+20,0x800b51c4,heading,32];t.r[29]=0x700000;t.run(0x80031bec)
        moved_heading=(heading+t.r[2])&4095
        trig=struct.unpack_from('<2h',image,0x80092ec4-base+2048+moved_heading*4)
        result=preparation(c,raw,fixed,phase,16,0x12000,heading,target,cue,trig,bounds)
        private.append([phase,16,0x12000,heading,target,cue,*bounds,*raw,*fixed,321,-654,987,1,*result]);steps+=c.steps+t.steps
    return rows,private,steps


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
    a=p.parse_args()
    if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output directory')
    image=a.exe.read_bytes();assert sha(image)==SUPPORTED_EXE;base=struct.unpack_from('<I',image,24)[0]
    resources=list(iter_resources(a.dat/'ST00.BIN'))
    overlay=next(r['payload'] for r in resources if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(overlay)==OVERLAY_SHA
    reducer,steps=reducer_cases(image,base,overlay)
    contact,owner,contactsteps=contact_cases(image,base,overlay,resources);steps+=contactsteps
    a.out.mkdir(parents=True,exist_ok=False)
    outputs={}
    for name,rows,header in [('action9-authored.tsv',reducer,'work80 player_to_actor playerheading actor_to_player cue motionpresent preparedraw4 preparedfixed4 floor contact preparedheading before36 after36 cueid cuewords2'),
                            ('action9-contact-authored.tsv',contact,'phase flags work80 heading target cue sin cos code flags raw6 bounds6 raw4 fixed4 velocity3 state74 wall floor afterraw4 afterfixed4 aftervelocity3 afterstate74 afterheading turn cueid cuewords2'),
                            ('action9-contact-owner.tsv',owner,'phase flags work80 heading target cue ownerbounds6 raw4 fixed4 velocity3 state74 wall floor afterraw4 afterfixed4 aftervelocity3 afterstate74 afterheading turn cueid cuewords2')]:
        f=a.out/name;f.write_text(header+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows));outputs[name]={'sha256':sha(f.read_bytes()),'rows':len(rows)}
    evidence={'schema':'mml-class03-action9-native-source/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),'overlay_sha256':sha(overlay),
              'reducer_cases':len(reducer),'authored_contact_cases':len(contact),'owner_contact_cases':len(owner),'instructions_executed':steps,'fixtures':outputs,
              'source_regions':[{'start':hex(lo),'end_exclusive':hex(hi),'sha256':sha((overlay if lo>=0x80100000 else image[2048:])[lo-(0x80100000 if lo>=0x80100000 else base):hi-(0x80100000 if lo>=0x80100000 else base)])}
                  for lo,hi in [(0x80103b58,0x80103df8),(0x8002fea4,0x8002ff34)]],
              'motion_call_arguments':[52,0,0],'distinct_angle_argument_orders':{'first':['player800B51C4','actorwork14'],'turn':['actorwork14','player800B51C4']},
              'sound_position':'Original176 producer/8001A050 queue writer copy post-wall raw14..1B before floor stores or full-lane rollback.',
              'external_premises':['Reducer differential uses authored motion/wall/floor hooks and distinct authored angle results. Original turn, body and sound queue writer execute.',
                  'Contact differential hooks only angle and80030058 fixed/GTE delta; original8002FEA4 accumulator/raw conversion, wall/floor, turn and sound queue writer execute.',
                  'GTE delta is authored from declared trig or private resident owner trig. No live runtime or retail parity claim.'],
              'retail_parity':False,'hardware_tested':False}
    (a.out/'source-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps({'reducer_cases':len(reducer),'contact_cases':len(contact),'owner_contact_cases':len(owner),'instructions':steps}))

if __name__=='__main__':main()

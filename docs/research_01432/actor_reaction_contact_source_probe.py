#!/usr/bin/env python3
"""Original reaction motion/rounding/wall wrapper on authored geometry.
GTE delta is an explicit boundary; original motion argument narrowing,
accumulation, raw conversion and complete wall instructions execute.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01417'),str(SOURCE/'docs/research_01418'),str(SOURCE/'docs/research_01419'),str(SOURCE/'docs/research_01422')]
from tutorial_actor_identity_probe import SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_callback_source_probe import authored_store_hook,signed16
from class60_06_source_probe import FastProbe
from contact_source_probe import synthetic_stage
from actor_action3_contact_source_probe import seed_work,state,AUTHORED_BOUNDS,WORK,BOUNDS
TABLES=(0x8011bee0,0x8011beec,0x8011bef8)
class Probe(FastProbe):
    def __init__(self,im,base,ov,trig):
        super().__init__(im,base,ov);self.trig=trig;self.motion=None;self.gte=None
    def get(self,a,n,signed=False):
        if n==4 and a==0x8002fea4 and self.motion is None:
            self.motion=[signed16(self.r[5]),signed16(self.r[6])]
        if n==4 and a==0x80030058 and self.gte is None:
            forward,vertical=signed16(self.r[6]),signed16(self.r[7]);self.gte=[forward,vertical]
            delta=[-self.trig[0]*forward,vertical*4096,-self.trig[1]*forward]
            authored_store_hook(self,a,[(4,i*4,4,v) for i,v in enumerate(delta)])
        return super().get(a,n,signed)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path);a=p.parse_args()
    if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output')
    im=a.exe.read_bytes();assert hashlib.sha256(im).hexdigest()==SUPPORTED_EXE;base=struct.unpack_from('<I',im,24)[0]
    ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(ov).hexdigest()==OVERLAY_SHA
    rows=[];steps=0
    for action,param,context,trig,point,code in itertools.product((7,8),(0,64),((0,19,0),(1,-1,8),(2,32767,1)),
            ((0,4096),(4096,0),(-3072,2048)),((-180,-60,0),(180,-60,0),(0,-60,-180),(0,-60,180),(0,-128,0),(0,256,0)),(1,3,8)):
        phase,timer,aux=context;c=Probe(im,base,ov,trig);synthetic_stage(c,1,code)
        raw=list(point)+[1234];fixed=[v*65536+(12345 if i!=1 else 0) for i,v in enumerate(raw)]
        seed_work(c,raw,fixed,phase,timer,0,4095)
        for o,w,v in [(9,1,action),(3,1,param),(11,1,aux),(110,2,128),(814,2,0),
                      (868,4,TABLES[2 if param&64 else 1])]:c.put(WORK+o,w,v)
        for row,address in enumerate(TABLES):
            for i in range(12):c.put(address+i,1,(row*53+i*17+3)&255)
        c.put(0x80098844,4,0x50000);c.put(0x5000c,4,0x50100);c.put(0x5010b,1,231)
        for i,v in enumerate(AUTHORED_BOUNDS):c.put(BOUNDS+i*2,2,v)
        c.r[4],c.r[29],c.r[31]=WORK,0x700000,0
        c.run(0x80103654 if action==7 else 0x8010388c,0x80103750 if action==7 else 0x80103a04)
        assert c.motion==c.gte
        rows.append([action,phase,aux,param,timer,*trig,code,*raw,*fixed,321,-654,987,1,*c.motion,c.r[2],*state(c)])
        steps+=c.steps
    blob=('\n'.join(' '.join(map(str,r)) for r in rows)+'\n').encode();a.out.mkdir(parents=True)
    (a.out/'contact-authored.tsv').write_bytes(blob)
    report={'schema':'mml-reaction-contact-source/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':SUPPORTED_EXE,'overlay_sha256':OVERLAY_SHA,
            'case_count':len(rows),'instructions_executed':steps,'fixture_sha256':hashlib.sha256(blob).hexdigest(),
            'retail_payload_exported':False,'limits':['Authored GTE deltas and authored geometry only; original narrowing/motion/rounding/wall execute.',
                     'Native unsupported geometry and map boundaries remain held; no original floor helper is called by these reaction branches.']}
    (a.out/'source-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()

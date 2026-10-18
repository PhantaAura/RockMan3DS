#!/usr/bin/env python3
"""Original reaction actions7/8, authored tables/motion/wall fixtures only.
Owner input is immutable and hash checked. Original control/timer/turn/audio
instructions run. No original instructions, owner table values or assets export.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01417'),str(SOURCE/'docs/research_01422')]
from tutorial_actor_identity_probe import SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_callback_source_probe import authored_store_hook,signed16
from class60_06_source_probe import FastProbe
WORK=0x1000
TABLES=(0x8011bee0,0x8011beec,0x8011bef8)
FIELDS=[(9,1,False),(10,1,False),(11,1,False),(172,1,False),(808,2,False),
        (812,2,True),(816,2,True),(810,2,False),(110,2,True),
        (68,2,True),(70,2,True),(72,2,True),(868,4,False),(880,4,False),
        (86,2,False),(876,2,True)]
FIELDS += [(20+i*2,2,True) for i in range(4)]+[(28+i*4,4,True) for i in range(4)]
class Probe(FastProbe):
    def __init__(self,im,base,ov):
        super().__init__(im,base,ov);self.motion=None;self.wall_velocity=None
    def get(self,a,n,signed=False):
        if n==4 and a==0x8002fea4 and self.motion is None:
            self.motion=[signed16(self.r[5]),signed16(self.r[6])]
        if n==4 and a==0x8004b414 and self.wall_velocity is None:
            self.wall_velocity=[super(Probe,self).get(WORK+68+i*2,2,True) for i in range(3)]
        return super().get(a,n,signed)
def seed(c,spec):
    action,phase,aux,timer,flags,param,hp,random,table,vx,vy,vz,heading,angle,contact=spec
    for o,w,v in [(1,1,0x20),(2,1,3),(3,1,param),(9,1,action),(10,1,phase),
                  (11,1,aux),(172,1,251),(808,2,flags),(812,2,timer),(816,2,731),
                  (810,2,random),(110,2,hp),(68,2,vx),(70,2,vy),(72,2,vz),
                  (868,4,table),(880,4,0xa5a5f00d),(86,2,heading),(876,2,-117)]:c.put(WORK+o,w,v)
    for i,v in enumerate((101,-7,-202,1234)):c.put(WORK+20+i*2,2,v)
    for i,v in enumerate((123456,-76543,999,0x13579)):c.put(WORK+28+i*4,4,v)
    for row,address in enumerate(TABLES):
        for i in range(12):c.put(address+i,1,(row*53+i*17+3)&255)
    c.put(0x80098844,4,0x3000);c.put(0x300c,4,0x4000);c.put(0x400b,1,231)
    c.put(WORK+814,2,0);c.put(0x80098938,4,0x9000);c.hook(0x80031198,angle)
    raw=(111,-17,-212,5678);fixed=(456789,-1234567,7654321,987654)
    authored_store_hook(c,0x8002fea4,[(4,20+i*2,2,v) for i,v in enumerate(raw)]+[(4,28+i*4,4,v) for i,v in enumerate(fixed)])
    authored_store_hook(c,0x8004b414,[(4,68+i*2,2,v) for i,v in enumerate((321,-654,987))],contact)
    c.r[4],c.r[29],c.r[31]=WORK,0x700000,0
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path);a=p.parse_args()
    if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output')
    im=a.exe.read_bytes();assert hashlib.sha256(im).hexdigest()==SUPPORTED_EXE
    base=struct.unpack_from('<I',im,24)[0]
    ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(ov).hexdigest()==OVERLAY_SHA
    specs=[]
    for action,phase,aux,timer,param,hp,contact in itertools.product((7,8),(0,1,2),(0,1,8,9),
            (-32768,-1,0,1,127,255,32767),(0,64),(-1,0,128),(0,1,0xf00,0x1000,0x1f00,0x80000000)):
        specs.append((action,phase,aux,timer,0x200e,param,hp,0x1234,TABLES[2 if param&64 else 1],-32768,17,19,4095,1024,contact))
    for action,timer,heading,angle in itertools.product((7,8),(-32768,-1,0,1,127,255,32767),
            (0,1023,1024,2048,4095,65535),(0,1023,1024,2048,4095,65535)):
        specs.append((action,3,8,timer,0x200e,0,128,0x1234,0,321,17,19,heading,angle,0))
    for action,phase in itertools.product((7,8),(4,255)):
        specs.append((action,phase,255,-32768,0x200e,0,128,0x1234,0,321,17,19,4095,0,0))
    rows=[];steps=0;sounds=0;recoveries=0
    for spec in specs:
        c=Probe(im,base,ov);seed(c,spec);c.run(0x80103654 if spec[0]==7 else 0x8010388c)
        after=[c.get(WORK+o,w,s) for o,w,s in FIELDS]
        sound=222 if any(v['target']=='0x8001a050' for v in c.calls) else -1
        words=[c.get(0x9004,4),c.get(0x9008,4)] if sound>=0 else [0,0]
        m=[1,*c.motion,*c.wall_velocity] if c.motion else [0]*6
        assert c.get(WORK+0x33c,4)==0
        rows.append([*spec,*m,*after,sound,*words]);steps+=c.steps;sounds+=sound>=0
        recoveries+=after[1]==3 and spec[1]!=3
    blob=('\n'.join(' '.join(map(str,r)) for r in rows)+'\n').encode();a.out.mkdir(parents=True)
    (a.out/'authored.tsv').write_bytes(blob)
    report={'schema':'mml-class03-reaction-source/1','status':'SOURCE_BACKED_PARTIAL',
            'exe_sha256':SUPPORTED_EXE,'overlay_sha256':OVERLAY_SHA,
            'entries':['0x80103654','0x8010388C'],'case_count':len(rows),'instructions_executed':steps,
            'sound222_cases':sounds,'recovery_transitions':recoveries,'fixture_sha256':hashlib.sha256(blob).hexdigest(),
            'field_layout':FIELDS,'retail_payload_exported':False,'retail_parity':False,
            'limits':['Timer table bytes and combat byte0B are authored premises, not exported owner values.',
                      'Motion and wall are authored store boundaries. Original turn helper and positional sound writer execute.',
                      'No native live packet clear, Buster/camera input, effect/defeat lifecycle or complete encounter claim.']}
    (a.out/'source-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()

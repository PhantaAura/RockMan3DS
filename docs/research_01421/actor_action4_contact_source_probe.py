#!/usr/bin/env python3
"""Original negative/dynamic/positive action4 motion/contact preparation proof.
GTE outputs are authored. Original phase/timer, accumulator, rounding, wall,
floor and positional cue177 writer execute. No owner payloads in fixture.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01417'),str(SOURCE/'docs/research_01418'),str(SOURCE/'docs/research_01419'),str(SOURCE/'docs/research_01420')]
from tutorial_actor_identity_probe import SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_action4_child_source_probe import Probe
from class20_03_callback_source_probe import authored_store_hook,signed32
from contact_source_probe import synthetic_stage
from actor_action3_contact_source_probe import seed_work,state,AUTHORED_BOUNDS,WORK,BOUNDS

def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output')
 im=a.exe.read_bytes();assert sha(im)==SUPPORTED_EXE;base=struct.unpack_from('<I',im,24)[0]
 ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(ov)==OVERLAY_SHA
 rows=[];steps=0
 cases=[(0,19,-112),(1,1,-112),(1,-32768,-112),(3,1,0),(3,2,-32),(3,12,-352),(3,0,224),(4,1,224),(4,-32768,224)]
 for context,trig,point,code in itertools.product(cases,((0,4096),(4096,0),(-3072,2048)),((-180,-60,0),(180,-60,0),(0,-60,-180),(0,-60,180),(0,-128,0),(0,256,0)),(1,3,8)):
  phase,timer,forward=context;heading=4095;c=Probe(im,base,ov);synthetic_stage(c,1,code)
  raw=list(point)+[1234];fixed=[v*65536+(12345 if i!=1 else 0) for i,v in enumerate(raw)]
  seed_work(c,raw,fixed,phase,timer,0,heading);c.put(WORK+9,1,4);c.put(WORK+172,1,21);c.put(WORK+814,2,0);c.put(0x80098938,4,0x9000)
  for i,v in enumerate(AUTHORED_BOUNDS):c.put(BOUNDS+i*2,2,v)
  delta=[-trig[0]*forward,0,-trig[1]*forward];authored_store_hook(c,0x80030058,[(4,i*4,4,v) for i,v in enumerate(delta)])
  c.r[4],c.r[29],c.r[31]=WORK,0x700000,0
  stop=0x801031b4 if forward==224 else 0x80103090;c.run(0x80102fc0,stop)
  # Capture actual original calls; never rely only on our authored branch premise.
  actual=[args[1] for addr,args in c.arguments if addr=='0x8002fea4'];assert len(actual)==1
  actual=(actual[0]&65535)-65536 if actual[0]&32768 else actual[0]&65535;assert actual==forward
  wall=next((args for addr,args in c.arguments if addr=='0x8004b414'),None)
  # Probe arguments intercept onlyselectedfamily; wall result owns work but
  # action4 does not store it at370. Synthetic contact core return is recovered
  # by matching original wrapper's last r2 before its floor call separately.
  # Instrument floor entry saves incoming v0 to scratch using r8/9 only.
  c2=Probe(im,base,ov);synthetic_stage(c2,1,code);seed_work(c2,raw,fixed,phase,timer,0,heading)
  c2.put(WORK+9,1,4);c2.put(WORK+172,1,21);c2.put(WORK+814,2,0);c2.put(0x80098938,4,0x9000)
  for i,v in enumerate(AUTHORED_BOUNDS):c2.put(BOUNDS+i*2,2,v)
  authored_store_hook(c2,0x80030058,[(4,i*4,4,v) for i,v in enumerate(delta)])
  c2.r[4],c2.r[29],c2.r[31]=WORK,0x700000,0
  floorentry=0x801031ac if forward==224 else 0x80103088;c2.run(0x80102fc0,floorentry)
  wallresult=c2.r[2];floorresult=signed32(c.r[2]);after=state(c)
  rows.append([phase,timer,heading,forward,*trig,code,0,0,*AUTHORED_BOUNDS,*raw,*fixed,321,-654,987,1,wallresult,floorresult,*after])
  steps+=c.steps+c2.steps
 a.out.mkdir(parents=True);(a.out/'contact-authored.tsv').write_text('# phase timer heading forward trig2 code flags raw6 bounds6 before raw4 fixed4 vel3 state wall floor after raw4 fixed4 vel3 state\n'+''.join(' '.join(map(str,r))+'\n' for r in rows))
 result={'schema':'mml-action4-contact-source/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(im),'overlay_sha256':sha(ov),'authored_cases':len(rows),'original_instructions':steps,
 'distinct_motion_forwards':sorted(set(row[3] for row in rows)),'limits':['GTE delta boundary authored; original action4 timers/motion accumulator/rawconversion/wall/floor execute.','No child consumer executes or becomes ready. Unsupported native geometry must remain held.','No live retail or hardware parity claim.']}
 (a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()

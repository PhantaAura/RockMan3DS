#!/usr/bin/env python3
"""Bounded original child contact/registry/shadow instruction differentials.
Every exported byte is an authored scalar input/result. Owner files read-only;
retail executable/overlay code and data are never emitted.
"""
import argparse,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01422'),str(SOURCE/'docs/research_01418')]
from class60_06_source_probe import FastProbe,full_result,sha,SUPPORTED_EXE,OVERLAY_SHA,iter_resources,WORK,FX,SIZE,store
from contact_source_probe import synthetic_stage
class ContactProbe(FastProbe):
 def get(self,a,n,signed=False):
  if a==0x8010fdf4 and n==4:self.wall_result=self.r[2]
  return super().get(a,n,signed)
def contact_rows(im,base,ov):
 rows=[];steps=0;corrected=0
 movements=[((0,-127,0),(0,-200,0)),((0,129,0),(0,256,0)),
 ((-99,-60,0),(-200,-60,0)),((99,-60,0),(200,-60,0)),
 ((0,-60,-99),(0,-60,-200)),((0,-60,99),(0,-60,200)),
 ((-99,-60,-99),(-200,-60,-200)),((0,-60,0),(0,-60,0))]
 for code,flags,state,move,vy,allocation in itertools.product((1,3,15,8),(0,0x8000),(0,1),movements,(-32768,-2560,4096),(0,FX)):
  c=ContactProbe(im,base,ov);synthetic_stage(c,1,code,flags,0)
  for i in range(SIZE):c.put(WORK+i,1,(i*13+7)&255)
  for off,width,value in [(0,1,3),(1,1,0x61),(2,1,6),(0x74,1,state),(0x78,4,0)]:c.put(WORK+off,width,value)
  raw=list(move[0])+[1234];previous=list(move[1])+[-444]
  store(c,20,2,raw);store(c,28,4,[v*65536+12345 for v in raw])
  store(c,44,2,previous);store(c,52,4,[v*65536+891 for v in previous])
  store(c,68,2,[321,vy,-987]);store(c,76,2,[7,512,-3]);c.put(0x800c1b60,1,0)
  c.put(0x80098938,4,0x9000);full_result(c,0x80031618,allocation)
  for i in range(72):c.put(FX+i,1,(i*17+3)&255)
  before=c.blob(WORK,SIZE);c.r[4],c.r[29],c.r[31]=WORK,0x700000,0;c.run(0x8010fdd4)
  wall=c.wall_result;after=c.blob(WORK,SIZE);sound=int(any(x['target']=='0x8001a050' for x in c.calls))
  effect=int(allocation!=0 and wall!=0);fx=c.blob(FX,72) if effect else [0]*72
  cue=[c.get(0x9000+i*4,4) for i in range(3)]
  rows.append([code,flags,0,state,allocation,*before,*after,wall,c.r[2],sound,*cue,effect,*fx]);steps+=c.steps;corrected+=wall!=0
 return rows,steps,corrected
def consumer_rows(im,base,ov):
 registry=[];shadow=[];steps=0
 for count,flags,contact in itertools.product((0,1,127),(0,1,2,3,4,255),(0,0xffff)):
  c=FastProbe(im,base,ov);identity=0x8009cf88;c.put(identity,1,flags);c.put(identity+124,2,contact)
  c.put(0x80098898,4,0x9000+count*4);c.r[4],c.r[31]=identity,0;c.run(0x80051bac)
  registry.append([count,flags,contact,c.get(0x80098898,4),c.get(identity+124,2),c.get(0x9000+count*4,4)]);steps+=c.steps
 for count,wy,floor,scale,type_,radius in itertools.product((0,63,64,0xffffffff),(-128,0,127),(-32769,-128,32767),(512,-65),(0,3),(60,65535)):
  c=FastProbe(im,base,ov);c.put(WORK+20,2,-101);c.put(WORK+22,2,wy);c.put(WORK+24,2,202)
  c.put(WORK+86,2,0xabcd);c.put(WORK+118,1,0);c.put(0x800987d8,4,count)
  full_result(c,0x80048e6c,floor&0xffffffff);c.r[4:8]=[WORK,radius,scale&0xffffffff,type_];c.r[29],c.r[31]=0x700000,0;c.run(0x8002c8cc)
  record=[c.get(0x800ace00+count*12+i*2,2) for i in range(6)] if count<64 else [0]*6
  shadow.append([count,wy,floor,scale,type_,radius,c.get(0x800987d8,4),*record]);steps+=c.steps
 return registry,shadow,steps
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--dat',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose new private output')
 im=a.exe.read_bytes();assert sha(im)==SUPPORTED_EXE;base=struct.unpack_from('<I',im,24)[0]
 ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(ov)==OVERLAY_SHA
 rows,steps,corrected=contact_rows(im,base,ov);registry,shadow,cs=consumer_rows(im,base,ov);a.out.mkdir(parents=True)
 for name,values in [('contact',rows),('registry',registry),('shadow',shadow)]:
  (a.out/(name+'-authored.tsv')).write_text('# authored original scalar premises/results\n'+''.join(' '.join(map(str,r))+'\n' for r in values))
 result={'schema':'mml-child-contact-consumers/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(im),'overlay_sha256':sha(ov),'contact_cases':len(rows),'contact_nonzero_source_results':corrected,'registry_cases':len(registry),'shadow_cases':len(shadow),'original_instructions':steps+cs,
 'limits':['Authored geometry executes original8004B414/8004B6AC/8010FDD4/reflection/audio writes. Native unknown geometry remains held.','Effect allocation premise authored; original effect131 primitive copy executes; its later callback not proved.','Registry append only; pair contact80051BE4 not covered. Native128 capacity is an explicit safety budget.','Shadow scalar floor receipt authored; original recordpack executes. Neither shadow rasterizer nor GTE/fog proved.','No live retail chronology or hardware parity claim.']}
 (a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()

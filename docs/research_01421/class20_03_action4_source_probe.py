#!/usr/bin/env python3
"""Original action4 reducer/pool proof with explicit authored consumer boundaries.
No owner instructions or assets are serialized. Unknown child callback is not stubbed
into a runtime readiness claim. Output fixtures are authored scalar premises only.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01417'),str(SOURCE/'docs/research_01420')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_callback_source_probe import authored_store_hook,signed32
from class20_03_action4_child_source_probe import Probe,seed_parent,POOL,BITMAP,WORK
FIELDS=[(9,1,False),(10,1,False),(172,1,False),(812,2,True),(816,2,True),(808,2,False),(86,2,False),(11,1,False),(814,2,True),(860,2,True),(880,4,False),(884,4,False)]
FIELDS += [(o+i*w,w,sgn) for o,w,sgn in [(20,2,True),(28,4,True),(44,2,True),(52,4,True),(84,2,False)] for i in range(4)]
def state(c):return [c.get(WORK+o,w,s) for o,w,s in FIELDS]
def sha(b):return hashlib.sha256(b).hexdigest()
def bodies(image,base,overlay):
 rows=[];steps=0
 for phase,timer,variant,request,floor,child in itertools.product((0,1,2,3,4,5,255),(-32768,-1,0,1,2,12,13),(0,1),(21,255),(-40,-7,25,26),(0,POOL)):
  c=Probe(image,base,overlay);seed_parent(c,variant,4095)
  for off,width,value in [(10,1,phase),(812,2,timer),(172,1,request),(816,2,731),(808,2,0x2010),(880,4,0x98765432)]:c.put(WORK+off,width,value)
  # Every observed mutating field must match, including complete fourth lanes.
  for i,v in enumerate((81,-7,-182,4321)):c.put(WORK+44+i*2,2,v);c.put(WORK+52+i*4,4,v*65536+567)
  for i,v in enumerate((6,4095,7,8)):c.put(WORK+84+i*2,2,v)
  c.put(0x80098844,4,0x3000);c.put(0x300c,4,0x4000);c.put(WORK+2,1,3);c.put(0x400c+variant*16,1,231)
  raw=[111,-17,-212,5678];fixed=[v*65536+12345 for v in raw]
  authored_store_hook(c,0x8002fea4,[(4,20+i*2,2,v) for i,v in enumerate(raw)]+[(4,28+i*4,4,v) for i,v in enumerate(fixed)])
  authored_store_hook(c,0x8004b414,[],0xf00);authored_store_hook(c,0x80048e6c,[],floor)
  authored_store_hook(c,0x80104078,[(4,884,4,child)],0)
  before=state(c);c.run(0x80102fc0);after=state(c)
  motion=[a[1] for addr,a in c.arguments if addr=='0x8002fea4'];assert len(motion)<=1
  forward=((motion[0]&65535)-65536 if motion[0]&32768 else motion[0]&65535) if motion else 0
  sound=177 if any(call['target']=='0x8001a050' for call in c.calls) else -1
  soundwords=[c.get(0x9004,4),c.get(0x9008,4)] if sound>=0 else [0,0]
  childcalled=int(any(call['target']=='0x80104078' for call in c.calls))
  rows.append([int(bool(motion)),forward,floor,child,231,*raw,*fixed,*before,*after,sound,*soundwords,childcalled]);steps+=c.steps
 return rows,steps

def pools(image,base,overlay):
 rows=[];steps=0
 for slot,variant,heading,trig in itertools.product((0,1,15,31,None),(0,1,-1),(0,1024,4095),((0,4096),(4096,0),(-3072,2048))):
  c=Probe(image,base,overlay);raw,fixed=seed_parent(c,variant,heading)
  mask=0xffffffff if slot is None else 0xffffffff^(0x80000000>>slot);c.put(BITMAP,4,mask)
  pointer=0 if slot is None else POOL+slot*0x10c
  if pointer:
   for off in range(0x10c):c.put(pointer+off,1,(off*17+3)&255)
  delta=[-trig[0]*1760,0,-trig[1]*1760];authored_store_hook(c,0x80030058,[(4,i*4,4,v) for i,v in enumerate(delta)])
  c.run(0x80104078)
  stores=list(bytes(c.get(pointer+i,1) for i in range(0x10c))) if pointer else [0]*0x10c
  sound=178 if any(call['target']=='0x8001a050' for call in c.calls) else -1
  words=[c.get(0x9004,4),c.get(0x9008,4)] if sound>=0 else [0,0]
  rows.append([mask,variant,heading,*trig,*raw,*fixed,c.get(WORK+884,4),c.get(BITMAP,4),sound,*words,*stores]);steps+=c.steps
 # Original pending-release helper + batch allocator-bit clear retain pending.
 release=[]
 for slot,occupied,pending,mask in itertools.product((0,1,15,31),(0,0xffffffff),(0,0x80100001),(0,0xffffffff)):
  c=Probe(image,base,overlay);pointer=POOL+slot*0x10c;c.put(BITMAP,4,occupied);c.put(0x80098808,4,pending);c.put(pointer,1,0xff)
  c.r[4],c.r[29],c.r[31]=pointer,0x700000,0;c.run(0x800318d8)
  afterpending=c.get(0x80098808,4);afterbyte=c.get(pointer,1)
  c.r[4]=c.r[5]=c.r[7]=c.r[8]=c.r[9]=c.r[10]=c.r[11]=0;c.r[6]=mask;c.r[31]=0;c.run(0x80031b3c)
  release.append([slot,occupied,pending,mask,afterpending,afterbyte,c.get(BITMAP,4)]);steps+=c.steps
 return rows,release,steps

def dispatch(image,base,overlay):
 c=Probe(image,base,overlay);c.r[29]=0x700000;c.run(0x80100324)
 table=c.get(0x1f800060,4);entries={str(i):hex(c.get(table+i*4,4)) for i in (6,9)}
 # Execute original pool updater with only authored callback returns to prove
 # it selects stage-local table for <128, resident table for >=128.
 callbacks=[]
 for subtype in (6,9,128,134):
  p=Probe(image,base,overlay);p.r[29]=0x700000;p.put(0x1f800060,4,table)
  for i in range(32):p.put(POOL+i*0x10c,1,0)
  p.put(POOL,1,1);p.put(POOL+6,1,3);p.put(POOL+2,1,subtype)
  target=c.get(table+subtype*4,4) if subtype<128 else p.get(0x80089820+subtype*4,4)
  if target:p.hook(target)
  p.run(0x80017210)
  callbacks.append({'subtype':subtype,'actual_callback':p.indirect[0]['target'] if p.indirect else None,'null_dispatch':target==0})
 return {'initializer':'0x80100324','scratch_owner':'0x1f800060','stage_table':hex(table),'actual_source_entries':entries,
         'pool_dispatch_cases':callbacks,'corrects':'01420 report incorrectly indexed resident table80089820 for subtype6/9; source uses it only for subtype>=128.'}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output')
 im=a.exe.read_bytes();assert sha(im)==SUPPORTED_EXE;base=struct.unpack_from('<I',im,24)[0]
 ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(ov)==OVERLAY_SHA
 b,bs=bodies(im,base,ov);ch,r,cs=pools(im,base,ov);d=dispatch(im,base,ov);a.out.mkdir(parents=True)
 for name,rows in [('action4-authored',b),('child-pool-authored',ch),('child-release-authored',r)]:
  (a.out/(name+'.tsv')).write_text('# authored source differential scalar premises and results\n'+''.join(' '.join(map(str,row))+'\n' for row in rows))
 result={'schema':'mml-action4-source-differential/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(im),'overlay_sha256':sha(ov),
 'action4_cases':len(b),'child_cases':len(ch),'release_cases':len(r),'original_instructions':bs+cs,'dispatch':d,
 'limits':['Action4 motion/wall/floor and child helper are authored body boundaries. Contact verified separately.',
           'Child GTE delta is authored, full original allocator/stores/conversion/sound executes.',
           'Release helper and batch bitmap clear execute; batch pending-mask reset remains outer ownership.',
           'Child lifecycle, collision, resources, rendering and damage remain UNKNOWN; no live retail parity claim.']}
 (a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('action4_cases','child_cases','release_cases','original_instructions','dispatch')}))
if __name__=='__main__':main()

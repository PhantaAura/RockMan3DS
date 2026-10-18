#!/usr/bin/env python3
"""Bounded action4 and child80104078 source research; no runtime semantics invented.

Executes original source allocator, child scalar initializer and body dispatch
with declared authored GTE/consumer boundaries. Owner bytes remain read only.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01417')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_callback_source_probe import authored_store_hook,signed32
WORK,POOL,BITMAP=0x1000,0x8009cf88,0x800987cc

def sha(b):return hashlib.sha256(b).hexdigest()
class Probe(OverlayProbe):
 def __init__(self,image,base,overlay):
  super().__init__(image,base,overlay);self.arguments=[]
 def get(self,address,size,signed=False):
  if size==4 and address in (0x80031518,0x80104078,0x8002fea4,0x8002ffc4,0x80030058,0x8001a050):self.arguments.append((hex(address),self.r[4:8]))
  return super().get(address,size,signed)
def array(c,address,width,values):
 for i,v in enumerate(values):c.put(address+i*width,width,v)
def seed_parent(c,variant,heading):
 c.r[4],c.r[29],c.r[31]=WORK,0x700000,0
 for off,width,val in [(9,1,4),(10,1,2),(11,1,19),(172,1,21),(812,2,1),(814,2,variant),(860,2,7),(880,4,0),(884,4,0x12345678),(86,2,heading)]:c.put(WORK+off,width,val)
 raw=[101,-7,-202,1234];fixed=[v*65536 for v in raw]
 for off,width,values in [(20,2,raw),(28,4,fixed),(44,2,raw),(52,4,fixed)]:array(c,WORK+off,width,values)
 c.put(0x80098938,4,0x9000)
 return raw,fixed
def helper_cases(image,base,overlay):
 cases=[];steps=0
 for slot,variant,heading,trig in itertools.product((0,1,31,None),(0,1,-1,2),(0,1024,4095),((0,4096),(4096,0),(-3072,2048))):
  c=Probe(image,base,overlay);raw,fixed=seed_parent(c,variant,heading)
  mask=0xffffffff if slot is None else 0xffffffff^(0x80000000>>slot);c.put(BITMAP,4,mask)
  pointer=0 if slot is None else POOL+slot*0x10c
  if pointer:
   for off in range(0x10c):c.put(pointer+off,1,(off*17+3)&255)
  delta=[-trig[0]*1760,0,-trig[1]*1760]
  authored_store_hook(c,0x80030058,[(4,i*4,4,v) for i,v in enumerate(delta)])
  c.run(0x80104078)
  assert c.get(WORK+884,4)==pointer
  if pointer:
   assert c.get(BITMAP,4)==0xffffffff and c.get(pointer,1)==1
   assert c.get(pointer+2,1)==(6 if variant==0 else 9) and c.get(pointer+3,1)==(0 if variant==0 else 3)
   expected=[signed32(fixed[0]+delta[0]),signed32(fixed[1]-(60 if variant==0 else 80)*65536),signed32(fixed[2]+delta[2])]
   assert [c.get(pointer+28+i*4,4,True) for i in range(3)]==expected
   assert c.get(pointer+100,2)==heading
   assert c.get(pointer+6,1)==3 and c.get(pointer+16,4)==0 and c.get(pointer+120,4)==0
   if variant==0:assert c.get(pointer+156,4)==32 and c.get(pointer+160,4)==16
   sound=any(call['target']=='0x8001a050' for call in c.calls)
   assert sound==(variant==0)
   if sound:
    assert c.get(0x9002,2)==178
    assert c.get(0x9004,4)==((raw[0]&65535)|((raw[1]&65535)<<16))
    assert c.get(0x9008,4)==((raw[2]&65535)|((raw[3]&65535)<<16))
   args=next(args for entry,args in c.arguments if entry=='0x8002ffc4')
   assert args==[pointer+20,pointer+28,heading,1760]
  else:
   assert c.get(BITMAP,4)==0xffffffff and not any(call['target']=='0x8001a050' for call in c.calls)
  cases.append({'authored_slot':slot,'authored_variant32E':variant,'authored_parent_heading':heading,'authored_trig':trig,
                'child_pointer':hex(pointer),'child_subtype':c.get(pointer+2,1) if pointer else None,
                'child_parameter03':c.get(pointer+3,1) if pointer else None,
                'scalar_stores_verified':pointer!=0,'original_calls':c.calls,'helper_arguments':c.arguments})
  steps+=c.steps
 return cases,steps

def action4_cases(image,base,overlay):
 cases=[];steps=0
 for phase,timer,variant,request,floor in itertools.product((0,1,2,3,4,5,255),(-32768,-1,0,1,2,12,13),(0,1),(21,255),(-7,26)):
  c=Probe(image,base,overlay);seed_parent(c,variant,0)
  for off,width,value in [(10,1,phase),(812,2,timer),(172,1,request)]:c.put(WORK+off,width,value)
  # Context stats pointer with authored unsigned byteC for phase4 expiry.
  c.put(0x80098844,4,0x3000);c.put(0x300c,4,0x4000)
  c.put(WORK+2,1,3);c.put(0x400c+variant*16,1,231)
  authored_store_hook(c,0x8002fea4,[],0);authored_store_hook(c,0x8004b414,[],0)
  authored_store_hook(c,0x80048e6c,[],floor)
  authored_store_hook(c,0x80104078,[],0)
  c.run(0x80102fc0)
  child=any(call['target']=='0x80104078' for call in c.calls)
  if child:assert c.get(WORK+10,1)==3 and c.get(WORK+172,1)==((21 if phase==0 else request)+1)&255 and c.get(WORK+812,2,True)==11
  motion=[args[1] for entry,args in c.arguments if entry=='0x8002fea4']
  assert len(motion)<=1
  cases.append({'authored_phase':phase,'authored_timer32C':timer,'authored_variant32E':variant,'authored_request':request,'authored_floor':floor,
                'after_phase':c.get(WORK+10,1),'after_request':c.get(WORK+172,1),'after_timer32C':c.get(WORK+812,2,True),
                'after_work35C':c.get(WORK+860,2,True),'child_helper_called':child,
                'source_motion_forward_signed':[(v&65535)-65536 if v&32768 else v&65535 for v in motion],
                'source_sound_ids':[c.get(0x9002,2)] if any(call['target']=='0x8001a050' for call in c.calls) else [],'original_calls':c.calls})
  steps+=c.steps
 return cases,steps

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local evidence directory')
 image=a.exe.read_bytes();assert sha(image)==SUPPORTED_EXE;base=struct.unpack_from('<I',image,24)[0]
 overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(overlay)==OVERLAY_SHA
 helpers,steps=helper_cases(image,base,overlay);bodies,bodysteps=action4_cases(image,base,overlay);steps+=bodysteps
 entries={str(slot):hex(struct.unpack_from('<I',image,0x80089820-base+2048+slot*4)[0]) for slot in (6,9)}
 result={'schema':'mml-class03-action4-child-source/1','status':'SOURCE_BACKED_PARTIAL','research_only':True,'native_implementation':False,
         'exe_sha256':sha(image),'overlay_sha256':sha(overlay),'action4_cases':len(bodies),'child_helper_cases':len(helpers),'instructions_executed':steps,
         'allocator':{'entry':'80031518','pool_base':hex(POOL),'capacity':32,'stride':'10C','bitmap':hex(BITMAP),'selection':'first clear bit from80000000 down00000001','exhaustion':'parent374 setzero, no child writes/cue'},
         'resident_child_update_entries':entries,'child_helper_cases_authored':helpers,'action4_cases_authored':bodies,
         'limits':['Source original allocator, child scalar stores, sound178 writer and8002FFC4 accumulator/rounding execute.',
                   'GTE/motion delta, parent work and pool bitmap/child contents are authored. No full child dispatch or combat claim.',
                   'Action4 body uses authored motion/wall/floor and child-helper return boundaries; native reducer not added.',
                   'No source semantics are inferred for subtype6/9 or variant32E beyond proven stores. Damage, lifecycle and resource/render consumers remain UNKNOWN.'],
         'regions':[{'start':hex(lo),'end_exclusive':hex(hi),'sha256':sha((overlay if lo>=0x80100000 else image[2048:])[lo-(0x80100000 if lo>=0x80100000 else base):hi-(0x80100000 if lo>=0x80100000 else base)])}
                    for lo,hi in [(0x80102fc0,0x8010324c),(0x80104078,0x801041b0),(0x80031518,0x80031598),(0x80017210,0x800172e0),(0x8002ffc4,0x80030058)]]}
 a.out.mkdir(parents=True,exist_ok=False);(a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({'action4_cases':len(bodies),'child_cases':len(helpers),'instructions':steps,'child_entries':entries}))
if __name__=='__main__':main()

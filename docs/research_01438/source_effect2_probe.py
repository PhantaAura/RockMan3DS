#!/usr/bin/env python3
"""Unchanged subtype2 scalar consumer with authored data/BIOS-return premises.
No code patches or helper hooks. Original BIOS A0/2F calls stop at their real
external boundary and resume at the original saved RA with authored V0. This
proves consumer arithmetic, not BIOS recurrence, live RNG or retail cadence.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01422'),str(SOURCE/'docs/research_01417')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from class60_06_source_probe import FastProbe,OVERLAY_SHA
FX,ALLOC,PENDING,GP,CALLBACK,COLORS=0x800a3b40,0x80098838,0x800988c8,0x80097864,0x80116e68,0x8011cd74
AUTHORED_COLORS=(0x010305,0x070b11,0x13171d,0x1f2529,0x2b2f35,0x3b3d43,0x47494f,0x535961)
class Probe(FastProbe):
 def __init__(self,image,base,overlay):
  super().__init__(image,base,overlay);self.r[29]=0x700000;self.r[28]=GP;self.track=False;self.reads=set();self.writes=set()
 def get(self,a,n,signed=False):
  if self.track and FX<=a<FX+64*72:self.reads.update(range(a-FX,a-FX+n))
  return super().get(a,n,signed)
 def put(self,a,n,v):
  if self.track and FX<=a<FX+64*72:self.writes.update(range(a-FX,a-FX+n))
  super().put(a,n,v)
def fill(c,p,b):
 for i,v in enumerate(b):c.put(p+i,1,v)
def blob(c,p,n):return bytes(c.get(p+i,1) for i in range(n))
def authored(seed,slot):
 b=bytearray((seed*23+i*17+11)&255 for i in range(72));b[0]=0;b[1]=128|slot;return b
def mask(lanes,slot):return ''.join('1' if slot*72+i in lanes else '0' for i in range(72))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for key in ('exe','overlay','out'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args();out=a.out.resolve()
 if out.exists() or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose NEW private output')
 image=a.exe.read_bytes();owner=OwnerExe(image);ov=a.overlay.read_bytes();assert hashlib.sha256(ov).hexdigest()==OVERLAY_SHA
 rows=[];instructions=0;bios=0;cases=[]
 # Sparse source-owned parent allocations exercise both allocation banks and
 # reuse before/after the parent. Caller dispatch does not read the bitmap.
 for seed,(slot,mode,param,state,timer,tag) in enumerate(itertools.chain(
   itertools.product((0,1,31,32,63),(0,),range(3),(0,),(0,1,4,8,16,17,255),(0,7)),
   itertools.product((0,32),(1,),range(4),range(2),(0,1,8,9,24,255),(0,7)))):
  c=Probe(image,owner.base,ov);pool=[authored(seed+i,i) for i in range(64)]
  parent=pool[slot];parent[0]=1;parent[2]=2;parent[3]=param;parent[6]=3;parent[8]=state;parent[11]=timer;parent[13]=mode;parent[14]=tag
  for i,raw in enumerate((0x7fff,0x8000,0xfff9)):struct.pack_into('<H',parent,28+i*2,raw)
  used=[0,0];used[slot//32]|=0x80000000>>(slot&31);pending=[0,0]
  # Selected cases retain occupied lower slots, forcing allocation bank1.
  if slot==32 and seed&1:used[0]=0xffffffff
  free=next((i for i in range(64)if not(used[i//32]&(0x80000000>>(i&31)))),None)
  child_before=bytes(pool[free]) if free is not None else bytes(72)
  before=bytes(parent)
  for i,w in enumerate(pool):fill(c,FX+72*i,w)
  for i in range(2):c.put(ALLOC+4*i,4,used[i]);c.put(PENDING+4*i,4,pending[i])
  # Authored data words are external inputs, not instruction patches/assets.
  for i,v in enumerate(AUTHORED_COLORS):c.put(COLORS+4*i,4,v)
  rng_choices=((0,0,0),(0x7fff,0x7fff,0x7fff),(0xffffffff,0xffffff00,0xfffffff0),(0x80000000,0x80000000,0x80000000),(0x12345678,0x65432101,0x76543210))
  values=rng_choices[seed%len(rng_choices)];returns=[];c.r[4]=FX+72*slot;c.track=True
  c.run(CALLBACK,0xa0)
  while c.r[10]==0xa0 and len(returns)<3 and c.r[31] in (0x80116f60,0x80116f98,0x80116fe4):
   ra=c.r[31];returns.append(ra);c.r[2]=values[len(returns)-1];c.run(ra,0xa0)
   # After the third return, run() ends at original0; stale register10 remains.
   if len(returns)==3:break
  c.track=False;assert not c.return_only_hooks
  expected_spawn=mode==0 and param<2 and (timer&(3 if param==0 else 7))==0
  assert len(returns)==(3 if expected_spawn and param==0 else 0)
  alloc_after=[c.get(ALLOC+4*i,4)for i in range(2)];pending_after=[c.get(PENDING+4*i,4)for i in range(2)]
  child=free if expected_spawn else -1;after=blob(c,FX+72*slot,72);child_after=blob(c,FX+72*free,72) if expected_spawn else child_before
  rows.append([slot,child,*used,*pending,*values,*AUTHORED_COLORS,before.hex(),child_before.hex(),after.hex(),child_after.hex(),*alloc_after,*pending_after,mask(c.reads,slot),mask(c.writes,slot),mask(c.writes,free if free is not None else 0)])
  instructions+=c.steps;bios+=len(returns);cases.append({'slot':slot,'mode0D':mode,'parameter03':param,'state08':state,'timer11':timer,'tag0E':tag,'child':child,'external_bios_returns':len(returns)})
 fixture=('slot child usedA usedB pendingA pendingB rng0 rng1 rng2 colors0 colors1 colors2 colors3 colors4 colors5 colors6 colors7 before childBefore after childAfter usedAfterA usedAfterB pendingAfterA pendingAfterB parentReads parentWrites childWrites\n'+''.join(' '.join(map(str,r))+'\n'for r in rows)).encode()
 proof={'schema':'mml-source-effect2-scalar/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'callback':hex(CALLBACK),'cases':len(rows),'original_instructions':instructions,'instruction_patches':0,'helper_hooks':0,'external_authored_bios_results':bios,'data_color_table_authored':True,'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'coverage':cases,'limits':['Unchanged original callback, allocator and deferred-release instructions execute; BIOS result boundary is authored, not executed.','Authored color data and work records are external scalar premises, not original assets.','No live chronology, full dispatcher, projection, rendering, original BIOS RNG or physical hardware parity claim.']}
 out.mkdir(parents=True);(out/'callback-authored.tsv').write_bytes(fixture);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({k:proof[k]for k in ('cases','original_instructions','instruction_patches','helper_hooks','external_authored_bios_results')}))
if __name__=='__main__':main()

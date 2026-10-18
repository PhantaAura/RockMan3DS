#!/usr/bin/env python3
"""Unchanged three prop attachment handlers on authored scalar pose banks.
Owner executable/overlay stay unchanged. Restricted MVMVA models only the
actually consumed MAC/IR arithmetic; GPU timing/unused flags are outside scope.
No owner models, poses, palettes, instruction bytes or listings are exported.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01414'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01423')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from actor_pose_proxy_source_probe import Probe,OVERLAY_SHA
PROP,ACTOR=0x800bf118,0x800c3580
class Trace(Probe):
 def __init__(self,image,base,overlay,prop,actor):
  super().__init__(image,base,overlay);self.prop=prop;self.actor=actor;self.track=False;self.reads=set();self.writes=set();self.ir_clips=0
 def cop2(self,word):
  super().cop2(word)
  if (word>>21&31)>=16:
   for raw in self.data2[25:28]:
    value=raw if raw<0x80000000 else raw-0x100000000
    self.ir_clips+=int(value < -32768 or value > 32767)
 def get(self,p,n,signed=False):
  if self.track and self.actor<=p<self.actor+1444:self.reads.update(range(p-self.actor,p-self.actor+n))
  return super().get(p,n,signed)
 def put(self,p,n,v):
  if self.track and self.prop<=p<self.prop+80:self.writes.update(range(p-self.prop,p-self.prop+n))
  super().put(p,n,v)
def fill(c,p,b):
 for i,v in enumerate(b):c.put(p+i,1,v)
def blob(c,p,n):return bytes(c.get(p+i,1)for i in range(n))
def authored(seed,n):return bytearray((seed*37+i*17+11)&255 for i in range(n))
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for k in ('exe','overlay','out'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();out=a.out.resolve()
 if out.exists() or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose NEW private output')
 image=a.exe.read_bytes();owner=OwnerExe(image);ov=a.overlay.read_bytes();assert sha(ov)==OVERLAY_SHA
 matrices=((4096,0,0,0,4096,0,0,0,4096),(-4096,0,0,0,-4096,0,0,0,-4096),(0,-4096,0,4096,0,0,0,0,4096),(32767,)*9,(-32768,32767,-32768,32767,-32768,32767,-32768,32767,-32768),(4095,1,-4097,-1,8193,-3,2047,-2049,17))
 refs=((0,4096,4096,0),(1,4095,4095,1),(-32768,32767,-32768,32767))
 rows=[];steps=gte=0;coverage=[];alias_cases=0;clips=clipped_cases=0
 for seed,(state,matrix,reference,edge,release) in enumerate(itertools.chain(*(itertools.product((state,),matrices,refs,range(4)if state==2 else range(2),range(2)if state==1 else (0,))for state in range(1,4)))):
  slot=(0,14,31)[seed%3];actor_slot=(0,3)[seed%2];prop=PROP+slot*80;actor=ACTOR+actor_slot*1444;c=Trace(image,owner.base,ov,prop,actor)
  before=authored(seed,80);before[0]=1;before[1]=0xe0|slot;before[2]=3;before[3]=0;before[6]=3;before[9]=state;before[12]=release
  pose=authored(seed,1444);pose[1]=0x40|actor_slot;pose[2]=5
  for i,v in enumerate(matrix):struct.pack_into('<h',pose,672+i*2,v)
  points=((32767,-32768,-1),(-32768,32767,1))
  for off in (384,408,416,440):
   for i,v in enumerate(points[(edge+(off//8))%2]):struct.pack_into('<h',pose,off+2*i,v)
  if state==2:
   pairs=(((32767,-32768,-1),(-32768,32767,1)),((32767,-32768,-32768),(32767,-32768,-32767)),((32767,-1,0),(32766,-2,1)),((-32768,32767,-32768),(-32767,32766,32767)))
   for offset,point in zip((416,440),pairs[edge]):
    for i,v in enumerate(point):struct.pack_into('<h',pose,offset+2*i,v)
  fill(c,prop,before);fill(c,actor,pose);c.put(0x800c4c6c,4,actor)
  zsin,zcos,xsin,xcos=reference;c.put(0x80092ec4,4,(zsin&65535)|((zcos&65535)<<16));c.put(0x80093ec4,4,(xsin&65535)|((xcos&65535)<<16))
  delta=(prop-0x800a3b40)&0xffffffff;high=(delta*0x38e38e39)>>32;offset=(high>>9)*4;pending=0x800988c8+offset;used=0x80098838+offset
  pending_before=(seed*0x1234567+0x1020304)&0xffffffff;used_word=(0xf5a5f0f0^(seed*0x32101))&0xffffffff
  c.put(pending,4,pending_before);c.put(used,4,used_word)
  # Authored caller stack, including the12 unowned result-copy lanes. Their
  # resulting bytes remain private/UNKNOWN to native code rather than zeroed.
  fill(c,0x6fff00,authored(seed,256));c.r[4]=prop;c.r[28]=0x80097864;c.r[29]=0x700000;c.track=True
  handler=(0x80112d14,0x80112e80,0x80112f70)[state-1];c.run(handler);c.track=False;assert not c.return_only_hooks
  after=blob(c,prop,80);pending_after=c.get(pending,4);writes=''.join('1'if i in c.writes else'0'for i in range(80));reads=''.join('1'if i in c.reads else'0'for i in range(1444))
  # Public authored output must not preserve arbitrary original-stack bytes.
  public_after=bytearray(after)
  if state==1:public_after[44:56]=before[44:56]
  rows.append([slot,actor_slot,state,*reference,pending,used,pending_before,used_word,pending_after,before.hex(),pose.hex(),public_after.hex(),writes,reads]);steps+=c.steps;gte+=c.gte_commands;alias_cases+=release;clips+=c.ir_clips;clipped_cases+=int(c.ir_clips>0)
  coverage.append({'state':state,'release':bool(release),'prop_slot':slot,'actor_slot':actor_slot,'matrix_clipping_fixture':reference==refs[-1]})
 fixture=('slot actorSlot state zeroSin zeroCos quarterSin quarterCos pendingAddress usedAddress pendingBefore usedWord pendingAfter before actor after writes actorReads\n'+''.join(' '.join(map(str,r))+'\n'for r in rows)).encode()
 proof={'schema':'mml-source-prop-attachment/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'cases':len(rows),'original_instructions':steps,'restricted_mvmva_commands':gte,'instruction_patches':0,'helper_hooks':0,'foreign_release_cases':alias_cases,'clipped_ir_components':clips,'cases_with_ir_clipping':clipped_cases,'fixture_sha256':sha(fixture),'coverage':coverage,'limits':['Authored pose, trigonometry data, globals and stack premises, not a live retail capture.','Restricted MVMVA MAC/IR arithmetic; GTE timing and unused flags are excluded.','State1 unowned12 stack-copy bytes are UNKNOWN, not verified zero.','Foreign80031988 alias writes are preserved only with explicitly bound external mask-word ownership.','No render alpha/blend, event cadence, full scene or hardware parity claim.']}
 out.mkdir(parents=True);(out/'attachment-authored.tsv').write_bytes(fixture);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({k:proof[k]for k in ('cases','original_instructions','restricted_mvmva_commands','foreign_release_cases','clipped_ir_components','cases_with_ir_clipping','fixture_sha256')}))
if __name__=='__main__':main()

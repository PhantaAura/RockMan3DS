#!/usr/bin/env python3
"""Unchanged two-point registration and sampled pair body, authored scalar data.
Opaque earlier player/GTE and timing consumers are outside the selected body;
no helper hooks or owner code patches. Authored unowned scratch is explicit.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01414'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01423')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from actor_pose_proxy_source_probe import Probe
BANK,COUNT,TARGETS=0x800a2f48,0x800988e0,(0x1000,0x1100)
class Trace(Probe):
 def __init__(self,image,base):super().__init__(image,base);self.pair_calls=0
 def get(self,p,n,signed=False):
  if p==0x800346ac and n==4:self.pair_calls+=1
  return super().get(p,n,signed)
def point(p,pad=1234):return (p[0]&65535)|((p[1]&65535)<<16),(p[2]&65535)|((pad&65535)<<16)
def shape(c,target,k,mode,ra,rb,cl,packet,absflag,p0,p1,mask=0,flags=0,pad=17):
 c.r=[0]*32;c.r[29]=0x7000;c.r[28]=0x80097864;c.r[4]=0x2000;c.r[5]=ra;c.r[6]=0x2100 if k else 0;c.r[7]=rb if k else mode<<13
 for addr,p in ((0x2000,p0),(0x2100,p1)):
  w=point(p);c.put(addr,4,w[0]);c.put(addr+4,4,w[1])
 c.put(target,4,0);c.put(target+4,4,flags);c.put(target+8,4,0x12345678);c.put(target+12,4,0x43211234);c.put(0x80098798,4,mask)
 for i in range(24):c.put(0x800be478+4*i,4,0x4000+0x100*i);c.put(0x4000+0x100*i+188,4,0x11112222);c.put(0x4000+0x100*i+192,4,0x33334444)
 if k:
  for off,v in ((16,k<<9),(20,mode<<13),(24,cl),(28,target),(32,packet),(36,absflag),(40,128)):c.put(0x7000+off,4,v)
  c.put(0x6fee,2,pad);c.run(0x8003430c)
 else:
  c.r[6]=0
  for off,v in ((16,cl),(20,target),(24,packet),(28,absflag),(32,128)):c.put(0x7000+off,4,v)
  c.run(0x800341bc)
 return c.r[2]
def fields(c,target):return [c.get(target,4),c.get(target+4,4),*[c.get(target+8+2*i,2,True)for i in range(3)],c.get(target+14,2)]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--exe',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);a=ap.parse_args();out=a.out.resolve()
 if out.exists()or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose NEW private output')
 im=a.exe.read_bytes();owner=OwnerExe(im);rows=[];steps=0
 for count,mask,flags,kind in ((0,0,0,1),(1,0xffffffff,0x80000080,1),(2,0xffffffff,0x00000180,2),(62,0,0,3),(63,0,0,1),(64,0,0,1)):
  c=Trace(im,owner.base);c.put(COUNT,4,count);p0=(-1,32767,-32768);p1=(0,32766,32767);pad=0xa5c3
  rc=shape(c,TARGETS[0],kind,3,32,64,16,0x28000008,0,p0,p1,mask,flags,pad)
  newest=[c.get(BANK+12*(count+i)+off,sz)if rc else 0 for i in range(2)for off,sz in ((0,4),(4,4),(8,4))]
  parallel=[c.get(0x800a32e0+4*(count+i),4)if rc else 0 for i in range(2)]
  rows.append(['R',count,mask,flags,kind,*p0,*p1,pad,rc,c.get(COUNT,4),c.get(TARGETS[0]+4,4),*newest,*parallel,c.get(0x40bc,4),c.get(0x40c0,4),c.get(0x57bc,4),c.get(0x57c0,4)])
  steps+=c.steps;assert not c.return_only_hooks
 specs=[]
 def add(ka,kb,a0,a1,b0,b1,ra=32,rb=32,abs_a=0x800000,abs_b=0,rule=255):specs.append((ka,kb,a0,a1,b0,b1,ra,rb,abs_a,abs_b,rule))
 for kind in (1,2,3):
  add(0,kind,(0,0,0),(0,0,0),(60,60,0),(0,0,0))
  add(0,kind,(0,0,0),(0,0,0),(64,64,0),(64,-64,0))
  add(0,kind,(0,0,0),(0,0,0),(65,0,0),(0,0,0))
 add(0,1,(0,0,0),(0,0,0),(60,60,0),(-60,-60,0),abs_b=0x800000)
 add(1,0,(60,60,0),(0,0,0),(0,0,0),(0,0,0),abs_b=0x800000)
 add(1,1,(0,0,0),(60,0,0),(20,20,0),(-60,0,0),abs_b=0x800000)
 add(0,1,(0,0,0),(0,0,0),(60,60,0),(0,0,0),rule=0)
 add(0,1,(0,0,0),(0,0,0),(4088,4088,4088),(32767,32767,32767),ra=2044,rb=2044)
 add(0,1,(-32768,0,0),(-32768,0,0),(32767,0,0),(0,0,0),ra=2044,rb=2044)
 for ka,kb,a0,a1,b0,b1,ra,rb,abs_a,abs_b,rule in specs:
  c=Trace(im,owner.base);c.put(COUNT,4,0);shape(c,TARGETS[0],ka,0,ra,ra,32,0x08000001,abs_a,a0,a1);shape(c,TARGETS[1],kb,3,rb,rb,16,0x28000008,abs_b,b0,b1)
  count=c.get(COUNT,4);steps+=c.steps;c.steps=0;c.r=[0]*32;c.r[28]=0x80097864;c.r[29]=0x7000;c.r[18]=0xff000000
  for i in range(8):c.put(0x80097d00+i,1,rule)
  scratch=(101,-117,13)
  for i,v in enumerate(scratch):c.put(0x6fd0+2*i,2,v)
  c.run(0x80034548,0x80034568);c.put(0x7010,4,0);c.run(0x80034588,0x8003466c)
  rows.append(['P',ka,kb,*a0,*a1,*b0,*b1,ra,rb,abs_a,abs_b,rule,*scratch,*fields(c,TARGETS[0]),*fields(c,TARGETS[1]),c.get(COUNT,4),c.pair_calls]);steps+=c.steps;assert not c.return_only_hooks
 fixture=('\n'.join(' '.join(map(str,row))for row in rows)+'\n').encode()
 proof={'schema':'mml-source-hit-proxy-composite/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,'registration_cases':6,'pair_cases':len(specs),'original_instructions':steps,'instruction_patches':0,'helper_hooks':0,'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'limits':['Authored records/coordinates/rules and unowned scratch are explicit scalar premises, not live retail encounters.','Earlier player/GTE consumers and clock instrumentation outside selected paired body are unexecuted.','Source coarse first-point AABB, interpolation sampling and signed wrap quirks are preserved.','No genuine firing input, global player pose, original Buster body, defeat/drop or hardware parity claim.']}
 out.mkdir(parents=True);(out/'composite-authored.tsv').write_bytes(fixture);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof))
if __name__=='__main__':main()

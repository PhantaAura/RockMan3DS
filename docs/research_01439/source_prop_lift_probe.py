#!/usr/bin/env python3
"""Unchanged source lift/drop oracle: authored work/curves, selected BIOS rand.
No instruction patches/helpers, original curve bytes/assets or saves exported.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01422'),str(SOURCE/'docs/research_01417')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from class60_06_source_probe import FastProbe,OVERLAY_SHA
BIOS_SHA='713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
E0,ACT,FX,EVENT,STATE,RAND,BIOS_RAND=0x800bf118,0x800c3580,0x800a3b40,0x800c4c48,0x8448,0x8007fe50,0xbfc060e4
CURVES=tuple(bytes((17*s+3*f+5)&255 for f in range(64))for s in range(3))
def sha(b):return hashlib.sha256(b).hexdigest()
def authored(seed,n):return bytes((seed*37+i*17+11)&255 for i in range(n))
def fill(c,a,b):
 for i,v in enumerate(b):c.put(a+i,1,v)
def blob(c,a,n):return bytes(c.get(a+i,1)for i in range(n))
def mask(s,n):return ''.join('1'if i in s else'0'for i in range(n))
class Probe(FastProbe):
 def __init__(self,im,base,ov,bios,prop,actor,peer):
  super().__init__(im,base,ov);self.bios=bios;self.r[29]=0x700000;self.r[28]=0x80097864;self.track=False
  self.windows={'prop':(prop,80),'actor':(actor,1444),'peer':(peer,80),'fx':(FX,4608)};self.reads={k:set()for k in self.windows};self.writes={k:set()for k in self.windows}
 def get(self,a,n,signed=False):
  if self.track:
   for k,(base,size)in self.windows.items():
    if base<=a<base+size:self.reads[k].update(range(a-base,a-base+n))
  if 0xbfc00000<=a and a+n<=0xbfc00000+len(self.bios):
   v=int.from_bytes(self.bios[a-0xbfc00000:a-0xbfc00000+n],'little');return v-(1<<(n*8))if signed and v&(1<<(n*8-1))else v
  return super().get(a,n,signed)
 def put(self,a,n,v):
  if self.track:
   for k,(base,size)in self.windows.items():
    if base<=a<base+size:self.writes[k].update(range(a-base,a-base+n))
  super().put(a,n,v)
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('exe','overlay','bios','out'):p.add_argument('--'+name,required=True,type=Path)
 a=p.parse_args();out=a.out.resolve()
 if out.exists()or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose new private output')
 image,ov,bios=a.exe.read_bytes(),a.overlay.read_bytes(),a.bios.read_bytes();owner=OwnerExe(image);assert sha(ov)==OVERLAY_SHA and sha(bios)==BIOS_SHA
 specs=[]
 for state,sub in[(1,0),(1,4),(2,2),(1,2),(2,0)]:
  for bone in(-32768,-64,0,32767):specs.append((state,sub,0,0,bone,125,0,0,0))
 for selector in range(3):
  for frame in(0,32,63):
   for bone in(-32768,-64,128,32767):
    for before in(-32768,-100,0,32767):specs.append((1,1,selector,frame,bone,before,0,0,0))
 for used0,used1 in[(0,0),(0xffffffff,0),(0xffffffff,0xffffffff),(0xfffffffe,0xffffffff),(0xffffffff,0xfffffffe)]:
  for seed in(0,1,31,0xffffffff):specs.append((1,1,0,1,-64,100,0,used0,used1,seed))
 for state,sub in[(1,3),(2,1)]:
  for fixed in(0,0xffff0000,0xffce0000,0xffcf4000,0xffcf3fff,0x7fffffff,0x80000000,0xffffffff,0xffce0001):specs.append((state,sub,0,0,0,100,fixed,0,0))
 rows=[];steps=services=0;coverage={}
 for case,spec in enumerate(specs):
  state,sub,selector,frame,bone,beforeY,fixed,used0,used1=spec[:9];seed=spec[9]if len(spec)>9 else(case%7)*0x1234567
  ps=(0,2,31)[case%3];acs=case%4;peer_slot=9;prop=E0+80*ps;actor=ACT+1444*acs;peer=E0+80*peer_slot
  c=Probe(image,owner.base,ov,bios,prop,actor,peer);c.r[4]=prop
  before=bytearray(authored(case,80));before[0]=7;before[1]=0xe0|ps;before[2]=3;before[3]=2;before[6]=0x23;before[8]=1;before[9]=state;before[10]=sub;struct.pack_into('<I',before,12,fixed);struct.pack_into('<3H',before,56,0x7fff,beforeY&65535,0x8000)
  ab=bytearray(authored(case+7,1444));ab[1]=0x40|acs;ab[14]=selector;struct.pack_into('<I',ab,152,0x510000);struct.pack_into('<H',ab,442,bone&65535)
  pb=bytearray(authored(case+17,80));pb[1]=0xe0|peer_slot
  fill(c,prop,before);fill(c,actor,ab);fill(c,peer,pb);c.put(0x510000,1,frame);c.put(EVENT+0x24,4,actor);c.put(EVENT+0x4c,4,peer)
  pool=bytearray(authored(case+23,4608))
  for s in range(64):pool[s*72+1]=0x80|s
  fill(c,FX,pool);c.put(0x80098838,4,used0);c.put(0x8009883c,4,used1)
  for s in range(3):
   ptr=0x500000+s*256;c.put(0x8011c9fc+s*4,4,ptr);fill(c,ptr,CURVES[s])
  c.put(STATE,4,seed);c.track=True;calls=[];pc=0x80113280 if state==1 else 0x80113354
  while True:
   c.run(pc,RAND)
   if c.r[31]==0:break
   ra=c.r[31];assert ra in(0x80113238,0x80113e54)and len(calls)<2
   c.run(RAND,0xa0);assert c.r[9]==47;c.run(BIOS_RAND,ra);calls.append((ra,c.r[2]));pc=ra
  c.track=False;assert not c.return_only_hooks
  after=blob(c,prop,80);pa=blob(c,peer,80);masks=(c.get(0x80098838,4),c.get(0x8009883c,4))
  child=next((s for s in range(64)if(masks[s//32]^(used0,used1)[s//32])&(0x80000000>>(s&31))),-1)
  cb=bytes(pool[child*72:(child+1)*72])if child>=0 else bytes(72);ca=blob(c,FX+child*72,72)if child>=0 else bytes(72)
  assert blob(c,actor,1444)==bytes(ab)
  calltext=','.join(f'{ra}:{v}'for ra,v in calls)or'-'
  rows.append([case,ps,acs,state,sub,selector,frame,bone,beforeY,fixed,used0,used1,seed,calltext,before.hex(),after.hex(),pb.hex(),pa.hex(),child,cb.hex(),ca.hex(),*masks,mask(c.reads['prop'],80),mask(c.writes['prop'],80),mask(c.reads['actor'],1444),mask(c.writes['peer'],80),mask({n-child*72 for n in c.writes['fx']if child>=0 and child*72<=n<(child+1)*72},72)])
  steps+=c.steps;services+=len(calls);key=str((state,sub,len(calls),child>=0));coverage[key]=coverage.get(key,0)+1
 fixture=('case propSlot actorSlot state sub selector frame boneY beforeY fixed used0 used1 seed calls before after peerBefore peerAfter child childBefore childAfter out0 out1 propReads propWrites actorReads peerWrites childWrites\n'+''.join(' '.join(map(str,r))+'\n'for r in rows)).encode()
 proof={'schema':'mml-source-prop-lift/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'selected_bios_sha256':BIOS_SHA,'cases':len(rows),'original_instructions':steps,'selected_bios_calls':services,'instruction_patches':0,'helper_hooks':0,'coverage':coverage,'fixture_sha256':sha(fixture),'authored_curve_and_actor_inputs':True,'original_assets_exported':False,'retail_parity':False,'hardware_tested':False,'limits':['Original callback/allocator/selected BIOS rand execute on authored scalar work/curve inputs.','Only data pointers/windows are authored; original instructions are unchanged.','Outer scheduler, BIOS dispatcher, live parent/RNG chronology and renderer are excluded.']}
 out.mkdir(parents=True);(out/'callback-authored.tsv').write_bytes(fixture);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({k:proof[k]for k in('cases','original_instructions','selected_bios_calls','instruction_patches','helper_hooks')}))
if __name__=='__main__':main()

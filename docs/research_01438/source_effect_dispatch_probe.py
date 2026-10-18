#!/usr/bin/env python3
"""Private unchanged MIPS call-boundary and scalar80 dispatcher observations."""
import argparse,json,struct,hashlib,sys
from pathlib import Path
SRC=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
for name in ('exe','overlay','out'):p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();out=a.out.resolve()
if out.exists() or out==SRC or SRC in out.parents:raise ValueError('Choose a new private output directory outside source')
sys.path[:0]=[str(SRC/'tools'),str(SRC/'docs/research_01422'),str(SRC/'docs/research_01417')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from class60_06_source_probe import FastProbe
im=a.exe.read_bytes();owner=OwnerExe(im)
ov=a.overlay.read_bytes()
assert hashlib.sha256(ov).hexdigest()=='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
BASE=0x800a3b40;ST=72;TABLE=0x8011a604;steps=0
class Probe(FastProbe):
 def __init__(self):
  super().__init__(im,owner.base,ov);self.r[29]=0x700000;self.reads=[];self.trace=False
 def get(self,a,n,signed=False):
  if self.trace and n==1 and BASE<=a<BASE+64*ST and (a-BASE)%ST==0:self.reads.append((a-BASE)//ST)
  return super().get(a,n,signed)
def row(c):return {'calls':c.calls,'indirect':c.indirect,'a0':hex(c.r[4]),'ra':hex(c.r[31]),'instructions':c.steps}
world=[]
for site in range(0x80015dbc,0x80015e7c,8):
 w=owner.word(site);assert w>>26==3;target=(site&0xf0000000)|((w&0x3ffffff)<<2)
 c=Probe();c.run(site,target);assert c.calls==[{'site':hex(site),'target':hex(target)}] and c.r[31]==site+8 and not c.return_only_hooks
 world.append(row(c));steps+=c.steps
boot=[]
for prior in [0,1,0x8011a604,0xffffffff,0x80089858,0x8011a644]:
 c=Probe();c.put(0x1f800064,4,prior);c.run(0x80100354,0x80100364)
 assert c.get(0x1f800064,4)==TABLE and c.steps==4 and not c.return_only_hooks
 boot.append({'prior':hex(prior),'result':hex(c.get(0x1f800064,4)),'instructions':c.steps});steps+=c.steps
cases=[]
for slot in [0,1,31,32,63]:
 for raw0,raw6,subtype in [(0,3,2),(1,0,2),(2,3,2),(3,2,2),(1,3,2),(3,3,2),(129,255,2),(255,255,2),(1,3,0),(1,3,3),(1,3,4),(1,3,5),(1,3,6),(1,3,9),(1,3,15),(1,3,128),(1,3,131),(1,3,255)]:
  c=Probe();c.put(0x1f800064,4,TABLE)
  for j in range(64):c.put(BASE+j*ST,1,0);c.put(BASE+j*ST+6,1,3);c.put(BASE+j*ST+1,1,128|j)
  c.put(BASE+slot*ST,1,raw0);c.put(BASE+slot*ST+6,1,raw6);c.put(BASE+slot*ST+2,1,subtype)
  # Deliberately empty allocation masks; original scalar dispatcher ignores them.
  c.put(0x80098838,4,0);c.put(0x8009883c,4,0)
  expected=(raw0&raw6&1)!=0 and subtype<128
  target=struct.unpack_from('<I',ov,TABLE-0x80100000+subtype*4)[0] if subtype<128 else 0
  c.trace=True;c.run(0x800172f4,target if expected else None);c.trace=False
  assert not c.return_only_hooks
  if expected:
   assert c.indirect==[{'site':'0x80017374','target':hex(target)}]
   assert c.r[4]==BASE+slot*ST and c.r[31]==0x8001737c
   assert c.reads==list(range(slot+1))
  else:assert not c.indirect and c.reads==list(range(64))
  cases.append({'slot':slot,'raw00':raw0,'raw06':raw6,'subtype02':subtype,'eligible':expected,'read_slots':c.reads,**row(c)});steps+=c.steps
result={'schema':'mml-lower80-dispatch-caller-proof/1','status':'SOURCE_BACKED_PARTIAL','scope':'UNCHANGED_CALL_AND_DISPATCH_BOUNDARIES_ONLY','owner_exe_sha256':OWNER_SHA,'overlay_sha256':hashlib.sha256(ov).hexdigest(),'original_instructions':steps,'instruction_patches':0,'helper_hooks':0,'world_call_boundaries':world,'table_boot_store_cases':boot,'scalar_dispatch_cases':cases,'table_token':hex(TABLE),'selected_source_table_entries':{str(i):hex(struct.unpack_from('<I',ov,TABLE-0x80100000+i*4)[0]) for i in [0,2,3,4,5,6,9,15]},'original_allocation_bitmaps_tested_zero':True,'retail_parity':False,'hardware_tested':False,'limits':['Callbacks stop at real entry; no callback was artificially acknowledged or executed.','World callee interiors and cooperative timing remain external boundaries.','Only selected known table entries are inspected; no adjacent table boundary implies arbitrary table size.','Absent/default scratch64 is not admitted; explicit original overlay store is required.']}
out.mkdir(parents=True);p=out/'dispatch-caller-proof.json';p.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'dispatch_cases':len(cases),'world_boundaries':len(world),'boot_store_cases':len(boot),'original_instructions':steps,'helper_hooks':0,'instruction_patches':0}))

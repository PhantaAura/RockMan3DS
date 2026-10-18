#!/usr/bin/env python3
"""Unchanged original actor state0..12 prefix oracle; owner bytes stay private."""
import argparse,json,struct,sys,hashlib
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(SOURCE/'docs/research_01434/scene_actor40_init'))
import source_scene_actor40_init_probe as m
# State/substate/request and a proved sequence frame used by the branch.
CASES=[(0,0,0,0),(0,1,0,1),
 (1,0,0,0),(1,1,16,0),(1,1,16,50),(1,1,16,61),(1,2,16,61),
 (1,3,16,0),(1,4,17,23),(1,5,17,23),(1,6,17,44),(1,7,17,61),
 (1,8,18,32),(1,9,18,32),(1,10,19,45),(1,11,20,59),(1,12,20,0),
 (2,0,0,0),(2,1,21,2),(2,1,21,22),(3,0,0,0),(3,1,22,0),
 (4,0,0,0),(4,1,23,0),(4,1,23,120),(4,1,23,127),
 (5,0,0,0),(5,1,25,0),(6,0,0,0),(6,1,48,0),(6,1,48,38),
 (6,2,34,0),(6,2,34,0),(6,3,34,0),
 (7,0,0,0),(7,1,26,0),(7,1,26,21),(7,2,26,0),
 (8,0,0,0),(8,1,27,0),(9,0,0,0),(9,1,28,9),(9,2,28,0),(9,2,28,32),(9,3,28,0),
 (10,0,0,0),(10,1,29,8),(10,2,29,13),(10,3,29,38),(10,4,29,0),
 (11,0,0,0),(11,1,30,0),(12,0,0,0),(12,1,31,40),(12,2,31,60),
 (12,3,31,63),(12,4,31,0),(12,5,32,3),(12,5,32,11)]
def setup(img,owner,overlay,ebd):
 c=m.InitProbe(img,owner.base,overlay);m.fill(c,0x8016c000,ebd)
 for lo,hi in ((0x8002e88c,0x8002e8d8),(0x8002e924,0x8002e980),(0x8002e9cc,0x8002ea18)):
  c.r=[0]*32;c.r[29]=0x700000;c.run(lo,hi)
 c.r=[0]*32;c.r[29]=0x700000;c.r[4]=0x800c4c48;c.run(0x80100efc)
 c.r=[0]*32;c.r[29]=0x700000;c.r[4]=m.WORK;c.steps=0;c.run(0x8010d824)
 return c
def call(c,pc,end=None):
 c.r=[0]*32;c.r[29]=0x700000;c.r[4]=m.WORK;c.steps=0;c.run(pc,end)
def frame_index(c,request,frame):
 c.r=[0]*32;c.r[29]=0x700000;c.r[4]=m.WORK;c.r[5]=request;c.run(0x800322a8)
 base=c.get(m.WORK+148,4)
 for i in range(256):
  if c.get(base+i*8,1)==frame:return i
  control=c.get(base+i*8+3,1)
  if control>=128:break
 raise ValueError('frame absent '+str((request,frame)))
def snapshot(c):
 return [m.read(c,m.WORK,1444).hex(),m.read(c,0x800bf118,32*80).hex(),m.read(c,0x800a3b40,64*72).hex(),
  str(c.get(0x800988c8,4)),str(c.get(0x800988cc,4))]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in('exe','overlay','ebd','out'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();out=a.out.resolve()
 if out.exists()or out.is_relative_to(SOURCE):raise ValueError('Choose a new private output directory')
 img=a.exe.read_bytes();owner=m.OwnerExe(img);overlay=a.overlay.read_bytes();ebd=a.ebd.read_bytes()
 rows=[];instructions=0;failed=[]
 for seed,(state,sub,req,frame) in enumerate(CASES):
  try:
   c=setup(img,owner,overlay,ebd);index=frame_index(c,req,frame)
   c.r=[0]*32;c.r[29]=0x700000;c.r[4]=m.WORK;c.r[5]=req;c.r[6]=index;c.run(0x800322a8)
   c.put(m.WORK+9,1,state);c.put(m.WORK+10,1,sub);c.put(m.WORK+12,1,1 if state==1 and sub==5 else 0)
   c.put(m.WORK+13,1,1 if seed%2 else 47)
   for axis in range(3):c.put(m.WORK+28+axis*4,4,(c.get(m.WORK+20+axis*2,2,True)<<16)+[0x8000,0x7fff,0x8001][axis])
   before=snapshot(c);event=m.read(c,0x800c4c48,168).hex()
   bits=[c.get(0x80098838,4),c.get(0x8009883c,4),c.get(0x800989cc,4)]
   call(c,0x8010d7a4,0x8010d800);instructions+=c.steps
   assert not c.return_only_hooks
   rows.append([seed,state,sub,req,frame,*bits,event,*before,*snapshot(c)])
  except Exception as ex:failed.append({'case':seed,'state':state,'substate':sub,'reason':str(ex)})
 # Consecutive state6 local updates establish48->34 and all47 exact X steps.
 c=setup(img,owner,overlay,ebd);c.put(m.WORK+9,1,6);c.put(m.WORK+10,1,0)
 sequence=[]
 for tick in range(120):
  before=snapshot(c);event=m.read(c,0x800c4c48,168).hex();bits=[c.get(0x80098838,4),c.get(0x8009883c,4),c.get(0x800989cc,4)]
  call(c,0x8010d7a4,0x8010d800);instructions+=c.steps
  assert not c.return_only_hooks
  rows.append([1000+tick,6,0,0,0,*bits,event,*before,*snapshot(c)])
  sequence.append({'tick':tick,'substate':c.get(m.WORK+10,1),'request':c.get(m.WORK+172,1),'frame':c.get(m.WORK+164,1),'rawx':c.get(m.WORK+20,2,True),'fixedx':c.get(m.WORK+28,4),'countdown0d':c.get(m.WORK+13,1)})
  if c.get(m.WORK+10,1)==3:break
 out.mkdir(parents=True);data=('seed state sub request frame used80a used80b usede0 event before beforee0 before80 beforependinga beforependingb after aftere0 after80 afterpendinga afterpendingb\n'+''.join(' '.join(map(str,row))+'\n'for row in rows)).encode()
 (out/'owner-states.tsv').write_bytes(data);(out/'reference-trig.bin').write_bytes(owner.read(0x80092ec4,16384))
 proof={'schema':'source-scene-actor40-state-prefix/1','status':'SOURCE_BACKED_PARTIAL','cases':len(rows),'authored_branch_cases':len(rows)-len(sequence),'consecutive_state6_calls':len(sequence),'original_instructions':instructions,'owner_exe_sha256':m.OWNER_SHA,'overlay_sha256':m.sha(overlay),'ebd_sha256':m.sha(ebd),'private_fixture_sha256':m.sha(data),'instruction_patches':0,'helper_hooks':0,'failed_cases':failed,'state6_sequence':sequence,'retail_parity':False,'hardware_tested':False,'limits':['Stops at8010D800 before contact; consecutive calls do not prove retail scheduling.','Original scalar/state/sequence/matrix fields are private differential inputs.','Audio request receipts require separate native audio delivery.','Mesh-index stores preserve work, renderer variant ownership is separate.']}
 (out/'proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({k:v for k,v in proof.items()if k!='state6_sequence'},indent=2))
if __name__=='__main__':main()

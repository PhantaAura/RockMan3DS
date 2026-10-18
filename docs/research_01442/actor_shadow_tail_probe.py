#!/usr/bin/env python3
"""Class03 shadow call/queue on authored fields, with explicit floor boundary.
No game bytes or instruction listings are exported. Fresh pool clear executes.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01414'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01423')]
from actor_pose_proxy_source_probe import Probe
from tutorial_actor_identity_probe import iter_resources,SUPPORTED_EXE,OVERLAY_SHA
WORK=0x1000
class Observe(Probe):
 def __init__(self,*a):super().__init__(*a);self.shadow_arguments=[];self.floor_arguments=[]
 def get(self,a,n,signed=False):
  if n==4 and a==0x8002c8cc:self.shadow_arguments.append(list(self.r[4:8]))
  if n==4 and a==0x80048e6c:self.floor_arguments.append(self.r[4])
  return super().get(a,n,signed)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--exe',type=Path,required=True);ap.add_argument('--dat',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);a=ap.parse_args()
 if a.out.exists() or a.out.resolve()==SOURCE or SOURCE in a.out.resolve().parents:raise ValueError('Choose NEW private output')
 image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SUPPORTED_EXE
 overlay=next(r['payload']for r in iter_resources(a.dat/'ST00.BIN')if r['load']==0x80100000 and 'PROGBIN'in r['internal'].upper());assert hashlib.sha256(overlay).hexdigest()==OVERLAY_SHA
 base=struct.unpack_from('<I',image,24)[0];rows=[];steps=0;floor_calls=0
 for work00,count,work76,heading,raw_y,floor in itertools.product((1,3),(0,63,64,65),(-128,0,1,127),(0,4095),(-32768,-1),(32767,)):
  c=Observe(image,base,overlay);c.r[16],c.r[17],c.r[29]=WORK,WORK+0x328,0x7000
  c.put(WORK,1,work00);c.put(WORK+0x328,2,4);c.put(WORK+0x76,1,work76);c.put(WORK+0x56,2,heading)
  for i,v in enumerate((101,raw_y,-202)):c.put(WORK+0x14+2*i,2,v)
  c.put(0x800987d8,4,count);c.hook(0x80048e6c,floor);c.run(0x8010234c,0x80102370)
  append=bool(work00&2)and count<64;after=c.get(0x800987d8,4)
  assert c.shadow_arguments==([[WORK,48,512,0]]if work00&2 else [])
  assert c.floor_arguments==([WORK]if append and work76==0 else [])
  assert after==count+int(append)
  record=[c.get(0x800ace00+count*12+off,2)if append else 0 for off in range(0,12,2)]
  rows.append([work00,count,work76,heading,101,raw_y,-202,floor,after,*record]);steps+=c.steps;floor_calls+=len(c.floor_arguments)
 c=Observe(image,base)
 for slot in range(16):c.put(0x8009f108+slot*0x3c0+0x76,1,0xa5)
 c.r[29]=0x7000;c.run(0x8002e840,0x8002e854)
 assert all(c.get(0x8009f108+slot*0x3c0+0x76,1)==0 for slot in range(16));steps+=c.steps
 data=('\n'.join(' '.join(map(str,row))for row in rows)+'\n').encode()
 proof={'schema':'mml-class03-shadow-tail/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':SUPPORTED_EXE,'overlay_sha256':OVERLAY_SHA,'cases':len(rows),'original_instructions':steps,'call_site':'0x80102368','function':'0x8002C8CC','arguments':{'radius':48,'scale':512,'type':0},'floor_return_boundary':'0x80048E6C','floor_calls':floor_calls,'fresh_clear':{'entry':'0x8002E840..8002E854','slots':16,'offset':'0x76','all_dirty_sentinels_cleared':True,'instructions':c.steps},'fixture_sha256':hashlib.sha256(data).hexdigest(),'caller_and_shadow_instruction_patches':0,'floor_return_hook':True,'limits':['Authored work/queue/floor fields, not live combat chronology.','Original class03 shadow caller and8002C8CC packing execute; floor return is explicit authored scalar hook.','Fresh pool clear owns zero work76 only at explicit fresh lifecycle; warm/untracked writes remain UNKNOWN.','Separate native wrapper uses actual bounded map/bounds query; unsupported geometry holds.','Shadow rasterization, actual Buster firing, full RNG, defeat and physicalOld3DS remain unproved.']}
 a.out.mkdir(parents=True);(a.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');(a.out/'shadow-authored.tsv').write_bytes(data);print(json.dumps({'cases':len(rows),'original_instructions':steps,'floor_calls':floor_calls,'fixture_sha256':proof['fixture_sha256']}))
if __name__=='__main__':main()

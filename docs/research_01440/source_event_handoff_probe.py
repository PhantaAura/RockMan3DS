#!/usr/bin/env python3
"""Unchanged type10 startup and phase7 tail on authored owners, without hooks.
The fade cases stop at the explicit scheduler intent boundary. Handoff cases
execute actual primitive/template copies and queue producer on authored data.
Original instruction bytes, lookup payloads and retail assets are not exported.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01422')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from class60_06_source_probe import FastProbe,OVERLAY_SHA
WORK,PLAYER,QUEUE=0x800c4c48,0x800b51b0,0x800a3a50
REGIONS=((0x80098a68,16),(0x80098a28,32),(0x80098a10,24))
def sha(data):return hashlib.sha256(data).hexdigest()
def authored(seed,size):return bytes((seed*37+i*17+11)&255 for i in range(size))
def fill(c,base,data):
 for i,value in enumerate(data):c.put(base+i,1,value)
def blob(c,base,size):return bytes(c.get(base+i,1)for i in range(size))
class Probe(FastProbe):
 def __init__(self,image,base,overlay):
  super().__init__(image,base,overlay);self.r[29]=0x700000;self.r[28]=0x80097864;self.r[31]=0
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('exe','overlay','out'):p.add_argument('--'+name,required=True,type=Path)
 a=p.parse_args();out=a.out.resolve()
 if out.exists()or out==SOURCE or SOURCE in out.parents:raise ValueError('Choose a new private output')
 image,overlay=a.exe.read_bytes(),a.overlay.read_bytes();owner=OwnerExe(image);assert sha(overlay)==OVERLAY_SHA
 rows=[];steps=0;display_changes=0
 # Distinct duration44/46 demonstrate that type10 is not an old-profile alias.
 for case,(duration,step,busy) in enumerate((d,s,b)for d in(0,1,65535)for s in(0,31)for b in(0,1,32768)):
  c=Probe(image,owner.base,overlay);before=authored(case,72);at=0
  for address,n in REGIONS:fill(c,address,before[at:at+n]);at+=n
  c.put(0x80098a72,2,busy);c.put(0x80097a44,2,2345);c.put(0x80097a46,2,duration);c.put(0x80097a4c,2,step);c.put(0x80097718,1,case)
  before=b''.join(blob(c,address,n)for address,n in REGIONS);c.r[4]=10
  c.run(0x80017c30,0x80012e10 if busy==0 else None)
  if busy==0:assert c.r[4]==2 and c.r[5]==0x80017e94
  after=b''.join(blob(c,address,n)for address,n in REGIONS)
  assert not c.return_only_hooks;steps+=c.steps
  rows.append(f'F {case} {duration} {step} {busy} {before.hex()} {after.hex()}')
 for case in range(48):
  c=Probe(image,owner.base,overlay);work=bytearray(authored(case,168));work[0]=1;work[1]=1;work[4]=7;fill(c,WORK,work);c.r[16]=WORK
  player=bytearray(authored(case+7,1184));player[0]=(0,1,2,255)[case%4];player[8]=(0,1,3,255)[case%4];fill(c,PLAYER,player)
  transition=authored(case+13,18);fill(c,0x800c3560,transition)
  flags=authored(case+17,256);fill(c,0x800be378,flags)
  count=(0,1,4,6)[case%4];queue=bytearray(authored(case+23,256))
  for i in range(16):
   struct.pack_into('<I',queue,i*16,0 if i>=count else(2 if i%3!=2 else 4))
   struct.pack_into('<I',queue,i*16+8,1 if i%4<2 else 2)
  fill(c,QUEUE,queue);c.put(0x800989a4,4,QUEUE+16*count);c.put(0x80098a72,2,0);c.put(0x80122428,1,0)
  # Actual source helper has distinct destination/template addresses. Its
  # zero offsets still copy the source words. Keep every such owner authored.
  windows=[]
  for index,header in enumerate((0x800f7820,0x800f7830)):
   tri=(case+index)%3;quad=(case//3+index)%3
   c.put(header,4,tri|(quad<<8));c.put(header+4,4,0x510000+index*0x20000);c.put(header+8,4,0x520000+index*0x20000)
   for ptr in(c.get(header+4,4),c.get(header+8,4)):
    size=1024;data=authored(case+31+index,size);fill(c,ptr,data);windows.append((ptr,data))
  before_display=[blob(c,ptr,len(data))for ptr,data in windows]
  c.run(0x80101260);assert not c.return_only_hooks
  display_changes+=sum(blob(c,ptr,len(data))!=before for(ptr,data),before in zip(windows,before_display))
  assert c.get(WORK,1)==0 and c.get(0x800c356c,1)==2
  steps+=c.steps;after_count=(c.get(0x800989a4,4)-QUEUE)//16
  rows.append(' '.join(map(str,['H',case,count,after_count,bytes(work).hex(),blob(c,WORK,168).hex(),bytes(player).hex(),blob(c,PLAYER,1184).hex(),transition.hex(),blob(c,0x800c3560,18).hex(),flags.hex(),blob(c,0x800be378,256).hex(),bytes(queue).hex(),blob(c,QUEUE,256).hex()])))
 fixture=('\n'.join(rows)+'\n').encode();out.mkdir(parents=True)
 (out/'handoff-authored.tsv').write_bytes(fixture)
 proof={'schema':'mml-source-event-handoff/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'fade_cases':18,'handoff_cases':48,'original_instructions':steps,'display_windows_changed':display_changes,'instruction_patches':0,'helper_hooks':0,'fixture_sha256':sha(fixture),'retail_parity':False,'hardware_tested':False,'limits':['Fade stops at80012E10 scheduler intent; BIOS thread completion is not synthesized.','Phase7 tail starts after the separately owned callback/camera/timeline consumers.','Actual template/destination writes execute on authored buffers; live receipt ownership remains required.','Transition loader, later player physics and CD command execution remain separate consumers.']}
 (out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

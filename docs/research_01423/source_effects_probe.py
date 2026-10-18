#!/usr/bin/env python3
"""Original effect allocator/common128/131 differential from readonly owner EXE.
All exported slot bytes and contexts are authored inputs/results. No owner
instructions, assets, function table or descriptor payload is exported.
"""
import argparse,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01422'),str(SOURCE/'docs/research_01417')]
from class60_06_source_probe import FastProbe,sha,SUPPORTED_EXE,full_result
from class20_03_callback_source_probe import authored_store_hook
FX=0x800a3b40
ALLOC=0x80098838
RELEASE=0x800988c8
GP=0x80097864
SIZE=72

def cpu(im,base):
 c=FastProbe(im,base,b'');c.r[29]=0x700000;c.r[31]=0;c.r[28]=GP;return c

def blob(c):return c.blob(FX,SIZE)
def seed(c,subtype=131,state=0,param=0,timer=3,extent=13,y=-11,color=0x2e404040):
 for i in range(SIZE):c.put(FX+i,1,(i*17+11)&255)
 for o,n,v in [(0,1,3),(1,1,128),(2,1,subtype),(3,1,param),(6,1,3),(8,1,state),(11,1,timer),(13,1,0),(14,1,0),(0x1e,2,y),(0x24,2,extent),(0x18,4,color)]:c.put(FX+o,n,v)
 c.put(ALLOC,4,0x80000000);c.put(ALLOC+4,4,0);c.put(RELEASE,4,0);c.put(RELEASE+4,4,0);c.r[4]=FX

def allocator(im,base):
 rows=[];steps=0
 for slot in list(range(64))+[64]:
  c=cpu(im,base)
  masks=[0xffffffff,0xffffffff]
  if slot<64:masks[slot//32]^=0x80000000>>(slot&31)
  target=FX+min(slot,63)*SIZE
  for i in range(SIZE):c.put(target+i,1,(i*19+slot*3+5)&255)
  before=c.blob(target,SIZE)
  for b in range(2):c.put(ALLOC+b*4,4,masks[b]);c.put(RELEASE+b*4,4,0)
  c.run(0x80031618)
  rows.append([slot,*masks,c.r[2],c.get(ALLOC,4),c.get(ALLOC+4,4),*before,*c.blob(target,SIZE)])
  steps+=c.steps
 return rows,steps

def lifecycle131(im,base):
 rows=[];steps=0
 for state,param,timer,extent,y,color in itertools.product(range(5),range(3),(0,1,3,255),(0,13,65535),(-32768,0,32767),(0,0x2e404040,0xffffffff)):
  c=cpu(im,base);seed(c,state=state,param=param,timer=timer,extent=extent,y=y,color=color)
  before=blob(c);c.run(0x80044c58)
  rows.append([*before,*blob(c),c.get(RELEASE,4),c.get(RELEASE+4,4)]);steps+=c.steps
 # Initialization's default parameter3/other branches normalize to zero.
 for param in (3,4,127,255):
  c=cpu(im,base);seed(c,param=param);before=blob(c);c.run(0x80044c58)
  rows.append([*before,*blob(c),c.get(RELEASE,4),c.get(RELEASE+4,4)]);steps+=c.steps
 return rows,steps

def lifecycle128(im,base):
 rows=[];steps=0
 for param,floor,y,queue_count in itertools.product((2,66),(-32769,-30,0,30,32768),(-32768,-1,0,32767),(0,23,24)):
  c=cpu(im,base);seed(c,subtype=128,param=param);c.put(FX,1,5);c.put(FX+0x3a,2,y)
  # Authored geometry response + keyed resource wrapper descriptor write.
  full_result(c,0x8004920c,floor&0xffffffff)
  authored_store_hook(c,0x8002f9c4,[(4,0x14,4,0x80161234)],1)
  for i in range(24*20):c.put(0x800be4f8+i,1,0xa5)
  c.put(0x80098938,4,0x800be4f8+queue_count*20)
  c.put(0x80098940,4,0x800be4f8+24*20)
  before=blob(c);c.run(0x80043f34)
  # Record only new authored queue row and actual source queue pointer movement.
  after_count=(c.get(0x80098938,4)-0x800be4f8)//20
  new=c.blob(0x800be4f8+min(queue_count,23)*20,20)
  rows.append([floor,queue_count,*before,*blob(c),after_count,*new]);steps+=c.steps
 return rows,steps

def release(im,base):
 rows=[];steps=0
 for slot in range(64):
  c=cpu(im,base);id_=FX+slot*SIZE
  masks=[0xffffffff,0xffffffff];pending=[0x12345678,0x89abcdef]
  for b in range(2):c.put(ALLOC+b*4,4,masks[b]);c.put(RELEASE+b*4,4,pending[b])
  c.put(id_,1,3);c.r[4]=id_;c.run(0x80031988)
  interim=[c.get(RELEASE+b*4,4) for b in range(2)]
  c.r[31]=0;c.run(0x80031aa4)
  rows.append([slot,*masks,*pending,*interim,c.get(id_,1),*[c.get(ALLOC+b*4,4) for b in range(2)]]);steps+=c.steps
 return rows,steps

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
 im=a.exe.read_bytes();assert sha(im)==SUPPORTED_EXE;base=struct.unpack_from('<I',im,24)[0]
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Use new private output directory')
 # Verify narrow scalar control counts and dispatch labels before applying port.
 word=lambda v:struct.unpack_from('<I',im,v-base+2048)[0]
 assert word(0x80089a58)==0x80043f34 and word(0x80089a64)==0x80044c58
 assert list(im[0x80097de4-base+2048:0x80097df0-base+2048])==[3,3,3,3,4,4,4,4,8,8,8,4]
 boot=cpu(im,base)
 boot.run(0x8002e924,0x8002e980)
 pool_boot={'all_nonclass_bytes_zero':all(boot.get(FX+i*SIZE+o,1)==0 for i in range(64) for o in range(SIZE) if o!=1),
 'class_bytes_match':all(boot.get(FX+i*SIZE+1,1)==128|i for i in range(64)),
 'allocation_masks':[boot.get(ALLOC+b*4,4) for b in range(2)],'pending_masks':[boot.get(RELEASE+b*4,4) for b in range(2)],'original_instructions':boot.steps}
 assert pool_boot['all_nonclass_bytes_zero'] and pool_boot['class_bytes_match']
 a.out.mkdir(parents=True);counts={};total=boot.steps
 for name,fn in [('allocator',allocator),('callback131',lifecycle131),('callback128',lifecycle128),('release',release)]:
  rows,n=fn(im,base);total+=n;counts[name]=len(rows)
  (a.out/(name+'.tsv')).write_text('# authored scalar contexts; actual source instruction outputs\n'+''.join(' '.join(map(str,r))+'\n' for r in rows))
 result={'schema':'mml-source-effects-01423/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(im),'cases':counts,'pool_boot':pool_boot,'original_instructions':total,
 'source_labels':{'pool_boot':'8002E924..8002E97C','allocator':'80031618','pending_release':'80031988','release_consumer':'80031B84..80031BB0','common_dispatch':'800173B4','callback128':'80043F34','callback131':'80044C58','primitive_dispatch':'80029B10'},
 'limits':['Authored contexts are not live retail observations.','128 floor and resource wrapper are explicit authored boundaries; actual audio writes execute.','128 later geometry/proxy/rotation consumers remain held.','Projection/fog/material/native rendering not proved by scalar callbacks.','No retail instructions, descriptors, table or assets exported.']}
 (a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()

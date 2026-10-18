#!/usr/bin/env python3
"""Source ST00 child subtype6 lifecycle differential, explicit authored boundaries.
Immutable owner executable/overlay are hash checked; scalar fixture contexts
are authored. Contact is separately tested. Actual proxy, registry, shadow,
fixed conversion, state dispatch and motion instructions execute here.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01417')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_callback_source_probe import authored_store_hook
WORK,FX=0x1000,0x800a3b40
SIZE=268
class FastProbe(OverlayProbe):
 def __init__(self,image,base,overlay):
  super().__init__(image,base,b'');self.overlay=overlay;self.contact_before=None;self.contact_after=None;self.proxy=None
 def blob(self,p,n):return [self.get(p+i,1) for i in range(n)]
 def get(self,a,n,signed=False):
  def byte(p):
   if p in self.mem:return self.mem[p]
   if 0x80100000<=p<0x80100000+len(self.overlay):return self.overlay[p-0x80100000]
   if self.base<=p<self.base+len(self.image)-2048:return self.image[p-self.base+2048]
   return 0
  v=sum(byte(a+i)<<(8*i) for i in range(n))
  if n==4 and a==0x8010fdd4 and self.contact_before is None:self.contact_before=self.blob(WORK,SIZE)
  if n==4 and a==0x800341bc and self.proxy is None:
   point=[self.get(self.r[4]+i*2,2,True) for i in range(3)]
   self.proxy=point+[self.get(self.r[29]+16,4),self.r[5],self.r[7],self.get(WORK+0x9c,4),self.get(WORK+0xa0,4)]
  return v-(1<<(8*n)) if signed and v&(1<<(8*n-1)) else v

def sha(b):return hashlib.sha256(b).hexdigest()
def full_result(c,address,result):
 # Unlike inherited16bit hook, owner allocation identities need all32bits.
 c.put(address,4,(15<<26)|(2<<16)|(result>>16&65535))
 c.put(address+4,4,(13<<26)|(2<<21)|(2<<16)|(result&65535))
 c.put(address+8,4,(31<<21)|8);c.put(address+12,4,0)
def store(c,off,width,values):
 for i,v in enumerate(values):c.put(WORK+off+i*width,width,v)
def initial(c,state,frame,param,packet,vel,delay,heading):
 for i in range(SIZE):c.put(WORK+i,1,(i*13+7)&255)
 for o,w,v in [(0,1,3),(1,1,0x61),(2,1,6),(3,1,param),(6,1,3),(8,1,state),(10,1,0),(12,1,149),(13,1,delay),(14,1,19),(15,1,frame),(100,2,heading),(116,1,1),(118,1,0),(120,4,0),(124,2,0),(156,4,32 if state==0 else packet),(160,4,16)]:c.put(WORK+o,w,v)
 raw=[101,-7,-202,1234];fixed=[v*65536+(12345 if i!=1 else 0) for i,v in enumerate(raw)]
 store(c,20,2,raw);store(c,28,4,fixed);store(c,44,2,[-19,-2,63,-444]);store(c,52,4,[v*65536+891 for v in [-19,-2,63,-444]])
 store(c,68,2,vel);store(c,76,2,[7,512,-3]);store(c,144,2,[13,17,-21])
 c.put(0x80098898,4,0xd000);c.put(0x800988e0,4,0);c.put(0x80098798,4,0);c.put(0x800987d8,4,0)
 # Authored UV pairs deliberately use upperLHUbites; originalstoreslowbyte.
 for bank in range(2):
  for f in range(6):
   c.put(0x8011c81c+bank*24+f*4,2,0x1200+bank*49+f*5)
   c.put(0x8011c81e+bank*24+f*4,2,0x3400+bank*31+f*7)
 return raw,fixed

def callback_cases(image,base,ov):
 rows=[];steps=0
 for state,frame,param,packet,vel,delay,cr,allocation in itertools.product(range(5),(0,5),(0,1,128),(0,0x20000000,0x08000000),((321,-654,987),(24570,24570,-24570)),(0,15,30),(0,1),(0,FX)):
  c=FastProbe(image,base,ov);initial(c,state,frame,param,packet,vel,delay,4095)
  # Authored angle/trig, floor, geometry-response and allocation premises only.
  sin,cos=-3072,2048;c.hook(0x8007a060,sin);c.hook(0x8007a12c,cos)
  authored_store_hook(c,0x8010fdd4,[(4,0x16,2,-18),(4,0x20,4,-18*65536)],cr)
  full_result(c,0x80031618,allocation);c.hook(0x80048e6c,26)
  for off in range(72):c.put(FX+off,1,(off*17+3)&255)
  before=c.blob(WORK,SIZE);c.r[4],c.r[29],c.r[31]=WORK,0x700000,0;c.run(0x8010f354)
  after=c.blob(WORK,SIZE);has_contact=int(c.contact_before is not None)
  contact_before=c.contact_before or [0]*SIZE
  contact_after=list(contact_before)
  if has_contact:
   contact_after[0x16:0x18]=list(struct.pack('<h',-18));contact_after[0x20:0x24]=list(struct.pack('<i',-18*65536))
  proxy=c.proxy or [0]*8
  # Registry alwaysclear7C; source actual800341BC writesA0=128 for globalmask0.
  shadow=[c.get(0x800ace00+o,2) for o in (0,2,4,6,8,10)]
  released=int(any(call['target']=='0x800318d8' for call in c.calls))
  effect_called=int(any(call['target']=='0x80110164' for call in c.calls))
  effect_bytes=c.blob(FX,72) if effect_called else [0]*72
  rows.append([sin,cos,2048,allocation,cr,has_contact,*contact_before,*contact_after,int(c.proxy is not None),*proxy,128,26,*before,*after,released,effect_called,*effect_bytes,*shadow])
  steps+=c.steps
 return rows,steps

def motion_cases(image,base,ov):
 rows=[];steps=0
 for v,a,f in itertools.product((-32768,-24577,-24576,-1,0,24576,24577,32767),(-32768,-512,-1,0,512,32767),(-2147483648,-1,0,2147483647)):
  c=FastProbe(image,base,ov);initial(c,1,0,0,0,[v,~v&65535,v],0,0);store(c,76,2,[a,a,-a]);store(c,28,4,[f,f,f,1234567])
  before=c.blob(WORK,SIZE);c.r[4],c.r[31]=WORK,0;c.run(0x80110070);rows.append(before+c.blob(WORK,SIZE));steps+=c.steps
 return rows,steps

def pool_case(image,base,ov):
 c=FastProbe(image,base,ov);c.r[29]=0x700000;c.r[31]=0
 c.run(0x8002e8d8,0x8002e924)
 return {'allocation_bitmap':c.get(0x800987cc,4),'pending_release_bitmap':c.get(0x80098808,4),
 'class_bytes':[c.get(0x8009cf88+i*268+1,1) for i in range(32)],
 'all_nonclass_bytes_zero':all(c.get(0x8009cf88+i*268+j,1)==0 for i in range(32) for j in range(268) if j!=1),'instructions':c.steps}

def primitive_cases(image,base,ov):
 rows=[];steps=0
 # Original integer corner construction (GTEcam-space center is authored).
 for center,extent in itertools.product(((-101,-202,777),(0,0,4096),(32760,-32760,193)),(0,32,255)):
  c=FastProbe(image,base,ov);c.r[8],c.r[9],c.r[5],c.r[29]=WORK,3,extent,0x700000;c.r[31]=0
  c.put(0x1f800070,4,0x9000)
  for i,v in enumerate(center):c.put(0x700020+i*4,4,v)
  c.run(0x80029e80,0x80029f70)
  rows.append([*center,extent,*[c.get(0x700030+i*2,2,True) for i in range(12)]])
  # Original UV/CLUT/tpage packing; GTEscreen output excluded here.
  c.r[7]=0x9000;c.r[8]=WORK;c.r[6]=0x1234;c.put(WORK,4,0x3456000e);c.run(0x8002a090,0x8002a0d0)
  rows[-1]+=[c.get(0x9000+o,4) for o in (12,20,28,36)];steps+=c.steps
 return rows,steps

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose new privateoutput')
 im=a.exe.read_bytes();assert sha(im)==SUPPORTED_EXE;base=struct.unpack_from('<I',im,24)[0]
 ov=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(ov)==OVERLAY_SHA
 a.out.mkdir(parents=True);rows,steps=callback_cases(im,base,ov)
 (a.out/'callback-authored.tsv').write_text('# authored callback premises/results; contact external, actualproxy/registry/shadow execute\n'+''.join(' '.join(map(str,r))+'\n' for r in rows))
 motion,ms=motion_cases(im,base,ov);primitive,ps=primitive_cases(im,base,ov);pool=pool_case(im,base,ov)
 for name,r in [('motion-authored',motion),('primitive-authored',primitive)]:
  (a.out/(name+'.tsv')).write_text('# authored original scalar premises/results\n'+''.join(' '.join(map(str,row))+'\n' for row in r))
 result={'schema':'mml-class60-06-source/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(im),'overlay_sha256':sha(ov),'callback_cases':len(rows),'motion_cases':len(motion),'primitive_cases':len(primitive),'pool_init':pool,'original_instructions':steps+ms+ps+pool['instructions'],
 'consumers':{'callback':'8010F354','state_entries':['8010F47C','8010F674','8010F8E8','8010F9F4','8010FC88'],'motion':'80110070','contact':'8010FDD4','proxy':'800341BC','registry':'80051BAC','shadow':'8002C8CC','primitive_renderer':'80029C18'},
 'limits':['Callback contact result/writes,trig,floor andeffectallocation are authored boundaries; nativecontact separatelyprobed.','Actual source proxy/registry/shadow, fixedconversion, scalar lifecycle,motion,effectinitializer andpoolboot execute.','Primitive proof admitsintegercorners andUVpacking withauthorcamspacecenter; GTEprojection/fog areexcluded.','No live retailchronology, authoredphysicsorhardwareparityclaim.']}
 (a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('callback_cases','motion_cases','primitive_cases','original_instructions','pool_init')}))
if __name__=='__main__':main()

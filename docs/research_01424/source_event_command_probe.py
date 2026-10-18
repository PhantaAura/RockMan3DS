#!/usr/bin/env python3
"""Original type4 logical instructions, authored CD boundaries; no code export.

Accepted complete cases have no hooks, MMIO input callbacks or replaced game
instructions. Separate CD boundary cases execute caller slices unchanged but
use explicit authored transport responses; they are not driver/retail parity.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
GLOBALS=[(0x800989C4,4),(0x80098828,4),(0x8009896C,1),(0x800988D0,1),(0x80098964,1),(0x80098AB8,1),(0x80098B42,1),(0x800AD142,2),(0x800AD14E,2),(0x800988EC,1),(0x800988C0,1),(0x800989B0,4)]
COMMANDS=0x800A3A40
sha=lambda b:hashlib.sha256(b).hexdigest()
def put(c,a,b):
 for i,v in enumerate(b):c.put(a+i,1,v)
def get(c,a,n):return bytes(c.get(a+i,1) for i in range(n))
def pattern(n,seed):return bytes((i*37+seed*11)&255 for i in range(n))
class Probe(OverlayProbe):
 def __init__(self,image,base,overlay):
  # These functions and all their callees are in the EXE. Verify the supplied
  # overlay hash once, but do not allocate its unused bytes for every case.
  super().__init__(image,base,b'');self.mix=[]
  for i in range(4):self.put(0x800922C4+i*4,4,0x2000+i)
 def put(self,address,n,value):
  super().put(address,n,value)
  # The CD register is banked. A channel value32 on bank2 is not the final
  # mixer-apply write32 on bank3; count only the unchanged source apply store.
  if address==0x2003 and n==1 and value==32 and self.get(0x2000,1)==3:
   self.mix.append(get(self,0x800AD154,4).hex())
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','dat','fixture','proof'):ap.add_argument('--'+name,type=Path,required=True)
 a=ap.parse_args()
 if a.fixture.exists() or a.proof.exists():raise SystemExit('Refusing to overwrite command evidence')
 image=a.exe.read_bytes();base=struct.unpack_from('<I',image,24)[0];assert sha(image)==SUPPORTED_EXE
 overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(overlay)==OVERLAY_SHA
 rows=[];steps=0;mix_calls=0;counts={'dispatch':0,'bcd_encode':0,'bcd_decode':0,'sync_callback':0,'transport_boundaries':0}
 def make(seed,phase,counter,status,count,special=2):
  c=Probe(image,base,overlay);c.r[29]=0x700000
  queue=bytearray(pattern(256,seed))
  for i in range(16):struct.pack_into('<I',queue,i*16,4 if i==0 else 2 if i<count else 0)
  put(c,COMMANDS,queue);c.put(0x80098A84,4,COMMANDS);c.put(0x800989A4,4,COMMANDS+count*16)
  values=[phase,counter,seed&255,73,1,status,special,(0x9235^seed)&65535,54321,200,71,0x81234567]
  for (addr,width),value in zip(GLOBALS,values):c.put(addr,width,value)
  c.put(0x800989C8,4,0x31415926);return c
 def scalar(c):return [c.get(address,width) for address,width in GLOBALS]
 cases=[(phase,counter,0,count) for phase in (0,1,2) for counter in (1,2,5,0xffffffff) for count in (1,7,15)]
 cases += [(2,0,status,1+(status%15)) for status in range(256)]
 cases += [(3,counter,status,1+(status%15)) for counter in (0,148,149,0xfffffffe,0xffffffff) for status in (0,1,31,32,33,64,128,255)]
 cases += [(phase,7,32,1) for phase in (4,9,0xffffffff)]
 for seed,(phase,counter,status,count) in enumerate(cases):
  c=make(seed,phase,counter,status,count);c.run(0x8001B89C)
  assert not c.return_only_hooks
  rows.append(['D',seed,phase,counter,status,count,*scalar(c),get(c,COMMANDS,256).hex(),len(c.mix),','.join(c.mix) or '-'])
  counts['dispatch']+=1;steps+=c.steps;mix_calls+=len(c.mix)
 for i in range(512):
  sector=[-2147483648,-151,-150,-149,-1,0,1,74,75,4499,4500,449999,2147483647][i%13] if i<130 else ((i*0x81234567+0x456789ab)&0xffffffff)
  if sector>0x7fffffff:sector-=0x100000000
  c=Probe(image,base,overlay);c.r[29]=0x700000;c.r[4]=sector&0xffffffff;c.r[5]=0x3000;c.run(0x80077E88)
  rows.append(['E',sector,get(c,0x3000,3).hex()]);steps+=c.steps;counts['bcd_encode']+=1
 for i in range(512):
  bcd=bytes(((i*17)&255,(i*61)&255,(i*113)&255));c=Probe(image,base,overlay);c.r[29]=0x700000;c.r[4]=0x3000;put(c,0x3000,bcd);c.run(0x80077F8C)
  rows.append(['L',bcd.hex(),c.r[2]]);steps+=c.steps;counts['bcd_decode']+=1
 for code in range(256):
  c=make(code,0,1,65,1);c.r[4]=code;c.put(0x80098964,1,0);c.put(0x80091FFC,4,0x8001cc08);c.run(0x8001CC08)
  rows.append(['C',code,c.get(0x80098964,1),c.get(0x800988C0,1),c.get(0x8009896C,1),c.get(0x80091FFC,4)])
  counts['sync_callback']+=1;steps+=c.steps
 boundaries=[]
 # Exact source mode-change argument preparation, stops before transport body.
 c=make(7,1,5,0,1);c.put(0x800988EC,1,0);c.run(0x8001B89C,0x8001D2BC)
 boundaries.append({'kind':'mode','function':hex(0x8001d2bc),'args':[hex(v) for v in c.r[4:7]],'parameters':get(c,c.r[5],1).hex(),'globals':scalar(c),'source_slice_complete':False});steps+=c.steps
 # Original caller stack filter/location and end-sector, then each CD call.
 c=make(11,2,0,0,1,0);record=bytearray(get(c,COMMANDS,16));struct.pack_into('<III',record,4,7,0xb3123456,0xe2654321);put(c,COMMANDS,record);c.put(0x80082CD0+7*12,4,51789)
 c.run(0x8001B89C,0x8001D2BC);steps+=c.steps
 for call,start,length in ((13,None,2),(2,0x8001BA40,3),(27,0x8001BA54,0)):
  if start is not None:c.steps=0;c.run(start,0x8001D2BC);steps+=c.steps
  boundaries.append({'kind':'seek','command':call,'args':[hex(v) for v in c.r[4:7]],'parameters':get(c,c.r[5],length).hex() if length else '', 'end_sector':c.get(0x800989B0,4),'source_slice_complete':False})
  rows.append(['B',call,get(c,c.r[5],length).hex() if length else '-',c.get(0x800989B0,4)])
 # Response is an explicit authored transport boundary, never a helper hook.
 c.put(0x80098A98,1,0x21);c.steps=0;c.run(0x8001BA68);steps+=c.steps
 boundaries.append({'kind':'read_response_tail','authored_response0':33,'after_status80098AB8':c.get(0x80098AB8,1),'after_phase':c.get(0x800989C4,4),'mix_calls':len(c.mix),'transport_executed':False});mix_calls+=len(c.mix)
 rows.append(['T',c.get(0x80098AB8,1),c.get(0x800989C4,4),c.get(0x800989A4,4)-COMMANDS,len(c.mix),get(c,COMMANDS,256).hex()])
 # Original pause prefix including all127 real CDmix callees; stopped before transport.
 c=make(9,0,0,7,1);c.put(0x80098964,1,0);c.run(0x8001B89C,0x8001D254);steps+=c.steps;mix_calls+=len(c.mix)
 boundaries.append({'kind':'pause','function':hex(0x8001d254),'args':[hex(v) for v in c.r[4:7]],'mix_calls':len(c.mix),'last_mix':c.mix[-1],'callback':hex(c.get(0x80091FFC,4)),'source_slice_complete':False})
 counts['transport_boundaries']=len(boundaries)
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('# Authored inputs, unchanged source outputs. D dispatch/E sector encode/L BCD decode/C sync callback/B control boundary/T response tail.\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows))
 def region(lo,hi):return {'start':hex(lo),'end_exclusive':hex(hi),'sha256':sha(image[2048+lo-base:2048+hi-base])}
 proof={'schema':'mml-source-event-command/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),'overlay_sha256':sha(overlay),'cases':counts,'original_instructions_executed':steps,'original_cd_mix_calls':mix_calls,'complete_case_helper_hooks':[],'transport_boundaries':boundaries,'fixture_sha256':sha(a.fixture.read_bytes()),'regions':[region(lo,hi) for lo,hi in ((0x8001B89C,0x8001BB4C),(0x8001CAAC,0x8001CB30),(0x8001CB7C,0x8001CC08),(0x8001CC08,0x8001CC7C),(0x8001D254,0x8001D324),(0x8001D324,0x8001D414),(0x80077E88,0x8007800C),(0x80078EFC,0x80078F84))],'status_owner':{'writer':'8001BA7C','source':'response80098A98[0] after8001D2BC(27)','reader':'8001D878','raw_bit20':'CD reading flag; not inferred generic event completion'},'limits':['Full dispatch cases avoid CD transport calls through already-mode200/specialmode2 or source phases3+; completecases have no hooks.','Normal seek caller slices and response-tail use explicit authored transport boundaries; original CD driver and real media chronology remain unverified.','MMIO mix writes are observed logical packets only, not audible or CD hardware behavior.','No source instruction, lookup table, audio, assets, save data or cooked owner payload is exported.'],'retail_parity':False,'hardware_tested':False,'PHYSICS_1TO1':False}
 a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({'cases':counts,'original_instructions_executed':steps,'original_cd_mix_calls':mix_calls}))
if __name__=='__main__':main()

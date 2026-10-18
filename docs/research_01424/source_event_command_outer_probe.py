#!/usr/bin/env python3
"""Execute original outer caller slices around explicit authored CD responses.

No CD-driver implementation, helper hook, live retail progression or hardware
claim. Observes the real status writer and dispatch ordering, not elapsed time.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from source_event_command_probe import Probe,GLOBALS,COMMANDS,put,get,pattern,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','dat','fixture','proof'):ap.add_argument('--'+name,type=Path,required=True)
 a=ap.parse_args()
 if a.fixture.exists() or a.proof.exists():raise SystemExit('Refusing to overwrite outer evidence')
 image=a.exe.read_bytes();base=struct.unpack_from('<I',image,24)[0];assert sha(image)==SUPPORTED_EXE
 overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(overlay)==OVERLAY_SHA
 rows=[];steps=0;boundaries=0
 for seed in range(32):
  status=(0,1,31,32,33,63,128,255)[seed%8];sector=(0,75,4500,91731)[seed//8];bcd=bytes([0,0,0])
  encode=Probe(image,base,overlay);encode.r[4]=sector;encode.r[5]=0x3000;encode.run(0x80077E88);bcd=get(encode,0x3000,3);steps+=encode.steps
  c=Probe(image,base,overlay);c.r[29]=0x700000;queue=bytearray(pattern(256,seed));count=1+(seed%15)
  for i in range(16):struct.pack_into('<I',queue,i*16,4 if i==0 else 2 if i<count else 0)
  put(c,COMMANDS,queue);c.put(0x80098A84,4,COMMANDS);c.put(0x800989A4,4,COMMANDS+count*16)
  values=[3,7,0,2,1,0,2,(0x9235^seed)&65535,54321,200,71,1000000]
  for (address,width),value in zip(GLOBALS,values):c.put(address,width,value)
  c.put(0x800989C8,4,0x31415926);c.put(0x800989D8,4,0)
  c.run(0x8001C95C,0x8001D2BC);assert c.r[4]==1;boundaries+=1
  put(c,0x80098A98,bytes([status]+[19]*7));c.run(0x8001C9DC,0x8001D2BC)
  if status&32:
   assert c.r[4]==17;boundaries+=1;put(c,0x80098A98,bytes([19]*5)+bcd);c.run(0x8001CA08)
  assert not c.return_only_hooks
  rows.append([seed,status,count,bcd.hex(),*(c.get(address,width) for address,width in GLOBALS),c.get(0x800989D8,4),get(c,COMMANDS,256).hex(),len(c.mix),','.join(c.mix) or '-']);steps+=c.steps
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('# Explicit authored command1/17 responses; unchanged original outer caller/BCD/type4 instructions.\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows))
 proof={'schema':'mml-source-event-command-outer/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),'overlay_sha256':sha(overlay),'cases':len(rows),'authored_cd_response_boundaries':boundaries,'original_instructions_executed':steps,'helper_hooks':[],'cd_driver_executed':False,'fixture_sha256':sha(a.fixture.read_bytes()),'region':{'start':'0x8001C95C','end_exclusive':'0x8001CAAC','sha256':sha(image[2048+0x8001C95C-base:2048+0x8001CAAC-base])},'source_status_writer':'8001C9E8 copies command1 response[0] to80098AB8 before command17 physical-location and type4 dispatch','limits':['Command1 and17 responses are authored at explicit transport boundaries; no driver receipt chronology/parity claim.','Only the no-error and before-end branch is completed; reset/CdInit/end-loop/pause are native holds.','No instruction, retail tables, model, texture, audio, video, save or cooked owner payload exported.'],'retail_parity':False,'hardware_tested':False,'PHYSICS_1TO1':False}
 a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({key:proof[key] for key in ('cases','authored_cd_response_boundaries','original_instructions_executed')}))
if __name__=='__main__':main()

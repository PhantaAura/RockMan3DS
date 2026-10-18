#!/usr/bin/env python3
"""Read-only source ownership proof for action9's unsigned player heading."""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01420')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE,OVERLAY_SHA,iter_resources
from class20_03_action9_source_probe import Probe

def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--dat',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args()
 if a.out.exists() or a.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output')
 image=a.exe.read_bytes();assert sha(image)==SUPPORTED_EXE;base=struct.unpack_from('<I',image,24)[0]
 overlay=next(r['payload'] for r in iter_resources(a.dat/'ST00.BIN') if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper());assert sha(overlay)==OVERLAY_SHA
 c=OverlayProbe(image,base,overlay);c.r[29]=0x700000;c.r[4]=37;c.run(0x8003be40,0x8003be6c)
 assert c.r[4]==0x800b51b0 and c.r[5]==37
 instructions=c.steps;cases=[]
 for heading,held,target in itertools.product((0,1,2047,2048,4095,32767,32768,65535),(0,2),(0,4095,65535)):
  c=OverlayProbe(image,base,overlay);c.r[4:8]=[0x800b51b0,4,32,2];c.r[29]=0x700000
  c.put(0x800b51b0+0x56,2,heading);c.put(0x800b51b0+0x11c,2,held);c.put(0x800b51b0+0xb4,2,0)
  c.run(0x80042154)
  expected=(heading+(4 if held else -4))&4095
  assert c.get(0x800b5206,2)==expected
  n=Probe(image,base,overlay,target,7);n.r[4]=0x1000;n.r[29]=0x700000;n.put(0x100a,1,2)
  n.put(0x800b5206,2,expected);n.put(0x800a3968,4,1024) # unrelated authored camera yaw
  n.run(0x80103b58)
  result=n.get(0x1000+0x36e,2);assert result==((target-expected)&65535)
  cases.append({'authored_initial_player_heading':heading,'held':held,'authored_player_to_actor_angle':target,'source_updated_player_heading':expected,'action9_unsigned_difference':result})
  instructions+=c.steps+n.steps
 result={'schema':'mml-action9-player-heading-source/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':sha(image),'overlay_sha256':sha(overlay),
         'player_entry':'8003BE40','player_body':'8003BE6C','original_player_work_base':'800B51B0','heading_offset':'56','action9_unsigned_heading_address':'800B5206',
         'pointer_prefix_checks':1,'turn_and_action9_propagation_cases':len(cases),'instructions_executed':instructions,
         'native_field_owner':'RuntimeSession::compat_player().bytes[0x56..0x57], written by original-derived player turn and spawn/transition stores.',
         'masking':'Action9 reads unsigned full16bits. Use raw little-endian compat halfword, not camera heading or a masked accessor.',
         'cases':cases,'regions':[{'start':hex(lo),'end_exclusive':hex(hi),'sha256':sha(image[lo-base+2048:hi-base+2048])} for lo,hi in [(0x8003be40,0x8003be6c),(0x80042154,0x80042238)]],
         'external_premises':['Work/input values and action9 angle results are authored.', 'Source player entry pointer, original turn helper and original action9 body execute.','Full player callback, whole encounter and live parity are not claimed.']}
 a.out.mkdir(parents=True,exist_ok=False);(a.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({'pointer_checks':1,'cases':len(cases),'instructions':instructions}))
if __name__=='__main__':main()

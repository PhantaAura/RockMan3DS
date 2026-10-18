#!/usr/bin/env python3
"""Hash-gated owner MIPS scalar cases; excludes actual integration/audio and retail gameplay parity."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'research_01414'))
from player_contact_source_probe import CPU,SUPPORTED_EXE
FIELDS=((9,1),(10,1),(11,1),(0xac,1),(0x44,2),(0x46,2),(0x48,2),(0x74,1),(0x116,2),(0x175,1))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
 image=a.exe.read_bytes();digest=hashlib.sha256(image).hexdigest()
 if digest!=SUPPORTED_EXE:raise ValueError('unsupported executable')
 base=struct.unpack_from('<I',image,0x18)[0];rows=[]
 for phase in range(8):
  for vertical in (-800,-577,-576,-448,-48,0,48):
   for held in (0,0x10,0x4000,0x4010):
    for control in (-1,0,1):
     c=CPU(image,base);c.r[29]=0x4000;c.r[4]=0x1000;c.r[5]=0;c.run(0x800425fc);c.r[4]=0x1000;c.r[31]=0;c.steps=0
     # Only two excluded effects: original motion integration and sound. All
     # action8 branching, air-control work, animation and shot helpers execute.
     for addr in (0x80041e90,0x8001a008):c.put(addr,4,(31<<21)|8);c.put(addr+4,4,0)
     for off,value in ((8,1),(9,8),(10,phase),(0x74,1),(0xa7,control),(0xac,20),(0xad,20),(0x175,1)):c.put(0x1000+off,1,value)
     c.put(0x1044,2,512);c.put(0x1046,2,vertical);c.put(0x111c,2,held)
     c.run(0x8003dc5c)
     values=[c.get(0x1000+off,n,off in (0x44,0x46,0x48,0x116)) for off,n in FIELDS]
     rows.append((phase,vertical,held,control,*values))
 a.out.parent.mkdir(parents=True,exist_ok=True)
 if a.out.exists():raise ValueError('output exists; preserve earlier evidence')
 a.out.write_text('# phase vertical held control next_action next_phase next_0b animation forward vertical_after strafe contact yaw_offset shot_flags\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows))
 result={'format':'running-jump-source-evidence/1','status':'SOURCE_BACKED_PARTIAL','exe_sha256':digest,'entry':'0x8003DC5C','cases':len(rows),'field_offsets':[hex(o) for o,n in FIELDS],'fixture_sha256':hashlib.sha256(a.out.read_bytes()).hexdigest(),'profile':'action8 direction0, no modifier, high-jump flag0, contact1, request20, forward512','excluded_calls':['0x80041E90 motion integration','0x8001A008 sound'],'unknown':['other direction rows','modifier action3 landing','complete source tick/contact ordering','hardware timing'],'PHYSICS_1TO1':False,'source_blocks':[{'start':hex(lo),'end_exclusive':hex(hi),'sha256':hashlib.sha256(image[lo-base+2048:hi-base+2048]).hexdigest()} for lo,hi in ((0x800406a8,0x80040710),(0x8003dc5c,0x8003e068),(0x8003e068,0x8003e224))]}
 a.out.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()

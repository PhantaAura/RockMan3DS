#!/usr/bin/env python3
"""Read-only bounded scalar/profile snapshot; never retain raw PS1 RAM."""
import argparse,hashlib,json,struct,urllib.request
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--api',default='http://127.0.0.1:18081');a=p.parse_args()
exe=a.exe.read_bytes()
expected='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
assert hashlib.sha256(exe).hexdigest()==expected,'Owner executable profile mismatch'
load=struct.unpack_from('<I',exe,0x18)[0]
ram=urllib.request.urlopen(a.api.rstrip('/')+'/api/v1/cpu/ram/raw',timeout=8).read()
assert len(ram)==2097152,'Only original2MB reference supported'
def off(addr):
 v=addr&0xffffff
 assert v+4<=len(ram),'Snapshot address outside RAM'
 return v
def u32(addr):return struct.unpack_from('<I',ram,off(addr))[0]
def u16(addr):return struct.unpack_from('<H',ram,off(addr))[0]
def i16(addr):return struct.unpack_from('<h',ram,off(addr))[0]
def u8(addr):return ram[off(addr)]
regions=[]
for begin,end in [(0x8004b984,0x8004b9fc),(0x80048fb0,0x80049028),(0x80064718,0x80064794)]:
 ex=exe[2048+begin-load:2048+end-load];live=ram[off(begin):off(end)]
 regions.append({'begin':hex(begin),'end_exclusive':hex(end),'owner_exe_range_sha256':hashlib.sha256(ex).hexdigest(),'live_ram_range_sha256':hashlib.sha256(live).hexdigest(),'matches':ex==live})
assert all(region['matches'] for region in regions), 'Live source code profile mismatch'
idx=[]
expected_cases={78:((-256,0,-8448),0x80194630),94:((256,0,-7424),0x801947b0),123:((-256,0,-5376),0x80194600)}
for number,(xyz_expected,desc_expected) in expected_cases.items():
 address=0x8015c000+12*number;xyz=[i16(address+2*i) for i in range(3)];raw6=u16(address+6);word=u32(address+8);desc=word&0xffffff
 row={'source_idx_index':number,'position':xyz,'raw6':raw6,'descriptor_word':hex(word),'source_identity_matches':tuple(xyz)==xyz_expected and word==desc_expected}
 if desc and desc+12<=len(ram):
  alt=u32(desc+4);default=u32(desc+8);selected=alt if raw6&0xfff else default
  row.update({'alternate_contact_pointer':hex(alt),'default_contact_pointer':hex(default),'selected_contact_pointer':hex(selected),'selection':'alternate' if raw6&0xfff else 'default'})
  if (selected&0xffffff)+12<=len(ram):row['selected_pointer_plus8_word']=hex(u32(selected+8))
 idx.append(row)
actors=[]
for slot in range(16):
 work=0x8009f108+slot*0x3c0
 if u8(work)&1 and (u8(work+1)&0xe0)==0x20 and u8(work+2)==3:
  actors.append({'slot':slot,'work':hex(work),'active':u8(work),'class_slot_raw':u8(work+1),'subtype_raw':u8(work+2),'state':u8(work+8),'state_raw_9':u8(work+9),'state_raw_a':u8(work+10),'position':[i16(work+0x14+2*i) for i in range(3)],'stats_index':u8(work+0xc),'context_high_byte':u8(work+0xf),'hp':i16(work+0x6e)})
result={'schema':'redux-contact-live-profile/2','status':'SOURCE_BACKED_PARTIAL','scope':'Read-only original-game emulator snapshot; no hardware or retail parity claim','api':'localhost:18081 GET /api/v1/cpu/ram/raw','read_bytes':len(ram),'raw_ram_retained':False,'source_exe_sha256':expected,'regions':regions,'profile_match':all(x['matches'] for x in regions),'ram_u32_at_0x00000008':u32(8),'raw_stats_selector':u8(0x800c1b6b),'stats_table_pointer':hex(u32(0x80098844)),'player_active':u8(0x800b51b0),'player_class_slot_raw':u8(0x800b51b1),'player_state':u8(0x800b51b8),'player_state_raw_9':u8(0x800b51b9),'player_state_raw_a':u8(0x800b51ba),'player_position':[i16(0x800b51c4+2*i) for i in range(3)],'idx':idx,'actors_class20_03':actors,'limits':['Scene must be confirmed by original rendered capture/user observation.','One snapshot does not prove mutable IDX semantics or all gameplay NULL behavior.']}
del ram
if a.out.exists():raise SystemExit('Refusing to overwrite a live reference snapshot')
a.out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ['profile_match','ram_u32_at_0x00000008','raw_stats_selector','player_active','player_class_slot_raw','player_state','player_position','idx','actors_class20_03']}))

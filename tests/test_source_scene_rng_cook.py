import copy,json,struct,sys,unittest,zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import cook_source_scene_rng_tape as tape
import cook_source_rng_profile as rng

def blob(rows):return b''.join((json.dumps(r)+'\n').encode() for r in rows)
def authored():
 global_rows=[dict(kind='observer_start',bios_sha256=rng.BIOS_SHA,source_exe_sha256=rng.EXE_SHA,overlay_sha256=rng.OVERLAY_SHA,scope='CURRENT_INSTALLATION',seed32=0,max_rows=128,game_boot_parity=False,retail_parity=False),dict(kind='game_entry',vsync=1,game_entry_count=1,rand_ordinal=0,seed32=0,source_identity_matches=True)]
 seed=0;values=[]
 for n in range(1,8):
  before=seed;seed=(seed*1103515245+12345)&0xffffffff;value=(seed>>16)&32767;ra=0x801133e4 if n==1 else tape.RAS[n-2]
  global_rows.append(dict(kind='bios_rng_enter',vsync=2+n,rand_ordinal=n,game_entry_count=1,service=47,caller_ra=ra,seed_before32=before,a0_argument=0))
  global_rows.append(dict(kind='bios_rng_return',vsync=2+n,rand_ordinal=n,service=47,caller_ra=ra,seed_before32=before,seed_after32=seed,v0_actual=value));values.append((seed,value))
 ctor=dict(kind='scene_constructor',pc=0x80100efc,loops=100,vsync=4,seed32=values[0][0],rand_ordinal=1,pending_rng=False,event_active=1,event_id=1,event_phase=5,event_frame=0,source_actor=0,actor_valid=False,actor_state08=255,position=[0,0,0],pool80_low=0,pool80_high=0,scratch_half2=21,scratch0=1,scratch1=2,main_phase=3,main_subphase=2,a0=0x800c4c48,callback_entry=0,read_only=True,retail_parity=False)
 callback=dict(ctor);callback.update(kind='scene_rng_callback',pc=0x80100d6c,event_phase=6,source_actor=0x800c3580,actor_valid=True,actor_state08=1,position=[7,9,11],callback_entry=1,scratch_half2=22)
 rows=[dict(kind='scene_observer_start',read_only=True,retail_parity=False),ctor,callback]
 for n in range(6):rows.append(dict(kind='scene_rng_return',pc=tape.RAS[n],callback_entry=1,event_frame=0,rand_ordinal=n+2,seed32=values[n+1][0],v0_actual=values[n+1][1],pending_rng=True,read_only=True,retail_parity=False))
 return rows,global_rows
class TestTape(unittest.TestCase):
 def test_encode_pair(self):
  s,g=authored();out,p=tape.cook(blob(s),blob(g));self.assertEqual(len(out),400);self.assertEqual(tape.validate_package(out),1);self.assertEqual(struct.unpack_from('<H6B',out,224),(21,1,2,3,2,1,5));self.assertEqual(struct.unpack_from('<I',out,24)[0],zlib.crc32(out[32:])&0xffffffff);self.assertEqual(p['scene_returns'],6);self.assertFalse(p['retail_parity'])
 def test_reject_wrong_context_and_returns(self):
  mutations=[(1,'pc',0),(1,'pending_rng',True),(2,'source_actor',0),(2,'actor_valid',False),(2,'actor_state08',2),(2,'event_frame',1),(2,'position',[True,9,11]),(2,'pool80_low',-1),(2,'scratch_half2',65536),(2,'extra',1),(3,'pc',0),(3,'seed32',0),(3,'v0_actual',65536),(3,'rand_ordinal',1),(3,'callback_entry',2),(3,'event_frame',1),(3,'read_only',False),(3,'retail_parity',True)]
  for index,key,value in mutations:
   with self.subTest(index=index,key=key):
    s,g=authored();s[index][key]=value
    with self.assertRaises(ValueError):tape.cook(blob(s),blob(g))
 def test_global_gaps_and_conditional_count(self):
  s,g=authored()
  for rows in (s[:-1],s+[s[-1]],s[:2]+s[3:]):
   with self.assertRaises(ValueError):tape.cook(blob(rows),blob(g))
  bad=copy.deepcopy(g);bad[3]['v0_actual']^=1
  with self.assertRaises(ValueError):tape.cook(blob(s),blob(bad))
  s[2]['pool80_low']=0xffffffff;s[2]['pool80_high']=0xffffffff
  with self.assertRaises(ValueError):tape.cook(blob(s),blob(g))
 def test_malformed_package(self):
  s,g=authored();data,_=tape.cook(blob(s),blob(g))
  for offset in (0,4,6,8,12,16,20,24,28,32,96,226,227,228,232,256,264,265,274,284,287,288,296,304,312,384):
   with self.subTest(offset=offset):
    bad=bytearray(data);bad[offset]^=255
    if offset>=32:struct.pack_into('<I',bad,24,zlib.crc32(bad[32:])&0xffffffff)
    with self.assertRaises(ValueError):tape.validate_package(bytes(bad))
  for bad in (data[:-1],data+bytes(1)):
   with self.assertRaises(ValueError):tape.validate_package(bad)
 def test_valid_scalar_domain_is_not_accuracy_proof(self):
  s,g=authored();data,_=tape.cook(blob(s),blob(g));changed=bytearray(data)
  changed[224]^=255;struct.pack_into('<I',changed,24,zlib.crc32(changed[32:])&0xffffffff)
  self.assertEqual(tape.validate_package(bytes(changed)),1)
  s[3]['v0_actual']^=1
  with self.assertRaises(ValueError):tape.cook(blob(s),blob(g))
 def test_duplicate_or_partial_json(self):
  s,g=authored();raw=blob(s)
  with self.assertRaises(ValueError):tape.cook(raw[:-1],blob(g))
  with self.assertRaises(ValueError):tape.cook(raw.replace(b'"pc":',b'"pc":0,"pc":',1),blob(g))
if __name__=='__main__':unittest.main()

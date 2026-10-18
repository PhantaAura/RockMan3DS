#!/usr/bin/env python3
"""Private, bounded replay of OBSERVED scene RNG returns; no generated seed.

Pairs semantic source return PCs with the complete audited global RNG stream.
This is selected OpenBIOS replay, not retail or native chronology proof.
"""
import argparse,hashlib,json,struct,zlib
from pathlib import Path
import cook_source_rng_profile as rng
from cook_source_loop_snapshot import validate_resource_owners,EBD_SHA
HEADER,RECORD,MAX_RECORDS=256,144,64
RAS=(0x80100dd4,0x80100dec,0x80100e04,0x80100e40,0x80100e58,0x80100e70)
CONTEXT=set('kind pc loops vsync seed32 rand_ordinal pending_rng event_active event_id event_phase event_frame source_actor actor_valid actor_state08 position pool80_low pool80_high scratch_half2 scratch0 scratch1 main_phase main_subphase a0 callback_entry read_only retail_parity'.split())
RETURN=set('kind pc callback_entry event_frame rand_ordinal seed32 v0_actual pending_rng read_only retail_parity'.split())
def require(ok,why):
 if not ok:raise ValueError(why)
def integer(row,key,maximum=0xffffffff):
 v=row.get(key);require(type(v) is int and 0<=v<=maximum,'Invalid bounded scalar '+key);return v
def observed_rows(blob):
 rows,_,partial=rng.decode_snapshot(blob,False,max_bytes=1024*1024)
 require(not partial and len(rows)<=4096,'Invalid semantic capture budget')
 for i,row in enumerate(rows):
  fields=CONTEXT if row.get('kind') in ('scene_constructor','scene_rng_callback') else RETURN if row.get('kind')=='scene_rng_return' else set('kind read_only retail_parity'.split()) if i==0 and row.get('kind')=='scene_observer_start' else None
  require(fields is not None and set(row)==fields,'Unsupported semantic row/fields')
  require(row['read_only'] is True and row['retail_parity'] is False,'Writable/parity semantic row')
  if fields==CONTEXT:
   for key in fields-set('kind pending_rng actor_valid position read_only retail_parity'.split()):integer(row,key)
   for key in ('event_active','event_id','event_phase','actor_state08','scratch0','scratch1','main_phase','main_subphase'):integer(row,key,255)
   integer(row,'scratch_half2',65535)
   require(type(row['pending_rng']) is bool and type(row['actor_valid']) is bool,'Invalid context flags')
   p=row['position'];require(type(p) is list and len(p)==3 and all(type(v) is int and -32768<=v<=32767 for v in p),'Invalid actor position')
  if fields==RETURN:
   for key in ('pc','callback_entry','event_frame','rand_ordinal','seed32'):integer(row,key)
   integer(row,'v0_actual',32767);require(type(row['pending_rng']) is bool,'Invalid return flag')
 return rows
def cook(semantic,global_trace):
 _,global_proof=rng.cook(global_trace,capture_prefix=True)
 require(global_proof['excluded_complete_rows']==0 and not global_proof['partial_tail_excluded'],'Paired global trace must finish on a complete return')
 global_rows,_,_=rng.decode_snapshot(global_trace,False)
 returns={r['rand_ordinal']:r for r in global_rows if r['kind']=='bios_rng_return' and r['service']==47}
 rows=observed_rows(semantic);ctors=[r for r in rows if r['kind']=='scene_constructor'];require(len(ctors)==1,'Exactly one paired constructor required');ctor=ctors[0]
 require((ctor['pc'],ctor['a0'],ctor['event_active'],ctor['event_id'],ctor['event_phase'],ctor['event_frame'],ctor['scratch1'],ctor['main_phase'],ctor['main_subphase'])==(0x80100efc,0x800c4c48,1,1,5,0,2,3,2) and ctor['scratch0'] in (0,1) and not ctor['pending_rng'],'Wrong constructor boundary')
 boundary=returns.get(ctor['rand_ordinal']);require(boundary and boundary['seed_after32']==ctor['seed32'],'Unpaired constructor RNG state')
 groups=[]
 for row in rows[rows.index(ctor)+1:]:
  if row['kind']=='scene_rng_callback':groups.append([row,[]])
  else:
   require(row['kind']=='scene_rng_return' and groups,'Return without callback');groups[-1][1].append(row)
 require(1<=len(groups)<=MAX_RECORDS,'Callback count exceeds bounded replay')
 payload=bytearray()
 for index,(row,calls) in enumerate(groups):
  require((row['pc'],row['a0'],row['event_active'],row['event_id'],row['event_phase'],row['event_frame'],row['callback_entry'])==(0x80100d6c,0x800c4c48,1,1,6,index,index+1),'Wrong or discontinuous callback context')
  require(not row['pending_rng'] and row['actor_valid'] and row['actor_state08']==1 and row['scratch1']==2 and row['main_phase']==3 and row['main_subphase']==2,'Unsupported callback ownership')
  actor=row['source_actor'];require(actor>=0x800c3580 and (actor-0x800c3580)%1444==0 and (actor-0x800c3580)//1444<4,'Invalid actor pool identity')
  free=64-bin(row['pool80_low']).count('1')-bin(row['pool80_high']).count('1');count=3*min(2,free) if index<31 else 0
  require(len(calls)==count,'Conditional allocator/RNG call count mismatch')
  before=returns.get(row['rand_ordinal']);require(before and before['seed_after32']==row['seed32'],'Unpaired callback initial RNG state')
  raw=bytearray(RECORD);struct.pack_into('<IIBBH3HH2I4BQQ',raw,0,index,actor,1,count,row['scratch_half2'],*[v&65535 for v in row['position']],0,row['pool80_low'],row['pool80_high'],1,6,1,0,row['rand_ordinal'],row['rand_ordinal']+count)
  for n,call in enumerate(calls):
   ordinal=row['rand_ordinal']+n+1;original=returns.get(ordinal)
   require(call['callback_entry']==index+1 and call['event_frame']==index and call['pc']==RAS[n] and call['rand_ordinal']==ordinal,'Unpaired semantic return/caller order')
   require(original and original['caller_ra']==call['pc'] and original['v0_actual']==call['v0_actual'] and original['seed_after32']==call['seed32'],'Semantic return differs from completed global observation')
   struct.pack_into('<IIQ',raw,48+n*16,call['pc'],call['v0_actual'],ordinal)
  payload+=raw
 header=bytearray(HEADER);struct.pack_into('<4sHH5I',header,0,b'L3SR',1,HEADER,1,len(groups),RECORD,HEADER+len(payload),0)
 header[32:224]=b''.join(bytes.fromhex(v) for v in (rng.EXE_SHA,rng.OVERLAY_SHA,EBD_SHA,rng.BIOS_SHA,rng.sha(semantic),rng.sha(global_trace)))
 struct.pack_into('<H6B',header,224,ctor['scratch_half2'],ctor['scratch0'],2,3,2,1,5)
 package=header+payload;struct.pack_into('<I',package,24,zlib.crc32(package[32:])&0xffffffff)
 proof={'schema':'mml-recorded-scene-rng/1','status':'MATCHES_MEASURED_FIELDS','scope':'RECORDED_OPENBIOS_SCENE','callback_records':len(groups),'scene_returns':sum(len(c) for _,c in groups),'audited_global_returns':global_proof['actual_rand_returns'],'semantic_trace_sha256':rng.sha(semantic),'global_trace_sha256':rng.sha(global_trace),'paired_constructor':ctor,'first_callback':groups[0][0],'package_sha256':rng.sha(package),'package_bytes':len(package),'read_only':True,'retail_parity':False,'hardware_tested':False,'memory_cards_inserted':False,'source_payload_embedded':False,'native_context_must_match':True,'limitations':['Actual observed return replay only; no generated/default seed or universal chronology.','Original effect updates/reclamation and later actor update/animation are not proved by these receipts.','Runtime must hold on any position/pool/scratch/owner mismatch.','Loop listener count does not define scratch counter; constructor halfword is directly observed.']}
 return bytes(package),proof

def validate_package(data):
 require(type(data) is bytes and HEADER+RECORD<=len(data)<=HEADER+MAX_RECORDS*RECORD,'Wrong L3SR bounded size')
 magic,version,header,scope,count,record,total,crc,reserved=struct.unpack_from('<4sHH6I',data)
 require((magic,version,header,scope,record,total,reserved)==(b'L3SR',1,HEADER,1,RECORD,len(data),0) and 1<=count<=MAX_RECORDS and total==HEADER+count*RECORD,'Wrong L3SR envelope')
 require(crc==zlib.crc32(data[32:])&0xffffffff,'Wrong L3SR checksum')
 require(data[32:160]==b''.join(bytes.fromhex(v) for v in (rng.EXE_SHA,rng.OVERLAY_SHA,EBD_SHA,rng.BIOS_SHA)),'Wrong L3SR owner identities')
 require(data[160:192]!=bytes(32) and data[192:224]!=bytes(32),'Missing observed trace identities')
 half,scratch,cadence,main,sub,event,phase=struct.unpack_from('<H6B',data,224)
 require(scratch in (0,1) and (cadence,main,sub,event,phase)==(2,3,2,1,5) and data[232:256]==bytes(24),'Wrong paired constructor/reserved fields')
 prior=0
 for i in range(count):
  raw=data[HEADER+i*RECORD:HEADER+(i+1)*RECORD]
  frame,actor,state,calls,half,x,y,z,reserved,low,high,event,phase,active,reserved_byte,before,after=struct.unpack_from('<IIBBH3HH2I4BQQ',raw)
  require(frame==i and state==1 and calls in (0,3,6) and reserved==0 and reserved_byte==0 and (event,phase,active)==(1,6,1),'Wrong recorded callback fields')
  require(actor>=0x800c3580 and (actor-0x800c3580)%1444==0 and (actor-0x800c3580)//1444<4,'Wrong recorded actor')
  expected=3*min(2,64-bin(low).count('1')-bin(high).count('1')) if frame<31 else 0
  require(calls==expected and 0<before<=after<=32767 and after==before+calls and before>=prior,'Wrong conditional call/observed ordinal context');prior=after
  for n in range(6):
   ra,value,ordinal=struct.unpack_from('<IIQ',raw,48+n*16)
   require((ra==RAS[n] and value<=32767 and ordinal==before+n+1) if n<calls else (ra,value,ordinal)==(0,0,0),'Wrong recorded return/order')
 return count

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for arg in ('semantic','global-trace','profile','exe','bios','st00','area','out'):p.add_argument('--'+arg,type=Path,required=True)
 a=p.parse_args();require(not a.out.exists(),'Choose a new private output directory')
 source=Path(__file__).resolve().parents[1];out=a.out.resolve();require(out!=source and source not in out.parents,'Private output must be outside source')
 profile=json.loads(a.profile.read_text());require(profile.get('read_only_observer') is True and profile.get('retail_parity') is False and profile.get('memory_cards_inserted') is False,'Invalid reference profile')
 require(profile.get('scene_observer_sha256')==rng.sha((source/'docs/research_01434/scene_rng_observer.lua').read_bytes()),'Scene observer identity mismatch')
 require(rng.sha(a.exe.read_bytes())==rng.EXE_SHA and rng.sha(a.bios.read_bytes())==rng.BIOS_SHA and profile.get('exe_sha256')==rng.EXE_SHA and profile.get('bios_sha256')==rng.BIOS_SHA,'Owner/reference identities differ')
 from build_stage_catalog import iter_resources
 validate_resource_owners(list(iter_resources(a.st00)),list(iter_resources(a.area)))
 package,proof=cook(a.semantic.read_bytes(),a.global_trace.read_bytes());validate_package(package);proof['reference_profile_sha256']=rng.sha(a.profile.read_bytes())
 a.out.mkdir(parents=True);(a.out/'source_scene_rng.l3sr').write_bytes(package);(a.out/'provenance.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps({k:proof[k] for k in ('status','callback_records','scene_returns','audited_global_returns','package_bytes','package_sha256')}))
if __name__=='__main__':main()

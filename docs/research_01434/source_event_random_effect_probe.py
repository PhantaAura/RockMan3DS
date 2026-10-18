#!/usr/bin/env python3
"""Focused unchanged80100D6C/family80/selected-BIOS oracle on authored owners.

Actual BIOS rand instructions execute at declared A0 service boundaries. No
owner instruction bytes, BIOS/overlay payloads or original tables are exported.
All differential fixtures stay in the NEW private evidence directory.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources
BIOS_SHA='713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
WORK,POOL=0x800c4c48,0x800a3b40
BIOS_RAND,RAND_STATE,RAND_WRAPPER=0xbfc060e4,0x8448,0x8007fe50
RAS=(0x80100dd4,0x80100dec,0x80100e04,0x80100e40,0x80100e58,0x80100e70)
def sha(data):return hashlib.sha256(data).hexdigest()
def authored(seed,count):return bytes((seed*37+i*17+count*11)&255 for i in range(count))
def fill(cpu,address,data):
 for i,value in enumerate(data):cpu.put(address+i,1,value)
def read(cpu,address,count):return bytes(cpu.get(address+i,1) for i in range(count))
def diff(before,after):return ','.join(f'{i}:{value}' for i,(old,value) in enumerate(zip(before,after)) if old!=value) or '-'
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','st00','bios','out'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();out=a.out.resolve()
 if out.exists() or SOURCE==out or SOURCE in out.parents:raise ValueError('Choose NEW private evidence directory')
 image,bios=a.exe.read_bytes(),a.bios.read_bytes();owner=OwnerExe(image)
 if sha(bios)!=BIOS_SHA:raise ValueError('Unsupported selected BIOS')
 overlays=[r['payload'] for r in iter_resources(a.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper()]
 if len(overlays)!=1 or sha(overlays[0])!=OVERLAY_SHA:raise ValueError('Unsupported overlay')
 overlay=overlays[0];rows=[];steps=wrappers=services=0
 frames=(0,6,30,31,0x7fffffff,0x80000000,0xffffffff)
 bitmaps=((0,0),(0xfffffffe,0),(0xffffffff,0xfffffffe),(0xffffffff,0xffffffff),
          (0xfffefffe,0xffffffff),(0xfffffffc,0xffffffff))
 seeds=(0,1,0xffffffff,0x12345678,0x80000000,0xa5a5a5a5)
 positions=((0,0,0),(32767,32768,65535),(65535,0,32767),(31,15,31),(65472,65520,65472),(1234,5678,9012))
 for frame in frames:
  for variation,(used0,used1) in enumerate(bitmaps):
   case=len(rows);c=OverlayProbe(image,owner.base,overlay);c.r[29]=0x700000;c.r[4]=WORK
   c.mem.update({0xbfc00000+i:value for i,value in enumerate(bios)})
   work=bytearray(authored(case,168));work[0]=work[1]=1;work[4]=6
   actor_slot=case%4;actor=0x800c3580+actor_slot*1444
   struct.pack_into('<I',work,16,frame);struct.pack_into('<I',work,36,actor);fill(c,WORK,work)
   fill(c,actor,authored(case,1444));c.put(actor+1,1,0x40|actor_slot)
   for axis,value in enumerate(positions[variation]):c.put(actor+20+axis*2,2,value)
   c.put(0x80098b20,4,0x80000000>>actor_slot)
   pool=bytearray(authored(case,64*72))
   for slot in range(64):pool[slot*72+1]=0x80|slot
   fill(c,POOL,pool);c.put(0x80098838,4,used0);c.put(0x8009883c,4,used1)
   c.put(0x80122424,1,255);c.put(RAND_STATE,4,seeds[variation]);calls=[];pc=0x80100d6c
   while True:
    c.run(pc,RAND_WRAPPER)
    # Return0 ends the unchanged callback. Otherwise the original JAL RA
    # identifies the exact source request at this declared A0 ABI boundary.
    if c.r[31]==0:break
    ra=c.r[31]
    if len(calls)>=6 or ra!=RAS[len(calls)]:raise ValueError('Unexpected original random call order')
    before=c.get(RAND_STATE,4);c.run(RAND_WRAPPER,0xa0);assert c.r[9]==47
    c.run(BIOS_RAND,ra);value=c.r[2];after=c.get(RAND_STATE,4)
    assert value<=32767
    calls.append((ra,value,1001+len(calls),before,after));wrappers+=1;services+=1;pc=ra
   assert not c.return_only_hooks and read(c,WORK,168)==bytes(work)
   active=frame<31 or frame>=0x80000000
   expected_free=min(2,64-(bin(used0).count('1')+bin(used1).count('1'))) if active else 0
   assert len(calls)==expected_free*3
   alloc=[];bitmap=(c.get(0x80098838,4),c.get(0x8009883c,4))
   for bank in range(2):
    added=bitmap[bank]^(used0 if bank==0 else used1)
    for slot in range(32):
     if added&(0x80000000>>slot):alloc.append(bank*32+slot)
   assert len(alloc)==expected_free
   control=c.get(0x80122424,1);assert control==(0 if frame==6 else 255)
   calltext=','.join(f'{ra}:{value}:{ordinal}' for ra,value,ordinal,_,_ in calls) or '-'
   rows.append([case,frame,used0,used1,actor_slot,*positions[variation],len(calls),
                ','.join(map(str,alloc)) or '-',calltext,diff(pool,read(c,POOL,64*72)),*bitmap,control])
   steps+=c.steps
 fixture=('case frame used0 used1 actor_slot pos0 pos1 pos2 calls slots receipts pool80 out0 out1 control\n'+
          ''.join(' '.join(map(str,row))+'\n' for row in rows)).encode()
 proof={'schema':'mml-source-event-random-effect/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,
        'overlay_sha256':OVERLAY_SHA,'selected_bios_sha256':BIOS_SHA,'callback':'80100D6C','cases':len(rows),
        'unchanged_original_instructions':steps,'selected_original_bios_calls':services,'original_wrappers_executed':wrappers,
        'declared_A0_selected_service_boundaries':services,'instruction_patches':0,'helper_hooks':0,
        'original_allocator':'80031618','pool_base':'800A3B40','pool_records':64,'record_bytes':72,
        'observed_rng_counts':sorted(set(row[8] for row in rows)),
        'frame6_control_store_proved':True,'signed_negative_frame_branch_proved':True,
        'fixture_sha256':sha(fixture),'fixture_owner_local':True,'retail_parity':False,'hardware_tested':False,
        'limits':['Initial BIOS states and pool/actor records are authored differential inputs, not a live scene seed.',
                  'Unchanged selected BIOS rand runs at declared A0 ABI boundaries; the full BIOS dispatcher and competing consumers are excluded.',
                  'This proves selected scalar stores/conditional allocation/call order, not resource/update/draw semantics or complete RNG chronology.',
                  'Pool residency/header knowledge and completed RNG receipts remain explicit native admission requirements.',
                  'No original code, tables, BIOS/overlay payload, retail assets or saves are exported.']}
 out.mkdir(parents=True);(out/'source-event-random-effect.tsv').write_bytes(fixture)
 (out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
 print(json.dumps({key:proof[key] for key in ('cases','unchanged_original_instructions','selected_original_bios_calls','observed_rng_counts','instruction_patches','helper_hooks')}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Unchanged owner pad, phase-dispatch and cadence oracle; scalar/authored output only."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from tutorial_actor_identity_probe import OverlayProbe,SUPPORTED_EXE

PAD0,PAD1=0x800c0c00,0x800c0d68
def authored(seed,port):return bytes((seed*37+port*113+i*17)&255 for i in range(72))
def fill(cpu,address,data):
 for i,value in enumerate(data):cpu.put(address+i,1,value)
def read(cpu,address,count):return bytes(cpu.get(address+i,1) for i in range(count))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--exe',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
 a=p.parse_args()
 if a.out.exists():raise ValueError('Choose new evidence directory')
 image=a.exe.read_bytes()
 if hashlib.sha256(image).hexdigest()!=SUPPORTED_EXE:raise ValueError('Unsupported owner EXE')
 base=struct.unpack_from('<I',image,24)[0]
 def cpu():
  c=OverlayProbe(image,base,b'');c.r[29]=0x700000;return c
 pad_steps=boot_steps=gate_steps=phase_steps=vblank_steps=suffix_steps=0
 rows=[]
 for seed in range(256):
  c=cpu();before=[]
  for port,address in enumerate((PAD0,PAD1)):
   data=bytearray(authored(seed,port));data[0]=255 if (seed+port)%7==0 else 0
   data[1]=(seed*19+port*0x41)&255;data[37]=seed&3
   for i in range(6):data[56+i]=(seed+i)%3;data[64+i]=(seed+i)%9
   before.append(bytes(data));fill(c,address,data)
  c.run(0x80012298);assert not c.return_only_hooks
  rows.append([seed,before[0].hex(),before[1].hex(),read(c,PAD0,72).hex(),read(c,PAD1,72).hex()]);pad_steps+=c.steps
 # Exact actual cold-pad clear loops cover1228bytes per owner. Only the first
 #72 authored bytes are emitted, never original data or the adjacent owners.
 for port,address in enumerate((PAD0,PAD1)):
  c=cpu();fill(c,address,authored(213,port));c.r[4]=address;c.r[5]=address+1224
  c.run(0x800177cc);assert read(c,address,72)==bytes(72);boot_steps+=c.steps
 scratch=[]
 for before in (0,1,2,3,32767,32768,65534,65535):
  c=cpu();c.put(0x1f800000,4,0xabab|(before<<16));c.run(0x80012004,0x80012020)
  assert c.get(0x1f800000,1)==0 and c.get(0x1f800001,1)==2 and c.get(0x1f800002,2)==before
  scratch.append([before,c.get(0x1f800002,2)]);boot_steps+=c.steps
 gates=[]
 for cadence in (0,1,2,4,255):
  for elapsed in (0,1,2,3,4,254,255,256,32768,0xffffffff):
   c=cpu();c.r[18]=0x1f800001;c.put(0x1f800001,1,cadence);c.put(0x80098888,4,elapsed)
   c.run(0x80011ce8,0x80011cfc);ready=int(c.r[2]==0)
   c.run(0x80011cfc,0x80011d04 if ready else 0x80011cec)
   assert ready==int(elapsed>=cadence)
   gates.append([cadence,elapsed,ready]);gate_steps+=c.steps
 phases=[]
 for subphase in range(-1,10):
  c=cpu();c.r[16]=0x800c1b10;c.r[17]=0x80082114;c.put(0x800c1b10,1,3)
  c.run(0x800155d4,0x800158f0);assert c.r[4]==0x800c1b10
  c.put(0x800c1b11,1,subphase);c.run(0x8001591c,0x80015944 if 0<=subphase<8 else 0x80015d98)
  if 0<=subphase<8:
   branch=c.r[2];c.run(0x80015944,branch)
   if subphase in (1,3,4,5,7):c.run(branch,0x80015db4);ready=1
   else:ready=0
  else:branch=0x80015d98;ready=0
  phases.append([3,subphase,ready]);phase_steps+=c.steps
 active_phases=[]
 for active in (1,2,7,128,255):
  c=cpu();c.r[16]=0x800c1b10;c.r[18]=0x800c3558
  c.put(0x800c356c,1,0);c.put(0x800c4c48,1,active)
  c.run(0x800159f0,0x80015db4)
  active_phases.append([3,2,0,active,1]);phase_steps+=c.steps
 vblanks=[]
 for before in (0,1,2,255,65535,0x7fffffff,0x80000000,0xfffffffe,0xffffffff):
  c=cpu();c.put(0x80098888,4,before);c.run(0x800123fc);after=c.get(0x80098888,4)
  assert after==(before+1)&0xffffffff;vblanks.append([before,after]);vblank_steps+=c.steps
 # Each unchanged caller JAL/delay-slot slice ends at its original callee.
 # Declared external return boundaries resume at the real RA; no callee is
 # replaced, no original instruction is patched, and the suffix is not claimed
 # to execute the opaque functions' internal side effects.
 calls=[(0x80011e70,0x80012c80),(0x80011e78,0x8001a110),(0x80011e80,0x8001c95c),
        (0x80011e88,0x800203a8),(0x80011e90,0x80053b40),(0x80011e98,0x80064060),
        (0x80011ea0,0x8007d02c),(0x80011ea8,0x80017b24),(0x80011eb0,0x800178fc)]
 c=cpu();suffix=[]
 for pc,target in calls:
  c.run(pc,target);suffix.append({'caller':hex(pc),'consumer':hex(target),'resume':hex(c.r[31])})
  if target==0x8007d02c:assert c.r[4]==0
 c.r[28]=0x10000;c.put(0x10000,4,0);c.put(0x1f800002,2,65535)
 c.run(0x80011eb8,0x80011ce8);assert c.get(0x1f800002,2)==0;suffix_steps=c.steps
 a.out.mkdir(parents=True)
 files={
  'source-main-loop-pads.tsv':'seed before0 before1 after0 after1\n'+''.join(' '.join(map(str,row))+'\n' for row in rows),
  'source-main-loop-gates.tsv':'cadence elapsed ready\n'+''.join(' '.join(map(str,row))+'\n' for row in gates),
  'source-main-loop-phase.tsv':'phase subphase ready\n'+''.join(' '.join(map(str,row))+'\n' for row in phases),
  'source-main-loop-phase-active.tsv':'phase subphase control active ready\n'+''.join(' '.join(map(str,row))+'\n' for row in active_phases),
  'source-main-loop-vblank.tsv':'before after\n'+''.join(' '.join(map(str,row))+'\n' for row in vblanks)}
 for name,data in files.items():(a.out/name).write_text(data)
 proof={'schema':'mml-source-main-loop-scalar/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':SUPPORTED_EXE,
        'pad_cases':len(rows),'pad_original_instructions':pad_steps,'pad_cold_clear_cases':2,
        'scratch_prefix_cases':len(scratch),'scratch_prefix_preserves_half2':True,'cold_prefix_original_instructions':boot_steps,
        'gate_cases':len(gates),'gate_original_instructions':gate_steps,'phase_cases':len(phases),
        'phase_active_event_fastpath_cases':len(active_phases),
        'phase_original_instructions':phase_steps,'vblank_cases':len(vblanks),'vblank_original_instructions':vblank_steps,
        'suffix_call_boundary_cases':len(suffix),'suffix_original_instructions':suffix_steps,'suffix':suffix,
        'declared_opaque_callee_return_boundaries':len(calls),'helper_hooks':0,'instruction_patches':0,
        'fixture_sha256':{name:hashlib.sha256(data.encode()).hexdigest() for name,data in files.items()},
        'scratch_half_1f800002_initial_owner':'UNKNOWN','retail_parity':False,'hardware_tested':False,
        'limits':['Authored pad/scalar owners, not observed retail controller or BIOS chronology.',
                  'Phase3 subphases0/6 and subphase2 outside the admitted active-event fast path require further branch owners.',
                  'The original80015DB4 update pass invokes other actor families before the E0 sweep.',
                  'Suffix proof checks call ordering only; opaque callees are external return boundaries.',
                  'Boot scratch prefix and BIOS/game BSS do not establish initial scratch halfword2.',
                  'No original code, retail asset, source table or save bytes are exported.']}
 (a.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
 print(json.dumps({key:proof[key] for key in ('pad_cases','pad_original_instructions','phase_cases','gate_cases','vblank_cases','scratch_half_1f800002_initial_owner')}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Run unchanged8010102C over authored callback/owner cases. No code/assets exported."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe,OVERLAY_SHA,iter_resources
WORK=0x800c4c48
POOLS=((0x800c3580,4,1444,0x40),(0x800a3b40,64,72,0x80),(0x800bf118,32,80,0xe0),(0x800a2d08,16,36,0xc0))
# These are measured callback identities; no executable table or code emitted.
CASES=((0x80100738,(0,425,465,1)),(0x801007f0,(0,1)),(0x8010086c,(0,30,31)),
       (0x8010089c,(0,1)),(0x801008a4,(0,1)),(0x801008d4,(0,20,21)),
       (0x80100920,(0,30,96,97)),(0x801009b0,(0,120,210,310,370,371)),
       (0x80100a3c,(0,50,51)),(0x80100a70,(0,60,61)),
       (0x80100a94,(0,26,33,40,49,50)),(0x80100be8,(0,1)),
       (0x80100c30,(0,1,13,25,68,69)),(0x80100d34,(0,1)),
       (0x80100e9c,(0,1)),(0x80100ecc,(0,1)))
def authored(seed,n):return bytes((seed*37+i*17+n*11)&255 for i in range(n))
def fill(c,a,data):
 for i,v in enumerate(data):c.put(a+i,1,v)
def read(c,a,n):return bytes(c.get(a+i,1) for i in range(n))
def diff(before,after):
 return ','.join(f'{i}:{v}' for i,(old,v) in enumerate(zip(before,after)) if old!=v) or '-'
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','st00','out'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args()
 if a.out.exists():raise ValueError('Choose new owner-local evidence directory')
 image=a.exe.read_bytes();owner=OwnerExe(image)
 overlay=next(r['payload'] for r in iter_resources(a.st00) if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
 if hashlib.sha256(overlay).hexdigest()!=OVERLAY_SHA:raise ValueError('Unsupported overlay')
 rows=[];instructions=0;seed=0
 for callback,frames in CASES:
  for frame in frames:
   for variation in range(3):
    c=OverlayProbe(image,owner.base,overlay);c.r[29]=0x700000;c.r[4]=WORK
    work=bytearray(authored(seed,168));work[0]=work[1]=1;work[4]=6;work[8]=variation
    struct.pack_into('<I',work,0x10,frame);struct.pack_into('<I',work,0xa4,0x8011b2f0)
    struct.pack_into('<I',work,0x24,POOLS[0][0])
    for index in range(1,16):struct.pack_into('<I',work,0x24+index*4,POOLS[2][0]+(index-1)*80)
    fill(c,WORK,work)
    buffers=[]
    for base,count,size,family in POOLS:
     data=bytearray(authored(seed,count*size))
     for slot in range(count):data[slot*size+1]=family|slot
     if family==0xc0:data[3]=(seed+3)&255
     fill(c,base,data);buffers.append(bytes(data))
    used80=(0x80000000,0xffffffff,0xc0000000)[variation];used_e0=(0xfffe0000,0xffff0000,0xffffff00)[variation]
    used_c0=(0,0x80000000,0xff000000)[variation]
    for address,value in ((0x80098b20,0x80000000),(0x80098838,used80),(0x8009883c,0),(0x800989cc,used_e0),(0x800987ec,used_c0)):c.put(address,4,value)
    camera=authored(seed,188);controls=bytearray(authored(seed,21));controls[0]=variation
    fill(c,0x800bc738,camera);fill(c,0x80122424,controls)
    input_word=(0,2,65535)[variation];c.put(0x1f800002,2,input_word);c.put(0x800a2d0b,1,(seed+3)&255)
    # Authored selected table slot, not a copied retail function table.
    c.put(0x8011b2f4,4,callback);c.run(0x8010102c)
    assert not c.return_only_hooks
    assert any(x['target']==hex(callback) for x in c.indirect)
    assert read(c,WORK,168)==bytes(work),'Unexpected workstore'
    changes=[diff(data,read(c,base,len(data))) for data,(base,*_) in zip(buffers,POOLS)]
    rows.append([seed,callback,frame,variation,input_word,used80,used_e0,used_c0,*changes,diff(camera,read(c,0x800bc738,188)),
                 read(c,0x80122424,21).hex(),c.get(0x800a2d0b,1),c.get(0x80098838,4),c.get(0x8009883c,4),c.get(0x800989cc,4),c.get(0x800987ec,4)])
    instructions+=c.steps;seed+=1
 fixture=('seed callback frame control input used80 usedE0 usedC0 pool40 pool80 poolE0 poolC0 camera controls global2D0B out80low out80high outE0 outC0\n'+
          ''.join(' '.join(map(str,row))+'\n' for row in rows)).encode()
 proof={'schema':'source-event-callbacks/1','status':'SOURCE_BACKED_PARTIAL','owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,
        'cases':len(rows),'unchanged_original_instructions':instructions,'instruction_patches':0,'helper_hooks':0,
        'fixture_sha256':hashlib.sha256(fixture).hexdigest(),'callback_count':len(CASES),'retail_parity':False,'hardware_tested':False,
        'limits':['Authored selected timeline slots are differential tests, not live scene chronology.',
                  'New effect allocations do not prove their resource, update, audio or rendering owners.',
                  'SharedRNGcallback80100D6C is not executed; no invented random stream.',
                  'Shared scratch counter1F800002 remains a separately required live owner when overlaycontrol80122424 becomes1.']}
 a.out.mkdir(parents=True,exist_ok=False);(a.out/'source-event-callbacks.tsv').write_bytes(fixture)
 (a.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

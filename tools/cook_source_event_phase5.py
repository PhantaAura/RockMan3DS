#!/usr/bin/env python3
"""Owner-local neutral phase5 records; no original instructions/geometry copied.

Only exact supported owner identities are admitted. Output is exclusively new
and outside source; the cooked file is not part of any source checkpoint.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

SOURCE=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(SOURCE/'tools')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from build_stage_catalog import iter_resources

OVERLAY_SHA='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
EBD_SHA='ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15'
def sha(data):return hashlib.sha256(data).hexdigest()
def require(test,reason):
    if not test:raise ValueError(reason)

def cook(image,stage,area):
    owner=OwnerExe(image)
    overlay=next(r['payload'] for r in iter_resources(stage)
                 if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    ebd=next(r['payload'] for r in iter_resources(area) if r['load']==0x8016c000)
    require(sha(overlay)==OVERLAY_SHA,'Unsupported phase5 overlay')
    require(sha(ebd)==EBD_SHA,'Unsupported phase5 area EBD')
    count=struct.unpack_from('<I',ebd)[0]
    require(count==14 and 4+count*16<=len(ebd),'Phase5 resource directory coverage')
    payload=bytearray(struct.pack('<III',0x8011af4c,0x8011b2e8,0))
    bindings=[]
    for i in range(16):
        raw=overlay[0x1ae0c+i*20:0x1ae0c+(i+1)*20]
        require(len(raw)==20 and raw[0]==i and raw[3]==(0x40 if i==0 else 0xe0),'Unsupported phase5 spawn domain')
        token,index,variant=0,0xffffffff,0
        if i:
            variant=overlay[0x1b377+i]
            # Exact8002FA38 first-match directory order for familyE0/subtype.
            matches=[j for j in range(count) if ebd[4+j*16:7+j*16]==bytes((0xe0,raw[1],variant))]
            require(matches,'Phase5 original resource lookup miss is not admitted')
            index=matches[0];token=struct.unpack_from('<I',ebd,8+index*16)[0]
            require(0x8016c000<=token<0x8016c000+len(ebd),'Phase5 resource identity outside EBD')
            bindings.append({'spawn_index':i,'resource_directory_index':index,
                             'resource_offset':token-0x8016c000,'variant':variant})
        payload+=raw+struct.pack('<IIB3x',token,index,variant)
    header=struct.pack('<4sHH6I',b'L3P5',1,96,1,16,96+len(payload),zlib.crc32(payload)&0xffffffff,32,12)
    package=header+bytes.fromhex(OVERLAY_SHA)+bytes.fromhex(EBD_SHA)+payload
    require(len(package)==620,'Noncanonical phase5 package')
    proof={'schema':'mml-source-event-phase5-cook/1','status':'SOURCE_BACKED_PARTIAL',
           'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,'ebd_sha256':EBD_SHA,
           'ebd_payload_bytes':len(ebd),'runtime_owner_admission':'ACTUAL_RETAINED_SHA256_AND_FIRST_MATCH_SOURCE_DIRECTORY_JOIN',
           'records':16,'selected_resource_bindings':15,'spawn_families':{'40':1,'E0':15},
           'selected_resource_identities':bindings,'package_bytes':len(package),
           'package_sha256':sha(package),'original_records_embedded_in_source':False,
           'original_code_exported':False,'retail_parity':False,'hardware_tested':False,
           'limits':['Constructed script records and resource identity do not prove geometry draw or callback execution.',
                     'Original virtual addresses are identity tokens only; native code never dereferences them.',
                     'Script callback80100738 and command interpreter progression remain unbound.']}
    # A source block hash is evidence, not exported original bytes.
    proof['constructor_sha256']=sha(overlay[0xefc:0x102c])
    proof['pool_reset_prefixes_sha256']={hex(lo):sha(owner.read(lo,hi-lo)) for lo,hi in
        ((0x8002e88c,0x8002e8d8),(0x8002e924,0x8002e980),(0x8002e9cc,0x8002ea18))}
    return package,proof

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','st00','area','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();output=args.out.resolve()
    require(not output.exists() and SOURCE not in output.parents and output!=SOURCE,'Choose a NEW directory outside source')
    package,proof=cook(args.exe.read_bytes(),args.st00,args.area)
    output.mkdir(parents=True,exist_ok=False)
    (output/'source_event_phase5.l3p5').write_bytes(package)
    (output/'cook-evidence.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
if __name__=='__main__':main()

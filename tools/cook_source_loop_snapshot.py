#!/usr/bin/env python3
"""Cook one PRIVATE, optional recorded OpenBIOS scene-entry L3LS snapshot.

Only the frozen read-only01432 observation/profile and exact supplied owner
identities are supported. No owner instructions execute and no default/native
frame seed is created. The resulting checkpoint requires witnessed constructor
admission and remains excluded from source releases.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

SOURCE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(SOURCE/'tools'))
from build_stage_catalog import iter_resources

EXE_SHA='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OVERLAY_SHA='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
EBD_SHA='ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15'
BIOS_SHA='713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
OBSERVATION_SHA='1813a6a7eaf7d2f866f3735221a523b1d8f375e05f9d787705b2a94368342460'
PROFILE_SHA='6152f14890229e62e11f1a1f90556b064984c574bb4a6940240211e27c525b01'
PC=0x80100efc
PACKAGE_BYTES=176
SCOPE='OpenBIOS reference; no universal retail/native chronology claim'
ROW_FIELDS=set('kind pc ordinal loops vsync scratch0 scratch1 scratch_half2 word_80098888 main_phase main_subphase event_id event_phase pad_packet pad_held pad_pressed read_only retail_parity'.split())

def require(condition,reason):
    if not condition:raise ValueError(reason)
def sha(data):return hashlib.sha256(data).hexdigest()
def bounded(row,name,maximum):
    value=row.get(name)
    require(type(value) is int and 0<=value<=maximum,'Invalid bounded observed scalar: '+name)
    return value
def unique_object(pairs):
    result={}
    for key,value in pairs:
        require(key not in result,'Duplicate JSON field: '+key);result[key]=value
    return result
def decode_json(blob):
    require(isinstance(blob,bytes) and 0<len(blob)<=1024*1024,'Empty/oversized observation JSON')
    def nonfinite(_):raise ValueError('Nonfinite JSON number')
    value=json.loads(blob.decode('utf-8'),object_pairs_hook=unique_object,parse_constant=nonfinite)
    require(isinstance(value,dict),'Observation JSON must be an object')
    return value
def validate_context(observation,profile):
    require(observation.get('status')=='MATCHES_MEASURED_FIELDS' and observation.get('scope')==SCOPE,
            'Unaccepted observation/scope')
    require(observation.get('read_only') is True and observation.get('memory_cards_inserted') is False and
            observation.get('native_seed_admitted') is False,'Writable/parity/default-seed observation rejected')
    require(profile.get('read_only_observer') is True and profile.get('memory_cards_inserted') is False and
            profile.get('retail_parity') is False,'Unsupported reference profile flags')
    require(profile.get('exe_sha256')==EXE_SHA and profile.get('bios_sha256')==BIOS_SHA,
            'Reference owner identity mismatch')
    for key in ('trace_sha256',):
        value=observation.get(key)
        require(type(value) is str and len(value)==64 and all(c in '0123456789abcdef' for c in value) and
                value!='0'*64,'Missing observation digest: '+key)
    for key in ('observer_sha256','profile_sha256'):
        value=profile.get(key)
        require(type(value) is str and len(value)==64 and all(c in '0123456789abcdef' for c in value) and
                value!='0'*64,'Missing reference digest: '+key)
    records=observation.get('scene_constructor')
    require(isinstance(records,list) and len(records)==1 and isinstance(records[0],dict),
            'Exactly one measured scene constructor is required')
    row=records[0]
    require(set(row)==ROW_FIELDS,'Unsupported/missing scene observation fields')
    require(row.get('kind')=='scene_constructor' and row.get('read_only') is True and
            row.get('retail_parity') is False,'Unsupported scene observation flags')
    for name in ('pc','ordinal','loops','vsync','word_80098888'):bounded(row,name,0xffffffff)
    for name in ('scratch0','scratch1','main_phase','main_subphase','event_id','event_phase'):bounded(row,name,255)
    for name in ('scratch_half2','pad_held','pad_pressed'):bounded(row,name,65535)
    require(row['pc']==PC and row['ordinal']==1 and row['loops']>0 and
            (row['loops']-1)&65535==row['scratch_half2'],'Wrong constructor or inconsistent loop/halfword')
    require((row['scratch0'],row['scratch1'],row['event_id'],row['event_phase'],row['main_phase'],row['main_subphase'])==
            (0,2,1,5,3,2),'Unsupported recorded constructor phase/cadence')
    packet=row.get('pad_packet')
    require(isinstance(packet,list) and len(packet)==8 and all(type(v) is int and 0<=v<=255 for v in packet),
            'Malformed bounded observed pad packet')
    return row
def validate_package(package):
    require(isinstance(package,bytes) and len(package)==PACKAGE_BYTES,'Wrong L3LS size')
    require(struct.unpack_from('<4sHHI',package)==(b'L3LS',1,PACKAGE_BYTES,1),'Wrong L3LS header/scope')
    require(struct.unpack_from('<I',package,12)[0]==zlib.crc32(package[16:])&0xffffffff,'Wrong L3LS CRC32')
    pc,loops,half,cadence,event,phase,main,sub,scratch0=struct.unpack_from('<IIH6B',package,16)
    require(pc==PC and loops>0 and (loops-1)&65535==half,'Wrong L3LS loop boundary')
    require((cadence,event,phase,main,sub,scratch0)==(2,1,5,3,2,0),'Wrong L3LS constructor phase')
    require(package[32:160]==b''.join(bytes.fromhex(v) for v in (EXE_SHA,OVERLAY_SHA,EBD_SHA,BIOS_SHA)),
            'Wrong L3LS owner identity')
    require(package[160:]==bytes(16),'Nonzero L3LS reserved fields')
def package_from_row(row):
    payload=struct.pack('<IIH6B',row['pc'],row['loops'],row['scratch_half2'],row['scratch1'],
                        row['event_id'],row['event_phase'],row['main_phase'],row['main_subphase'],row['scratch0'])
    payload+=b''.join(bytes.fromhex(v) for v in (EXE_SHA,OVERLAY_SHA,EBD_SHA,BIOS_SHA))+bytes(16)
    package=struct.pack('<4sHHII',b'L3LS',1,PACKAGE_BYTES,1,zlib.crc32(payload)&0xffffffff)+payload
    validate_package(package)
    return package
def validate_resource_owners(program_resources,area_resources):
    overlays=[r for r in program_resources if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper()]
    # The area EBD is an independent owner, not the broader ST00 program file.
    ebds=[r for r in area_resources if r['load']==0x8016c000]
    require(len(overlays)==1 and overlays[0].get('type',0)==0 and sha(overlays[0]['payload'])==OVERLAY_SHA,
            'Unsupported supplied ST00 overlay owner')
    require(len(ebds)==1 and sha(ebds[0]['payload'])==EBD_SHA,'Unsupported supplied ST00 area-zero EBD owner')
def cook(observation_blob,profile_blob,image,bios,st00,area):
    require(sha(observation_blob)==OBSERVATION_SHA,'Unsupported frozen01432 observation identity')
    require(sha(profile_blob)==PROFILE_SHA,'Unsupported frozen01432 reference profile identity')
    observation,profile=decode_json(observation_blob),decode_json(profile_blob)
    row=validate_context(observation,profile)
    require(sha(image)==EXE_SHA and sha(bios)==BIOS_SHA,'Supplied executable/BIOS identity mismatch')
    validate_resource_owners(list(iter_resources(Path(st00))),list(iter_resources(Path(area))))
    package=package_from_row(row)
    provenance={'schema':'mml-recorded-source-loop-snapshot-cook/1','status':'MATCHES_MEASURED_FIELDS',
        'scope':'One recorded OpenBIOS constructor boundary; optional native replay only',
        'observation_sha256':sha(observation_blob),'reference_profile_file_sha256':sha(profile_blob),
        'trace_sha256':observation['trace_sha256'],'reference_observer_sha256':profile['observer_sha256'],
        'reference_portable_profile_sha256':profile['profile_sha256'],
        'owner_exe_sha256':EXE_SHA,'overlay_sha256':OVERLAY_SHA,'ebd_sha256':EBD_SHA,'bios_sha256':BIOS_SHA,
        'checkpoint':{key:row[key] for key in ('pc','loops','scratch_half2','scratch0','scratch1','event_id','event_phase','main_phase','main_subphase')},
        'package_bytes':len(package),'package_sha256':sha(package),'payload_crc32':zlib.crc32(package[16:])&0xffffffff,
        'read_only_observation':True,'memory_cards_inserted':False,'default_enabled':False,
        'witnessed_native_constructor_admission_required':True,'source_payload_embedded':False,
        'universal_retail_or_native_seed':False,'native_chronology_proved':False,'retail_parity':False,'hardware_tested':False,
        'reference_run_completed':False,'reference_session_end':observation.get('reference_session_end'),
        'limits':['This is one measured constructor checkpoint in the selected installed OpenBIOS reference.',
                  'No executable or BIOS instructions execute during cooking; owner files are read only.',
                  'The incomplete reference run does not establish scene completion or universal startup state.',
                  'Native admission requires the successful original phase5 constructor receipt and matching live residency/generation.',
                  'Replay main/subphase/cadence owners retain recorded scope; host frame counts never initialize the counter.',
                  'The private checkpoint/provenance are excluded from source releases.']}
    return package,provenance
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('observation','profile','exe','st00','area','bios','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();output=args.out.resolve()
    require(not output.exists() and output!=SOURCE and SOURCE not in output.parents,'Choose a NEW private directory outside source')
    package,provenance=cook(args.observation.read_bytes(),args.profile.read_bytes(),args.exe.read_bytes(),args.bios.read_bytes(),args.st00,args.area)
    output.mkdir(parents=True,exist_ok=False)
    (output/'source_loop_snapshot.l3ls').write_bytes(package)
    (output/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    print(json.dumps({key:provenance[key] for key in ('status','scope','package_bytes','package_sha256','checkpoint','default_enabled','retail_parity')}))
    return 0
if __name__=='__main__':raise SystemExit(main())

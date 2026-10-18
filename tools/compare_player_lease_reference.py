#!/usr/bin/env python3
"""Normalize paired original Redux80035508 observations for the native checker.
Read-only reference input. Writes only a new external evidence directory. An
observer guard, pending call, unpaired return or absent data is UNKNOWN, never
a successful retail parity result. No state/control writes or game payload.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

SOURCE=Path(__file__).resolve().parents[1]
OWNER_SHA='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'

def u32(value):
    if not isinstance(value,int) or isinstance(value,bool) or not 0<=value<=0xffffffff:
        raise ValueError('Invalid captured unsigned32 scalar')
    return value

def words(value,count):
    if not isinstance(value,list) or len(value)!=count:raise ValueError('Invalid captured register/owner array')
    return list(map(u32,value))

def selected(row,mask):
    records=row.get('selected_owners')
    if not isinstance(records,list):raise ValueError('Missing captured owners')
    by_slot={}
    for record in records:
        slot=u32(record['slot'])
        if slot>=24 or slot in by_slot or not (mask&(0x80000000>>slot)):raise ValueError('Invalid selected owner slot')
        u32(record['pointer']);u32(record['physical']);words(record['words_bc_c0'],2)
        by_slot[slot]=record
    if len(by_slot)!=sum(bool(mask&(0x80000000>>slot)) for slot in range(24)):
        raise ValueError('Missing selected owner premise')
    return by_slot

def normalize(before,after):
    ordinal=u32(before['ordinal'])
    if u32(after['ordinal'])!=ordinal or u32(after['caller_ra'])!=u32(before['caller_ra']):raise ValueError('Unpaired lease call')
    if before.get('source_identity_matches') is not True:raise ValueError('Source identity was not admitted')
    mask=u32(before['mask_80098798'])
    if u32(after['mask_80098798'])!=mask:raise ValueError('Unexpected lease mask mutation')
    pointers=words(before['owner_pointers_800be478'],24)
    if words(after['owner_pointers_800be478'],24)!=pointers:raise ValueError('Unexpected owner table mutation')
    before_owners,after_owners=selected(before,mask),selected(after,mask)
    physical=[];owner_before=[];owner_after=[];aliases={}
    for slot in range(24):
        b,a=before_owners.get(slot),after_owners.get(slot)
        if b:
            if b['pointer']!=pointers[slot] or a['pointer']!=b['pointer'] or a['physical']!=b['physical']:
                raise ValueError('Owner pointer identity changed')
            pointer=u32(b['pointer']);address=u32(b['physical'])
            if pointer%4 or pointer//0x20000000 not in (0,4,5) or pointer%0x20000000!=address or address+0xc4>0x200000:
                raise ValueError('Owner pointer is outside admitted original RAM')
            alias=aliases.get(address)
            if alias is not None and (b['words_bc_c0']!=alias[1] or a['words_bc_c0']!=alias[2]):
                raise ValueError('Aliased owner snapshot is inconsistent')
            expected_first=slot if alias is None else alias[0]
            if u32(b['alias_first_slot'])!=expected_first or u32(a['alias_first_slot'])!=expected_first:
                raise ValueError('Aliased owner provenance is inconsistent')
            aliases[address]=(expected_first,b['words_bc_c0'],a['words_bc_c0'])
            physical.append(u32(b['physical']));owner_before+=words(b['words_bc_c0'],2);owner_after+=words(a['words_bc_c0'],2)
        else:physical.append(0);owner_before += [0,0];owner_after += [0,0]
    return [ordinal]+words(before['scratch_camera_1f800008'],8)+[mask]+words(before['cp2c'],32)+words(before['cp2d'],32)+physical+owner_before+words(after['cp2c'],32)+words(after['cp2d'],32)+owner_after

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',required=True,type=Path);parser.add_argument('--native-checker',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):raise ValueError('Choose new external evidence output')
    raw=args.reference.read_bytes();pending={};pairs=[];problems=[];normal_end=False;starts=0
    for line_number,line in enumerate(raw.splitlines(),1):
        if not line.strip():continue
        try:
            row=json.loads(line);kind=row.get('kind',row.get('type',row.get('event')))
            if kind=='lease_observer_start':
                starts+=1
                if starts!=1 or row.get('source_exe_sha256')!=OWNER_SHA or row.get('source_function')!=0x80035508:
                    raise ValueError('Unsupported or duplicate original observer provenance')
            elif kind=='player_lease_enter':
                if starts!=1:raise ValueError('Lease entry lacks admitted observer provenance')
                ordinal=u32(row['ordinal'])
                if ordinal in pending:raise ValueError('Duplicate pending lease ordinal')
                pending[ordinal]=row
            elif kind=='player_lease_return':
                ordinal=u32(row['ordinal'])
                if ordinal not in pending:raise ValueError('Lease return without entry')
                pairs.append(normalize(pending.pop(ordinal),row))
            elif kind=='lease_observer_guard_failure':problems.append({'line':line_number,'reason':row.get('reason','observer_guard')})
            elif kind=='lease_observer_budget_complete':
                if row.get('pending_return',row.get('pending')) is not False:raise ValueError('Observer stopped with pending call')
                normal_end=True
        except (ValueError,KeyError,TypeError) as error:problems.append({'line':line_number,'reason':str(error)})
    if pending:problems.append({'reason':'Unreturned source lease call','ordinals':list(pending)})
    if starts!=1:problems.append({'reason':'Missing admitted original observer provenance'})
    if not pairs:problems.append({'reason':'No completed admitted source lease call'})
    result={'schema':'mml-original-player-lease-comparison/1','status':'UNKNOWN','reference_sha256':hashlib.sha256(raw).hexdigest(),
            'completed_pairs':len(pairs),'whole_trace_accepted':not problems,'normal_budget_complete':normal_end,
            'problems':problems,'retail_parity':False,'source_function':'0x80035508','checker_invoked':False}
    args.out.mkdir(parents=True)
    if not problems:
        payload=''.join(' '.join(map(str,row))+'\n' for row in pairs).encode()
        fixture=args.out/'original-player-lease.tsv';fixture.write_bytes(payload)
        command=[str(args.native_checker),'--reference',str(fixture)]
        try:
            native=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,check=False)
        except OSError as error:
            result['checker_error']=str(error)
            result['problems'].append({'reason':'Native reference checker could not execute'})
            (args.out/'comparison-proof.json').write_text(json.dumps(result,indent=2)+'\n')
            print(json.dumps(result,indent=2))
            return 1
        (args.out/'native-comparison.jsonl').write_text(native.stdout)
        (args.out/'native-comparison.stderr.log').write_text(native.stderr)
        result.update({'checker_invoked':True,'checker_command':command,'checker_returncode':native.returncode,
                       'normalized_fixture_sha256':hashlib.sha256(payload).hexdigest()})
        summaries=[json.loads(line) for line in native.stdout.splitlines() if line.startswith('{')]
        summary=next((row for row in reversed(summaries) if row.get('schema')=='mml-source-player-lease-native-comparison/1'),None)
        result['native_result']=summary
        if native.returncode==0 and summary and summary.get('cases')==len(pairs) and summary.get('matched')==len(pairs):
            result['status']='MATCHES_MEASURED_FIELDS'
    (args.out/'comparison-proof.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return 0 if result['status']=='MATCHES_MEASURED_FIELDS' else 1

if __name__=='__main__':raise SystemExit(main())

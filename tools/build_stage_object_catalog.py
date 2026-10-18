#!/usr/bin/env python3
"""Evidence-gated cooker for retail 20-byte stage object-list pointer tables.

Only strong pointer runs are emitted: at least two in-overlay pointers in consecutive
u32 table slots must resolve to 20-byte object records terminated on-record by the
retail 0xFF/00/00/00 sentinel form. Multiple candidate runs are retained rather than
assigning unproved gameplay roles.
"""
from __future__ import annotations
import argparse, json, shutil, struct, sys, zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from build_stage_catalog import iter_resources

REC=20

def parse_list(data:bytes,base:int,ptr:int):
    if ptr<base or ptr>=base+len(data) or ((ptr-base)&3): return None
    off=ptr-base; records=[]
    for i in range(256):
        p=off+i*REC
        if p+REC>len(data): return None
        b=data[p:p+REC]
        if b[0]==0xff:
            if any(b[1:4]): return None
            return records
        # Strong-evidence filter learned from the exact USA common object lists.
        # Do not attach semantics to the byte; this merely excludes random code/data.
        if b[0]>0x3f: return None
        records.append(b)
    return None

def find_tables(data:bytes,base:int):
    valid={}
    for o in range(0,len(data)-3,4):
        ptr=struct.unpack_from('<I',data,o)[0]
        recs=parse_list(data,base,ptr)
        if recs is not None: valid[o]=(ptr,recs)
    keys=sorted(valid); runs=[];i=0
    while i<len(keys):
        seq=[keys[i]];i+=1
        while i<len(keys) and keys[i]==seq[-1]+4:seq.append(keys[i]);i+=1
        if len(seq)>=2 and sum(bool(valid[o][1]) for o in seq)>=2:
            runs.append((seq[0],[valid[o] for o in seq]))
    return runs

def cook(base:int,tables):
    flat_lists=[];objects=[];table_meta=[]
    for table_off,lists in tables:
        first=len(flat_lists)
        for ptr,recs in lists:
            of=len(objects);objects.extend(recs);flat_lists.append((ptr-base,of,len(recs)))
        table_meta.append((table_off,first,len(lists),1))
    header=48;off_tables=header;off_lists=off_tables+len(table_meta)*16;off_objects=off_lists+len(flat_lists)*16
    payload=bytearray()
    for x in table_meta: payload.extend(struct.pack('<IIII',*x))
    for x in flat_lists: payload.extend(struct.pack('<IIII',x[0],x[1],x[2],0))
    for r in objects: payload.extend(r)
    file_bytes=header+len(payload);crc=zlib.crc32(payload)&0xffffffff
    head=b'L3OT'+struct.pack('<HHIIIIIIIIII',1,header,0,len(table_meta),len(flat_lists),len(objects),base,off_tables,off_lists,off_objects,file_bytes,crc)
    assert len(head)==header
    return head+payload

def main():
    ap=argparse.ArgumentParser();ap.add_argument('dat_dir',type=Path);ap.add_argument('out_dir',type=Path);a=ap.parse_args()
    dat=a.dat_dir.resolve();out=a.out_dir.resolve()
    if out.exists():shutil.rmtree(out)
    out.mkdir(parents=True)
    manifest={'format':1,'generated':True,'source':'user-owned Mega Man Legends USA stage overlays','packages':[]}
    total_tables=total_lists=total_objects=0
    for cp in sorted(dat.glob('ST*.BIN')):
        overlays=[r for r in iter_resources(cp) if r['type']==0 and r['internal'].upper().endswith('.BIN') and 'PROGBIN' in r['internal'].upper() and r['load']==0x80100000]
        if not overlays:continue
        r=overlays[0];tables=find_tables(r['payload'],r['load'])
        entry={'package':cp.stem,'tables':len(tables),'lists':sum(len(x[1]) for x in tables),'objects':sum(len(recs) for _,ls in tables for _,recs in ls)}
        if tables:
            d=out/'objects'/cp.stem;d.mkdir(parents=True,exist_ok=True);p=d/'objects.l3ot';p.write_bytes(cook(r['load'],tables));entry['output']=str(p.relative_to(out))
        manifest['packages'].append(entry);total_tables+=entry['tables'];total_lists+=entry['lists'];total_objects+=entry['objects']
    (out/'object_catalog.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f"packages={len(manifest['packages'])} tables={total_tables} lists={total_lists} objects={total_objects} packages_with_tables={sum(p['tables']>0 for p in manifest['packages'])}")
if __name__=='__main__':main()

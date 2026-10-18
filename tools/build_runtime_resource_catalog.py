#!/usr/bin/env python3
"""Cook portable runtime forms of every EBD and MSG resource in a user-owned MML USA DAT set.

This deliberately emits only pointer-free generated L3 files and metadata. It never copies the
source resource payloads into the output or into source releases.
"""
from __future__ import annotations
import argparse, hashlib, json, re, shutil, subprocess, sys, struct, zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from build_stage_catalog import iter_resources

def sha256(p:Path):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1<<20),b''): h.update(b)
    return h.hexdigest()


def write_l3rc(resources, out_path:Path):
    strings=bytearray(); entries=[]
    for r in resources:
        name=r['internal'].encode('utf-8'); path=r['output'].encode('utf-8')
        no=len(strings); strings.extend(name); po=len(strings); strings.extend(path)
        pkg=r['package'].encode('ascii','strict')[:8].ljust(8,b'\0')
        kind=1 if r['kind']=='ebd' else 2
        entries.append((pkg,kind,int(r['load']),int(r['bytes']),int(r['crc32']),no,len(name),po,len(path)))
    header=40; entry_bytes=40; off_entries=header; off_strings=off_entries+len(entries)*entry_bytes
    payload=bytearray()
    for pkg,kind,load,nbytes,crc,no,nl,po,pl in entries:
        payload.extend(pkg); payload.extend(bytes((kind,0,0,0))); payload.extend(struct.pack('<IIIIIII',load,nbytes,crc,no,nl,po,pl))
    payload.extend(strings)
    file_bytes=header+len(payload); crc=zlib.crc32(payload)&0xffffffff
    head=b'L3RC'+struct.pack('<HHIIIIIIII',1,header,0,len(entries),entry_bytes,off_entries,off_strings,len(strings),file_bytes,crc)
    assert len(head)==header
    out_path.write_bytes(head+payload)

def safe_component(s:str):
    s=s.replace('..\\','').replace('\\','__').replace('/','__')
    return re.sub(r'[^A-Za-z0-9._-]+','_',s)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('dat_dir',type=Path);ap.add_argument('host_build_dir',type=Path);ap.add_argument('out_dir',type=Path);a=ap.parse_args()
    dat=a.dat_dir.resolve(); tools=a.host_build_dir.resolve(); out=a.out_dir.resolve()
    for t in ('l3cook_ebd','l3cook_msg'):
        if not (tools/t).is_file(): raise SystemExit(f'missing host tool: {tools/t}')
    if out.exists(): shutil.rmtree(out)
    out.mkdir(parents=True)
    manifest={'format':1,'generated':True,'source':'user-owned Mega Man Legends USA DAT directory','resources':[]}
    counts={'ebd':0,'msg':0}
    for cp in sorted(dat.glob('*.BIN')):
        for r in iter_resources(cp):
            upper=r['internal'].upper()
            kind='ebd' if upper.endswith('.EBD') else 'msg' if upper.endswith('.MSG') else None
            if not kind or r['type']!=0: continue
            package=cp.stem
            od=out/'resources'/package;od.mkdir(parents=True,exist_ok=True)
            dst=od/(safe_component(r['internal'])+('.l3ebd' if kind=='ebd' else '.l3msg'))
            # Source bytes live only in a temporary owner-side file and are removed immediately.
            tmp=out/'.cook_input.tmp';tmp.write_bytes(r['payload'])
            try:
                cmd=[str(tools/'l3cook_ebd'),str(tmp),hex(r['load']),str(dst)] if kind=='ebd' else [str(tools/'l3cook_msg'),str(tmp),str(dst)]
                p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
                if p.returncode!=0: raise RuntimeError(f"{cp.name}:{r['internal']}: {p.stderr.strip()}")
            finally:
                tmp.unlink(missing_ok=True)
            counts[kind]+=1
            manifest['resources'].append({'package':package,'kind':kind,'internal':r['internal'],'load':r['load'],'output':str(dst.relative_to(out)),'bytes':dst.stat().st_size,'crc32':zlib.crc32(dst.read_bytes())&0xffffffff,'sha256':sha256(dst)})
    (out/'resource_catalog.json').write_text(json.dumps(manifest,indent=2)+'\n')
    write_l3rc(manifest['resources'],out/'resource_catalog.l3rc')
    print(f"ebd={counts['ebd']} msg={counts['msg']} total={sum(counts.values())}")
    return 0
if __name__=='__main__': raise SystemExit(main())

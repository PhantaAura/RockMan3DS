#!/usr/bin/env python3
"""Index/cook every base stage quartet from a user-owned MML USA DAT extraction.

No game bytes are embedded in this script. It discovers type-0 MAP.IDX/MAP.MDT/STG/HED
resources from the user's files, feeds them through the pointer-free host cookers, and
writes only generated L3 runtime files plus metadata.
"""
from __future__ import annotations
import argparse, hashlib, json, re, shutil, struct, subprocess, tempfile, zlib
from pathlib import Path
SECTOR=2048

def iter_resources(container:Path):
    d=container.read_bytes()
    for off in range(0,max(0,len(d)-SECTOR+1),SECTOR):
        ps=off+0x40; z=d.find(b'\0',ps,min(ps+256,len(d)))
        if z<0: continue
        try: internal=d[ps:z].decode('ascii')
        except UnicodeDecodeError: continue
        if not internal.startswith('..\\'): continue
        typ,size=struct.unpack_from('<II',d,off); load=struct.unpack_from('<I',d,off+0x0C)[0]
        if off+SECTOR+size>len(d): continue
        yield {'container':container,'offset':off,'type':typ,'size':size,'load':load,'internal':internal,'payload':d[off+SECTOR:off+SECTOR+size]}

def sha(path:Path):
    h=hashlib.sha256();
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1<<20),b''): h.update(b)
    return h.hexdigest()

def stage_key(internal:str):
    m=re.search(r'\\(ST[0-9A-Fa-f]{2})_MAP\.(?:IDX|MDT)$',internal,re.I)
    if m: return m.group(1).upper()
    m=re.search(r'\\(ST[0-9A-Fa-f]{2})\.(?:STG|HED)$',internal,re.I)
    return m.group(1).upper() if m else None

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('dat_dir',type=Path); ap.add_argument('host_build_dir',type=Path); ap.add_argument('out_dir',type=Path); a=ap.parse_args()
    dat=a.dat_dir.resolve(); tools=a.host_build_dir.resolve(); out=a.out_dir.resolve()
    for tool in ('l3cook_st00','l3cook_render'):
        if not (tools/tool).is_file(): raise SystemExit(f'missing {tools/tool}')
    if out.exists(): shutil.rmtree(out)
    out.mkdir(parents=True)
    quartets={}
    for cp in sorted(dat.glob('*.BIN')):
        for r in iter_resources(cp):
            if r['type']!=0: continue
            key=stage_key(r['internal'])
            if not key: continue
            u=r['internal'].upper()
            slot='idx' if u.endswith('_MAP.IDX') else 'mdt' if u.endswith('_MAP.MDT') else 'stg' if u.endswith('.STG') else 'hed' if u.endswith('.HED') else None
            if slot: quartets.setdefault((cp.name,key),{})[slot]=r
    entries=[]
    with tempfile.TemporaryDirectory(prefix='l3stages-') as td0:
        td=Path(td0)
        for (container,key),q in sorted(quartets.items()):
            if set(q)!= {'idx','mdt','stg','hed'}: continue
            # Accept only the canonical shared load layout proved on the USA disc.
            loads={k:q[k]['load'] for k in q}
            expected={'idx':0x8015C000,'mdt':0x80164000,'stg':0x80194000,'hed':0x8013A000}
            if loads!=expected:
                entries.append({'container':container,'stage':key,'status':'unsupported_load_layout','loads':loads}); continue
            work=td/(Path(container).stem+'_'+key); work.mkdir()
            for k in ('idx','mdt','stg','hed'): (work/k).write_bytes(q[k]['payload'])
            package_id=Path(container).stem
            dst=out/'stages'/package_id; dst.mkdir(parents=True)
            try:
                subprocess.run([str(tools/'l3cook_st00'),str(work/'mdt'),str(work/'idx'),str(work/'stg'),str(work/'hed'),str(dst/'map.l3map')],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
            except subprocess.CalledProcessError as e:
                shutil.rmtree(dst,ignore_errors=True)
                entries.append({'container':container,'stage':key,'status':'map_cook_failed','error':(e.stderr or '').strip()[-800:]})
                continue
            base={'container':container,'stage':key,'map_bytes':(dst/'map.l3map').stat().st_size,'map_sha256':sha(dst/'map.l3map')}
            try:
                subprocess.run([str(tools/'l3cook_render'),str(work/'mdt'),str(work/'idx'),str(work/'stg'),str(work/'hed'),str(dst/'render.l3render')],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
                entries.append({**base,'status':'cooked','render_bytes':(dst/'render.l3render').stat().st_size,'render_sha256':sha(dst/'render.l3render')})
            except subprocess.CalledProcessError as e:
                # Map data is still exact/useful even when a render-stream variant is not decoded yet.
                # Preserve it and report the render gap explicitly instead of throwing away proved data.
                entries.append({**base,'status':'map_cooked_render_pending','error':(e.stderr or '').strip()[-800:]})
    manifest={'format':1,'source':'user-owned Mega Man Legends USA DAT directory','generated':True,'entries':entries}
    (out/'stage_catalog.json').write_text(json.dumps(manifest,indent=2)+'\n')
    # Compact pointer-free runtime catalog. It contains only metadata about the owner's
    # generated packages; no original game bytes or original filenames beyond four-byte stage ids.
    cooked_entries=[e for e in entries if e['status'] in ('cooked','map_cooked_render_pending')]
    blob=bytearray(b'L3SC')
    blob += struct.pack('<HHIIIIII',1,32,0,len(cooked_entries),32,32,32+32*len(cooked_entries),0)
    for e in cooked_entries:
        code=e['stage'].encode('ascii')
        if len(code)!=4: raise RuntimeError('stage code must be four ASCII bytes')
        package=Path(e['container']).stem.encode('ascii')
        if len(package)>8: raise RuntimeError('package id exceeds eight ASCII bytes')
        package=package+b'\0'*(8-len(package))
        package_dir=Path(e['container']).stem
        mp=(out/'stages'/package_dir/'map.l3map').read_bytes()
        rp=out/'stages'/package_dir/'render.l3render'
        rb=rp.read_bytes() if rp.is_file() else b''
        flags=1|(2 if rb else 0)
        blob += package + code + struct.pack('<IIIII',flags,len(mp),len(rb),zlib.crc32(mp)&0xffffffff,zlib.crc32(rb)&0xffffffff if rb else 0)
    struct.pack_into('<I',blob,28,zlib.crc32(blob[32:])&0xffffffff)
    (out/'stage_catalog.l3sc').write_bytes(blob)
    cooked=sum(e['status']=='cooked' for e in entries); pending=sum(e['status']=='map_cooked_render_pending' for e in entries); map_failed=sum(e['status']=='map_cook_failed' for e in entries); layouts=sum(e['status']=='unsupported_load_layout' for e in entries)
    maps_ok=cooked+pending
    print(f'base_stage_sets={len(entries)} maps_cooked={maps_ok} fully_cooked={cooked} render_pending={pending} map_failed={map_failed} unsupported_layout={layouts}')
    return 0 if map_failed==0 and layouts==0 else 1
if __name__=='__main__': raise SystemExit(main())

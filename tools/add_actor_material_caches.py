#!/usr/bin/env python3
"""Extend a verified owner-local pack in a NEW directory; no unchanged recook.

Reads the already supplied DAT files only for indexed upload composition. Never
extracts intermediate assets or embeds them in source. The complete output pack
is owner-local and must not be included in a source release.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
from build_hardware_test import validate_pack,sha256

ROOT=Path(__file__).resolve().parents[1]
UPLOAD_PREFIX=("INIT_DAT.BIN","HEAD00.BIN","ARM00R.BIN","ARM00L.BIN","LEG00.BIN","ST00.BIN")

def extend_pack(pack:Path,dat:Path,tool:Path,out:Path):
    pack,dat,tool,out=(p.resolve() for p in (pack,dat,tool,out))
    if out.exists():raise ValueError("output already exists; choose a new checkpoint directory")
    for protected in (pack,dat,ROOT,tool.parent):
        if out==protected or protected in out.parents or out in protected.parents:
            raise ValueError("output must be separate from source, pack, owner data and tools")
    verified=validate_pack(pack)
    if not tool.is_file():raise ValueError("actor material cooker missing")
    uploads=[dat/name for name in UPLOAD_PREFIX]
    if any(not path.is_file() for path in uploads+[dat/f'ST00_{a:02d}.BIN' for a in range(5)]):
        raise ValueError("owner tutorial indexed-resource containers missing")
    manifest=json.loads((pack/'manifest.json').read_text())
    if manifest.get('actor_materials') is not None:raise ValueError("input already has actor materials; choose an unextended baseline")
    shutil.copytree(pack,out)
    try:
        commands=[]
        for area in range(5):
            name=f'materials/actor_area_{area:02d}.l3mc';output=out/name
            if output.exists():raise ValueError("input contains an undeclared actor cache")
            command=[str(tool),str(out/f'ebd/area_{area:02d}.l3ebd'),str(output),
                     *(str(p) for p in uploads),str(dat/f'ST00_{area:02d}.BIN')]
            result=subprocess.run(command,capture_output=True,text=True,check=True)
            commands.append({'argv':command,'returncode':result.returncode,'output':result.stdout})
            manifest['outputs'].append({'path':'legends3ds/tutorial/'+name,
                'bytes':output.stat().st_size,'sha256':sha256(output)})
        manifest['actor_materials']={'format':1,'optional':True,'runtime_gate':'ACTOR_TEXTURES=1',
            'default_enabled':False,'shade':'UNKNOWN','status':'SOURCE_BACKED_PARTIAL',
            'upload_order':'inherited ordered VRAM resources; live original VRAM comparison pending',
            'previous_manifest_sha256':verified['manifest_sha256']}
        (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        current=validate_pack(out)
        if validate_pack(pack)!=verified:raise ValueError("original pack changed during extension")
        return {'status':'PASS','input_unchanged':True,'unchanged_files':verified['manifest_outputs'],
            'new_optional_files':5,'redistributable':False,'pack':str(out),
            'validation':current,'commands':commands}
    except BaseException:
        # Only the new directory created above is removed; previous pack untouched.
        shutil.rmtree(out)
        raise

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack',required=True,type=Path);p.add_argument('--dat',required=True,type=Path)
    p.add_argument('--host-build',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
    p.add_argument('--result',type=Path)
    a=p.parse_args();result=extend_pack(a.pack,a.dat,a.host_build/'l3cook_actor_material_cache',a.out)
    if a.result:a.result.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'status':result['status'],'pack':result['pack'],'new_optional_files':5,'input_unchanged':True}))
if __name__=='__main__':main()

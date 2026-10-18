#!/usr/bin/env python3
"""Add one optional initial-batch E0 material snapshot to a NEW owner-local pack.

All inherited outputs are copied byte-identically. Current-frame VRAM/palette
chronology remains unknown; this snapshot is a diagnostic, default OFF. Never
include this pack or its payload in the distributable source checkpoint.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
from build_hardware_test import validate_pack,sha256

ROOT=Path(__file__).resolve().parents[1]
UPLOADS=("INIT_DAT.BIN","HEAD01.BIN","ARM00R.BIN","ARM00L.BIN","LEG00.BIN","ST00.BIN","ST00_00.BIN")

def extend_pack(pack:Path,dat:Path,tool:Path,out:Path):
    pack,dat,tool,out=(p.resolve() for p in (pack,dat,tool,out))
    if out.exists():raise ValueError("choose a new output directory; existing checkpoints are immutable")
    for protected in (pack,dat,ROOT,tool.parent):
        if out==protected or protected in out.parents or out in protected.parents:
            raise ValueError("output must be separate from source, input pack, owner data and build tools")
    verified=validate_pack(pack)
    if not tool.is_file():raise ValueError("scene material cooker missing")
    uploads=[dat/name for name in UPLOADS]
    if any(not p.is_file() for p in uploads):raise ValueError("supported initial upload containers missing")
    manifest=json.loads((pack/'manifest.json').read_text())
    if manifest.get('scene_materials') is not None:raise ValueError("snapshot already present; retain original pack")
    if not (pack/'source_event_phase5.l3p5').is_file():raise ValueError("source scene profile missing")
    shutil.copytree(pack,out)
    try:
        name='materials/scene_e0_initial.l3sm';output=out/name
        output.parent.mkdir(exist_ok=True)
        if output.exists():raise ValueError("undeclared scene material snapshot")
        command=[str(tool),str(out/'source_event_phase5.l3p5'),str(out/'ebd/area_00.l3ebd'),str(output),*(str(p) for p in uploads)]
        run=subprocess.run(command,capture_output=True,text=True,check=True)
        manifest['outputs'].append({'path':'legends3ds/tutorial/'+name,'bytes':output.stat().st_size,'sha256':sha256(output)})
        manifest['scene_materials']={'format':1,'optional':True,'runtime_gate':'SOURCE_SCENE_TEXTURES=1',
            'default_enabled':False,'snapshot':'INITIAL_UPLOAD_BATCH','chronology':'SOURCE_BACKED_PARTIAL',
            'live_palette':'UNKNOWN','path':'legends3ds/tutorial/'+name,
            'uploads':[{'name':p.name,'sha256':sha256(p)} for p in uploads],
            'previous_manifest_sha256':verified['manifest_sha256']}
        (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        current=validate_pack(out)
        if validate_pack(pack)!=verified:raise ValueError("input pack changed during extension")
        return {'status':'PASS','input_unchanged':True,'unchanged_files':verified['manifest_outputs'],
            'new_optional_files':1,'redistributable':False,'pack':str(out),'validation':current,
            'commands':[{'argv':command,'returncode':run.returncode,'output':run.stdout}]}
    except BaseException:
        shutil.rmtree(out) # Only this new directory; never an input/checkpoint.
        raise

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('pack','dat','host-build','out'):p.add_argument('--'+name,required=True,type=Path)
    p.add_argument('--result',type=Path);a=p.parse_args()
    result=extend_pack(a.pack,a.dat,a.host_build/'l3cook_scene_e0_material',a.out)
    if a.result:a.result.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('status','pack','input_unchanged','new_optional_files')}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Build the broad pointer-free Legends3DS runtime-data foundation from a user-owned USA extraction.

Current sweep coverage is deliberately structural rather than a claim that every gameplay handler is
ported: all discovered base-stage maps/render streams plus every EBD directory and MSG bank are cooked.
No original Capcom resource payload is copied to the generated source tree by this script.
"""
from __future__ import annotations
import argparse, json, shutil, subprocess, sys
from pathlib import Path

def run(cmd):
    p=subprocess.run([str(x) for x in cmd],check=True,text=True,capture_output=True)
    if p.stdout.strip(): print(p.stdout.strip())

def main():
    ap=argparse.ArgumentParser();ap.add_argument('extracted_root',type=Path);ap.add_argument('host_build_dir',type=Path);ap.add_argument('out_dir',type=Path);a=ap.parse_args()
    root=a.extracted_root.resolve(); dat=root/'CDDATA'/'DAT'; tools=a.host_build_dir.resolve(); out=a.out_dir.resolve(); here=Path(__file__).resolve().parent
    if not (root/'ROCK_NEO.EXE').is_file() or not dat.is_dir(): raise SystemExit('expected extracted/ROCK_NEO.EXE and extracted/CDDATA/DAT')
    if out.exists(): shutil.rmtree(out)
    out.mkdir(parents=True)
    run([sys.executable,here/'verify_mml_us_reference.py',root])
    run([sys.executable,here/'build_stage_catalog.py',dat,tools,out/'world'])
    run([sys.executable,here/'build_runtime_resource_catalog.py',dat,tools,out/'script'])
    run([sys.executable,here/'build_stage_object_catalog.py',dat,out/'flow'])
    run([sys.executable,here/'build_audio_reference_catalog.py',root,out/'audio'])
    run([sys.executable,here/'cook_reference_tables.py',root/'ROCK_NEO.EXE',out/'reference.l3ref'])
    stage=json.load(open(out/'world'/'stage_catalog.json'))
    res=json.load(open(out/'script'/'resource_catalog.json'))
    obj=json.load(open(out/'flow'/'object_catalog.json'))
    audio=json.load(open(out/'audio'/'audio_reference_catalog.json'))
    summary={
      'format':1,'generated':True,'source':'user-owned Mega Man Legends USA extraction',
      'stage_packages':sum(e['status']=='cooked' for e in stage['entries']),
      'ebd_resources':sum(r['kind']=='ebd' for r in res['resources']),
      'message_resources':sum(r['kind']=='msg' for r in res['resources']),
      'object_table_packages':sum(p['tables']>0 for p in obj['packages']),
      'object_tables':sum(p['tables'] for p in obj['packages']),
      'object_lists':sum(p['lists'] for p in obj['packages']),
      'object_records':sum(p['objects'] for p in obj['packages']),
      'xa_sources':sum(e['kind']=='xa' for e in audio['entries']),
      'str_sources':sum(e['kind']=='str' for e in audio['entries']),
      'audio_routing_proven':False,
      'reference_tables':'reference.l3ref',
      'note':'Structural cook coverage only; unproved object-table roles and gameplay/event handler semantics remain runtime work.'
    }
    (out/'runtime_pack.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(f"runtime_pack stages={summary['stage_packages']} ebd={summary['ebd_resources']} msg={summary['message_resources']} object_tables={summary['object_tables']} object_lists={summary['object_lists']} objects={summary['object_records']}")
if __name__=='__main__': main()

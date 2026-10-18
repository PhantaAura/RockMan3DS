#!/usr/bin/env python3
from pathlib import Path
import argparse,json,hashlib,sys
REQUIRED={
1:['include/mml_renderer_recovery.h','include/mml_player_material.h','src/mml_renderer_recovery.c','src/mml_player_material.c'],
2:['include/mml_package_v2.h','src/mml_package_v2.c','tools/mml_content_graph_v2.py'],
3:['include/mml_retail_motion.h','src/mml_retail_motion.c','tools/trace_compare.py'],
4:['include/mml_h4_runtime.h','src/mml_h4_runtime.c'],
5:['include/mml_combat_v2.h','src/mml_combat_v2.c','include/mml_retail_anim_request.h'],
6:['include/mml_world_v2.h','src/mml_event_vm_v2.c','src/mml_enemy_bridge_v2.c'],
7:['include/mml_cutscene_v2.h','src/mml_cutscene_v2.c'],
8:['include/mml_retail_save_v2.h','src/mml_native_save_v2.c'],
9:['include/mml_runtime_diag_v2.h','src/mml_loader_guard_v2.c','include/mml_qol_v2.h'],
10:['tools/verify_cumulative_tree.py','state/CURRENT_CHECKPOINT.json','HOUR_MANIFEST.json']}
BAD={'.chd','.bin','.cue','.exe','.dat','.xa','.str','.mcr','.gme','.vgs','.3dsx','.cia'}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('root',nargs='?',default='.');a=ap.parse_args();r=Path(a.root)
 def integrated(x):
  if x.startswith('include/'):return 'include/legends3ds/p2/'+x[8:]
  if x.startswith('src/'):return 'src/p2/'+x[4:]
  if x.startswith('tools/'):return 'tools/p2/'+x[6:]
  if x=='HOUR_MANIFEST.json':return 'research/push2/'+x
  return x
 missing={str(h):[x for x in xs if not (r/integrated(x)).is_file()] for h,xs in REQUIRED.items()};missing={h:x for h,x in missing.items() if x}
 bad=[p.relative_to(r).as_posix() for p in r.rglob('*') if p.is_file() and p.suffix.lower() in BAD]
 out={'complete':not missing and not bad,'hours_present':[h for h in range(1,11) if str(h) not in missing],'missing':missing,'prohibited_payloads':bad}
 print(json.dumps(out,indent=2));sys.exit(0 if out['complete'] else 1)
if __name__=='__main__':main()

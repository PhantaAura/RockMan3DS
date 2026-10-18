#!/usr/bin/env python3
"""Generate and compare repeatable integration replays; not a retail golden trace."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess


def run(tools,pack,out):
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    spec=importlib.util.spec_from_file_location('trace',Path(__file__).parent/'p2/trace_compare.py')
    trace=importlib.util.module_from_spec(spec);spec.loader.exec_module(trace)
    scenarios={
        'FLAT_ACCEL':[(90,16,0,0)], 'RELEASE_COAST':[(30,16,0,0),(60,0,0,0)],
        'BRAKE':[(30,16,0,0),(30,64,0,0)], 'REVERSE':[(30,16,0,0),(60,64,0,0)],
        'TURN_45':[(8,2048,0,0)], 'TURN_90':[(16,2048,0,0)], 'TURN_180':[(32,2048,0,0)],
        'FIRE_WHILE_MOVING':[(60,16,1,0)], 'LOCK_MOVEMENT':[(60,16,0,1)]}
    results=[]
    for name,segments in scenarios.items():
        rows=[]
        for frames,mask,fire,lock in segments:
            for _ in range(frames):rows.append(f'{len(rows)} {mask} {fire} {lock} 0')
        inp=out/(name+'.tsv');inp.write_text('\n'.join(rows)+'\n')
        paths=[out/(name+f'.{i}.json') for i in range(2)]
        for path in paths:subprocess.run([str(Path(tools)/'l3replay'),str(pack),str(inp),str(path)],check=True)
        report=trace.compare(*[json.loads(p.read_text()) for p in paths])
        disabled=out/(name+'.no-animation.json')
        subprocess.run([str(Path(tools)/'l3replay'),str(pack),str(inp),str(disabled),'--no-animation'],check=True)
        # The disabled run suppresses only the presented frame; source animation
        # control records still execute because gameplay consumes their bytes.
        presentation_fields={'animation_frame'}
        without_animation=lambda path:[{k:v for k,v in row.items() if k not in presentation_fields} for row in json.loads(path.read_text())]
        isolation=trace.compare(without_animation(paths[0]),without_animation(disabled))
        if not isolation['exact']:raise RuntimeError('presentation changed gameplay '+name)
        results.append({'scenario':name,'frames':len(rows),'repeatable':report['exact'],'animation_isolation':isolation['exact'],
                        'status':'CANDIDATE','retail_comparison':False})
        if not report['exact']:raise RuntimeError('nondeterministic replay '+name)
    unknown=['JUMP_TAP','JUMP_HOLD','JUMP_APEX','LAND','AIR_CONTROL','SLOPE_UP','SLOPE_DOWN','LEDGE','STEP','WALL','KNOCKBACK']
    results += [{'scenario':n,'status':'UNKNOWN','reason':'full retail solver/state dispatch not connected'} for n in unknown]
    result={'PHYSICS_1TO1':False,'scope':'existing RuntimeSession tick; turn labels are input scenarios, not measured angles',
            'limitations':['grounded unavailable; animation uses the same bridge as the 3DS frontend','no retail golden traces',
                           'lock scenario has no registered retail target','combat uses inherited integration defaults'],
            'scenarios':results}
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('tools',type=Path);p.add_argument('pack',type=Path);p.add_argument('out',type=Path)
    a=p.parse_args();run(a.tools,a.pack,a.out)

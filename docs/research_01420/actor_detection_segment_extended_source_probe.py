#!/usr/bin/env python3
"""Hash-gated, read-only source query proof; authored fixtures and hashes only.

Runs complete original80049C54 without replacement classifier/Y hooks. Actual
stage scalar contexts are written only outside the source tree and stay local.
"""
import argparse
import hashlib
import itertools
import json
import random
import struct
import sys
from pathlib import Path
SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01419'), str(SOURCE / 'docs/research_01418'),
               str(SOURCE / 'docs/research_01415')]
from actor_detection_segment_source_probe import invoke
from contact_source_probe import synthetic_stage
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

def sha(blob): return hashlib.sha256(blob).hexdigest()

def authored_case(image, base, code, flags, raw6, idx, group, interval, source_class, start, end, bounds):
    c = OverlayProbe(image, base, b'')
    synthetic_stage(c, 1, code, flags, raw6)
    for i, value in enumerate(idx): c.put(0x400c + i * 2, 2, value)
    for i, value in enumerate(group): c.put(0x6000 + i * 2, 2, value)
    for i, value in enumerate(interval): c.put(0x7000 + i * 2, 2, value)
    result, cells = invoke(c, start, end, bounds, source_class)
    return [code, flags, raw6] + idx + group + interval + [source_class] + start + end + bounds + [result, cells], c.steps

def stage_cpu(image, base, overlay, selected, admitted, layout):
    c = OverlayProbe(image, base, overlay)
    for resource in selected.values():
        for off, value in enumerate(resource['payload']):
            c.put(resource['load'] + off, 1, value)
            c.put((resource['load'] & 0xffffff) + off, 1, value)
    for identity, seed in admitted:
        c.put(0x8015c006 + identity * 12, 2, seed); c.put(0x15c006 + identity * 12, 2, seed)
    sx, sz, width, height, cells = layout
    for address, size, value in [(0x1f800040, 1, sx), (0x1f800041, 1, sz),
        (0x1f800044, 2, width), (0x1f800046, 2, height), (0x1f800048, 4, cells), (0x1f80004c, 4, 0x8015c000)]:
        c.put(address, size, value)
    return c

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path); p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path); args = p.parse_args()
    if args.out.exists(): raise SystemExit('Refusing to overwrite detector geometry proof')
    args.out.mkdir(parents=True)
    image = args.exe.read_bytes(); assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    rows, stage_rows, instructions = [], [], 0
    codes = [1, 3, 15, 0x21, 0x25, 0x31, 0x35, 0x41, 0x45, 0x51, 0x55, 0x61, 0x71, 8, 0x301]
    profiles = [[-25,25,-35,35,-70,10],[-11,47,-23,31,-200,15],[0,0,0,0,-90,0]]
    coords = [-300,-136,-125,-75,-4,0,4,65,125,136,300]
    for code, bounds, cls, point in itertools.product(codes, profiles, [0,0xffff], itertools.product(coords, repeat=2)):
        x, z = point
        y = [-200,-139,-138,-60,0,197,198,250][(x + z) % 8]
        flags = 0x8005 if (x ^ z) & 1 else 5
        start = [x,y,z,1234]; end = [x+3,y-4,z-3,-1234]
        row, steps = authored_case(image,base,code,flags,(x+z)&1,[0,0,0],[-100,100,-100,100],[-128,128],cls,start,end,bounds)
        rows.append(row); instructions += steps
    # Authored translated/asymmetric/wrapped inputs preserve original short
    # stores and signed32 MULT/DIV semantics. No original constants exported.
    rng = random.Random(1420); trapped = 0
    for family in range(2,7):
        for case in range(160):
            idx = [rng.choice([0,20000,-20000,32740,-32740]) for _ in range(3)]
            x0,z0 = rng.randrange(-300,0),rng.randrange(-300,0)
            group = [x0,x0+rng.randrange(1,601),z0,z0+rng.randrange(1,601)]
            bounds = [rng.randrange(-60,1),rng.randrange(0,61),rng.randrange(-60,1),rng.randrange(0,61),-90,10]
            interval = [-100,100]; start=[rng.randrange(-1000,1001),rng.randrange(-32768,32768),rng.randrange(-1000,1001),1234]; end=start[:]
            code = (family<<4)|rng.choice([1,3,5,15]); flags=rng.choice([0,3,0x8000]);cls=rng.choice([0,0xffff])
            try: row,steps = authored_case(image,base,code,flags,0,idx,group,interval,cls,start,end,bounds)
            except ValueError as error:
                if 'division trap' not in str(error): raise
                trapped += 1; continue
            rows.append(row); instructions += steps
    resources=list(iter_resources(args.dat/'ST00.BIN'))
    overlay=next(r['payload'] for r in resources if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay)==OVERLAY_SHA
    selected={}
    for name,load in [('idx',0x8015c000),('mdt',0x80164000),('stg',0x80194000),('hed',0x8013a000)]:
        found=[r for r in resources if r['type']==0 and r['load']==load]; assert len(found)==1;selected[name]=found[0]
    idx,mdt,stg,hed=[selected[n]['payload'] for n in ['idx','mdt','stg','hed']]
    pointers=[]
    for slot in range(512):
        ptr=struct.unpack_from('<I',hed,slot*8+4)[0]
        if not ptr:break
        pointers.append(ptr&0xffffff)
    admitted=[]
    for identity in range(len(idx)//12):
        descriptor=struct.unpack_from('<I',idx,identity*12+8)[0]
        if not descriptor or descriptor&0xffffff not in pointers:continue
        header=struct.unpack_from('<I',stg,descriptor-0x80194000+24)[0]
        admitted.append((identity,struct.unpack_from('<H',stg,header-0x80194000+2)[0]))
    assert len(admitted)==994
    layout=struct.unpack_from('<4BI',mdt,12)
    bounds=list(struct.unpack_from('<6h',image,0x80097d0c-base+2048))
    for record in [0x8011a274,0x8011a28c,0x8011a2a4]:
        origin=list(struct.unpack_from('<3h',overlay,record-0x80100000+12))
        for dx,dz in itertools.product([-512,-256,-64,0,64,256,512],repeat=2):
            for dy in [0,-60]:
                c=stage_cpu(image,base,overlay,selected,admitted,layout)
                start=[origin[0]+dx,origin[1]+dy,origin[2]+dz,1234]
                end=[start[0]+32,start[1],start[2]-32,-1234]
                result,count=invoke(c,start,end,bounds,0xffff)
                stage_rows.append([0xffff]+start+end+bounds+[result,count]);instructions+=c.steps
    authored=args.out/'actor-segment-extended-authored.tsv'
    authored.write_text('code flags raw6 idx3 group4 interval2 class start4 end4 bounds6 result cells\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows))
    private=args.out/'actor-segment-extended-owner-stage.tsv'
    private.write_text('class start4 end4 ownerbounds6 result cells\n'+''.join('\t'.join(map(str,row))+'\n' for row in stage_rows))
    evidence={'schema':'actor-detection-segment-extended-source/1','status':'SOURCE_BACKED_PARTIAL',
        'exe_sha256':sha(image),'overlay_sha256':sha(overlay),'entry':'0x80049c54',
        'authored_cases':len(rows),'authored_original_trap_cases_excluded':trapped,'materialized_owner_stage_cases':len(stage_rows),
        'original_instructions_executed':instructions,'return_only_hooks':[],
        'authored_fixture_sha256':sha(authored.read_bytes()),'owner_stage_fixture_sha256':sha(private.read_bytes()),
        'resources':{n:sha(r['payload']) for n,r in selected.items()},'materialized_identities':len(admitted),
        'classifier_contract':'families2..5 original short-wrap and diagonal tolerance; family6 original diamond; family7 outer strict bounds only',
        'Y_contract':'8004A060 low8 dispatch; only unchanged interval admitted; slope branches held',
        'limits':['Authored/materialized RAM contexts; not live retail encounter or Old3DS proof.',
            'Owner stage scalar fixture stays local; authored fixture has no owner bounds, geometry or tables.',
            'Original classifier endpoint labels are consumed only as truthiness by this helper; their response scratch is outside this query contract.']}
    (args.out/'source-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps({k:evidence[k] for k in ['authored_cases','materialized_owner_stage_cases','original_instructions_executed','authored_original_trap_cases_excluded']}))
if __name__=='__main__':main()

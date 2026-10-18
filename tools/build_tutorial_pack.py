#!/usr/bin/env python3
"""Build the native Legends3DS tutorial asset pack from a user-owned extracted MML USA DAT directory.

This is an offline cooker. It does not download or redistribute Capcom data. The generated RomFS output
is for the owner of the source disc and MUST NOT be bundled in source releases.
"""
from __future__ import annotations
import argparse, ctypes, hashlib, json, os, struct, subprocess, tempfile, sys
from dataclasses import dataclass
from pathlib import Path
from mml_pipeline import known_negative_material_overlay, NEGATIVE_OVERLAY_SHA256
from cook_actor_combat import OwnerExe, build_metadata
from cook_source_fade import cook as cook_source_fade
from cook_source_event_phase5 import cook as cook_source_event_phase5
from cook_source_event_phase6 import cook as cook_source_event_phase6
from source_player_head import select_fresh_player_head
from cook_source_scene_view import cook as cook_source_scene_view

SECTOR=0x800
SOURCE_ROOT=Path(__file__).resolve().parents[1]
CORE_CONTAINERS=("ST00.BIN","ST00_00.BIN","ST00_01.BIN","ST00_02.BIN","ST00_03.BIN","ST00_04.BIN",
                 "INIT_DAT.BIN","HEAD01.BIN","ARM00R.BIN","ARM00L.BIN","LEG00.BIN")
TYPE0_WANTED={
 r"..\MAP\DATA\ST00_MAP.IDX":"stage/ST00_MAP.IDX",
 r"..\MAP\DATA\ST00_MAP.MDT":"stage/ST00_MAP.MDT",
 r"..\SCR\DATA\ST00.STG":"stage/ST00.STG",
 r"..\SCR\DATA\ST00.HED":"stage/ST00.HED",
 r"..\PROGBIN\R3_ST00.BIN":"stage/R3_ST00.BIN",
 r"..\PLBIN\PL00B00P.PBD":"player/PL00B00P.PBD",
 r"..\PLBIN\PL00H01P.PBD":"player/PL00H01P.PBD",
 r"..\PLBIN\PL00R00P.PBD":"player/PL00R00P.PBD",
 r"..\PLBIN\PL00L00P.PBD":"player/PL00L00P.PBD",
 r"..\PLBIN\PL00F00P.PBD":"player/PL00F00P.PBD",
 r"..\PLBIN\PL00M00S.PBD":"player/PL00M00S.PBD",
 r"..\PLBIN\PL00M00M.PBD":"player/PL00M00M.PBD",
 r"..\PLBIN\PL00R00S.PBD":"player/PL00R00S.PBD",
 r"..\PLBIN\PL00R00M.PBD":"player/PL00R00M.PBD",
 r"..\PLBIN\PL00L00S.PBD":"player/PL00L00S.PBD",
 r"..\PLBIN\PL00L00M.PBD":"player/PL00L00M.PBD",
 r"..\mess\st00_01.msg":"messages/st00_01.msg",
 r"..\mess\st00_02.msg":"messages/st00_02.msg",
}
TYPE0_OPTIONAL={r"..\SCR\DATA\ST00.HD2":"stage/ST00.HD2"}

@dataclass
class Res:
    container: Path; offset:int; typ:int; size:int; load:int; internal:str
    @property
    def payload_start(self): return self.offset+SECTOR

def iter_resources(container:Path):
    d=container.read_bytes()
    for off in range(0,max(0,len(d)-SECTOR+1),SECTOR):
        ps=off+0x40; z=d.find(b'\0',ps,min(ps+256,len(d)))
        if z<0: continue
        try: internal=d[ps:z].decode('ascii')
        except UnicodeDecodeError: continue
        if not internal.startswith('..\\'): continue
        typ,size=struct.unpack_from('<II',d,off); load=struct.unpack_from('<I',d,off+0x0C)[0]
        yield Res(container,off,typ,size,load,internal)

def sha256(path:Path):
    h=hashlib.sha256();
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1<<20),b''):h.update(chunk)
    return h.hexdigest()

def run(tool_dir:Path,*args:str):
    cmd=[str(tool_dir/args[0]),*map(str,args[1:])]
    print('+',' '.join(cmd)); subprocess.run(cmd,check=True)

def safe_name(internal:str):
    name=internal.replace('..\\','').replace('\\','__').replace('/','__')
    return ''.join(c if c.isalnum() or c in '._-' else '_' for c in name)

def _related(a:Path,b:Path)->bool:
    return a==b or a in b.parents or b in a.parents

def _publish_exclusive(stage:Path,out:Path)->None:
    """Atomically publish a directory without replacing even an empty output.

    POSIX rename() may replace an existing empty directory. Use the platform's
    no-replace operation instead; unsupported platforms fail safely.
    """
    if os.name=='nt':
        os.rename(stage,out)  # Windows rename rejects an existing destination.
        return
    libc=ctypes.CDLL(None,use_errno=True)
    if sys.platform=='darwin':
        rename=getattr(libc,'renamex_np',None)
        if rename is None: raise RuntimeError('exclusive directory publication unavailable')
        rename.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.c_uint]
        result=rename(os.fsencode(stage),os.fsencode(out),4)  # RENAME_EXCL
    elif sys.platform.startswith('linux'):
        rename=getattr(libc,'renameat2',None)
        if rename is None: raise RuntimeError('exclusive directory publication unavailable')
        rename.argtypes=[ctypes.c_int,ctypes.c_char_p,ctypes.c_int,ctypes.c_char_p,ctypes.c_uint]
        result=rename(-100,os.fsencode(stage),-100,os.fsencode(out),1)  # AT_FDCWD, RENAME_NOREPLACE
    else:
        raise RuntimeError('exclusive directory publication unavailable on this platform')
    if result:
        code=ctypes.get_errno()
        raise OSError(code,os.strerror(code),str(out))

def _write_actor_source_metadata(rock_neo:Path,runtime:Path,out:Path,manifest:dict)->None:
    """Emit exact-profile sidecars into unpublished staging; never overwrite.

    The shared metadata builder validates the exact owner EXE and source record
    identities. All payloads and provenance are prepared before writing; the
    outer pack publisher keeps any later failure from replacing an old pack.
    No session selector is chosen by cooking or area loading.
    """
    runtime,out,rock_neo=Path(runtime).resolve(),Path(out).resolve(),Path(rock_neo).resolve()
    if not runtime.is_relative_to(out) or any(_related(out,p) for p in (SOURCE_ROOT,rock_neo)):
        raise ValueError('actor metadata must stay in separate owner-local pack staging')
    if 'actor_source_metadata' in manifest:
        raise ValueError('actor metadata provenance already exists')
    packages,evidence=build_metadata(OwnerExe(rock_neo.read_bytes()))
    if any((runtime/name).exists() or (runtime/name).is_symlink() for name in packages):
        raise ValueError('actor metadata output already exists; choose fresh pack staging')
    provenance=dict(evidence)
    provenance['outputs']={name:{**evidence['outputs'][name],
        'path':str((runtime/name).relative_to(out))} for name in packages}
    provenance['runtime_selector_default']=False
    provenance['runtime_binding']='Optional exact session selector and source identity; unknown restore or missing metadata holds.'
    for name,data in packages.items():
        with (runtime/name).open('xb') as file: file.write(data)
    manifest['actor_source_metadata']=provenance

def build_pack(dat_dir:Path,host_build_dir:Path,out_dir:Path,
               with_indexed:bool=False,with_material_cache:bool=False)->Path:
    dat,tools=Path(dat_dir).resolve(),Path(host_build_dir).resolve()
    requested=Path(out_dir)
    if requested.is_symlink(): raise ValueError('output must not be a symlink')
    out=requested.resolve()
    owner=dat.parent.parent  # Required extraction layout: owner/CDDATA/DAT.
    if any(_related(out,p) for p in (SOURCE_ROOT,owner,dat,tools)):
        raise ValueError('output must be separate from source, owner data and host tools')
    if out.exists(): raise ValueError('output already exists; choose a fresh output directory')
    if not dat.is_dir(): raise ValueError('DAT directory not found')
    required_tools=['l3cook_st00','l3cook_render','l3cook_map_init','l3cook_st00_flow','l3cook_st00_spawn','l3cook_pbd','l3cook_player_comp','l3cook_anim','l3cook_skeleton','l3cook_msg','l3cook_ebd']
    if with_indexed: required_tools += ['l3cook_texture','l3cook_palette']
    if with_material_cache: required_tools += ['l3cook_material_cache','l3cook_player_material_cache','l3cook_actor_material_cache','l3cook_negative_materials','l3cook_scene_e0_material']
    for t in required_tools:
        if not (tools/t).is_file(): raise ValueError(f'missing host tool: {tools/t}')
    for name in CORE_CONTAINERS:
        if not (dat/name).is_file(): raise ValueError(f'missing tutorial container: {name}')
    if not (owner/'ROCK_NEO.EXE').is_file(): raise ValueError('ROCK_NEO.EXE not found beside CDDATA')
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.'+out.name+'-staging-',dir=out.parent) as stage:
        _cook_pack(dat,tools,Path(stage),with_indexed,with_material_cache)
        _publish_exclusive(Path(stage),out)
    return out

def _cook_pack(dat:Path,tools:Path,out:Path,with_indexed:bool,with_material_cache:bool)->None:
    (out/'legends3ds/tutorial').mkdir(parents=True)
    runtime=out/'legends3ds/tutorial'; manifest={'format':1,'source':'user-owned Mega Man Legends USA DAT directory','generated':True,'resources':[]}
    with tempfile.TemporaryDirectory(prefix='l3tutorial-') as td0:
        td=Path(td0); found={}
        resources=[]
        for cname in CORE_CONTAINERS:
            cp=dat/cname
            if not cp.exists(): continue
            for r in iter_resources(cp):
                resources.append(r)
                wanted = {**TYPE0_WANTED, **TYPE0_OPTIONAL}
                if r.internal in wanted and r.internal not in found:
                    if r.typ!=0: raise RuntimeError(f'{r.internal}: expected type 0, got {r.typ}')
                    if r.internal in TYPE0_OPTIONAL and r.load != 0x80139F00:
                        raise RuntimeError(f'{r.internal}: unsupported HD2 load address')
                    dst=td/wanted[r.internal]; dst.parent.mkdir(parents=True,exist_ok=True)
                    data_all=r.container.read_bytes()
                    if r.payload_start+r.size>len(data_all): raise RuntimeError(f'{r.container.name}:{r.internal}: type-0 payload out of bounds')
                    data=data_all[r.payload_start:r.payload_start+r.size]; dst.write_bytes(data)
                    found[r.internal]=(r,dst)
        missing=[k for k in TYPE0_WANTED if k not in found]
        if missing: raise RuntimeError('missing required tutorial resources: '+', '.join(missing))
        # Preserve required-resource diagnostics before the strict owner gate.
        # Sidecars enter the normal inventory below; provenance stays in the
        # existing manifest, without an EXE or extra JSON runtime payload.
        head_selection=select_fresh_player_head((dat.parent.parent/'ROCK_NEO.EXE').read_bytes(),dat)
        if head_selection['internal'] not in found:
            raise RuntimeError('Selected fresh original head resource was not collected')
        manifest['player_head_selection']=head_selection
        _write_actor_source_metadata(dat.parent.parent/'ROCK_NEO.EXE',runtime,out,manifest)
        fade_package,fade_evidence=cook_source_fade((dat.parent.parent/'ROCK_NEO.EXE').read_bytes())
        (runtime/'source_fade.l3fd').write_bytes(fade_package)
        manifest['source_fade_startup']={**fade_evidence,
            'path':str((runtime/'source_fade.l3fd').relative_to(out)),
            'status':'SOURCE_BACKED_PARTIAL','scheduler_completion':'UNKNOWN'}
        phase5_package,phase5_evidence=cook_source_event_phase5(
            (dat.parent.parent/'ROCK_NEO.EXE').read_bytes(),dat/'ST00.BIN',dat/'ST00_00.BIN')
        (runtime/'source_event_phase5.l3p5').write_bytes(phase5_package)
        manifest['source_event_phase5']={**phase5_evidence,
            'path':str((runtime/'source_event_phase5.l3p5').relative_to(out))}
        phase6_package,phase6_evidence=cook_source_event_phase6(
            (dat.parent.parent/'ROCK_NEO.EXE').read_bytes(),dat/'ST00.BIN')
        (runtime/'source_event_phase6.l3p6').write_bytes(phase6_package)
        manifest['source_event_phase6']={**phase6_evidence,
            'path':str((runtime/'source_event_phase6.l3p6').relative_to(out))}
        scene_view_package=cook_source_scene_view((dat.parent.parent/'ROCK_NEO.EXE').read_bytes())
        (runtime/'source_scene_view.l3sv').write_bytes(scene_view_package)
        manifest['source_scene_view']={'format':1,'optional':True,
            'runtime_gate':'SOURCE_EVENT_BOOT=1','default_enabled':False,
            'status':'SOURCE_BACKED_PARTIAL','projection_lifecycle':'EXPLICIT_FRESH_EXECUTABLE',
            'owner_exe_sha256':sha256(dat.parent.parent/'ROCK_NEO.EXE'),
            'path':str((runtime/'source_scene_view.l3sv').relative_to(out)),
            'tracked_actor_camera':'UNKNOWN','target_point_camera':'UNKNOWN','retail_parity':False}
        stage=td/'stage'; player=td/'player'; messages=td/'messages'
        hd2_args = [stage/'ST00.HD2'] if r'..\SCR\DATA\ST00.HD2' in found else []
        run(tools,'l3cook_st00',stage/'ST00_MAP.MDT',stage/'ST00_MAP.IDX',stage/'ST00.STG',stage/'ST00.HED',runtime/'st00.l3map',*hd2_args)
        run(tools,'l3cook_render',stage/'ST00_MAP.MDT',stage/'ST00_MAP.IDX',stage/'ST00.STG',stage/'ST00.HED',runtime/'st00.l3render',*hd2_args)
        run(tools,'l3cook_map_init',stage/'ST00_MAP.MDT',stage/'ST00_MAP.IDX',stage/'ST00.STG',stage/'ST00.HED',
            runtime/'st00.l3map',runtime/'st00.l3render',runtime/'st00.l3init','--stage0-startup',*hd2_args)
        manifest['map_initialization']={'format':'L3MI/1','status':'SOURCE_BACKED_PARTIAL',
            'profile':'stage0_startup','section_kind':35,'cooked':'legends3ds/tutorial/st00.l3init',
            'source_routine':'0x8003A3B4','writer':'0x8003A434',
            'admission':'first low24 descriptor match before HED zero terminator or512-entry cap',
            'seed':'unsigned primary-near render-header halfword at+2',
            'bindings':'full cooked L3MP and L3RG CRC32; original raw6 and neutral record identities',
            'evidence':'docs/research_01417/IDX_MATERIALIZATION_WRITER_AUDIT.md',
            'limits':'stage0 startup only; later variant mutations and resumed state remain unbound',
            'source_resources':[{ 'internal':internal,'sha256':sha256(found[internal][1])}
                for internal in (r'..\MAP\DATA\ST00_MAP.IDX',r'..\SCR\DATA\ST00.HED',r'..\SCR\DATA\ST00.STG')]}
        run(tools,'l3cook_st00_flow',stage/'R3_ST00.BIN',runtime/'st00.l3flow')
        run(tools,'l3cook_st00_spawn',stage/'R3_ST00.BIN',runtime/'st00.l3spawn')

        # Exact fixed-point reference table used by the original movement path.
        # The EXE is inferred from the normal extraction layout
        #   extracted/ROCK_NEO.EXE + extracted/CDDATA/DAT/
        # and is never copied to the runtime pack.  Only the pointer-free table
        # produced by the local cooker is retained.
        rock_neo=dat.parent.parent/'ROCK_NEO.EXE'
        if not rock_neo.is_file():
            raise RuntimeError(f'ROCK_NEO.EXE not found beside CDDATA: {rock_neo}')
        subprocess.run([sys.executable, str(Path(__file__).with_name('cook_reference_tables.py')),
                        str(rock_neo),str(runtime/'reference.l3ref')],check=True)
        pout=runtime/'player'; pout.mkdir()
        model_names=[('PL00B00P.PBD','body.l3model'),(head_selection['resource'],'head.l3model'),('PL00R00P.PBD','right.l3model'),('PL00L00P.PBD','left.l3model'),('PL00F00P.PBD','feet.l3model')]
        for src,dst in model_names: run(tools,'l3cook_pbd',player/src,pout/dst)
        run(tools,'l3cook_player_comp',*[player/x for x,_ in model_names],pout/'player.l3pc')
        def load(internal): return found[internal][0].load
        run(tools,'l3cook_anim',player/'PL00M00S.PBD',hex(load(r'..\PLBIN\PL00M00S.PBD')),player/'PL00M00M.PBD',hex(load(r'..\PLBIN\PL00M00M.PBD')),pout/'main.l3anim')
        run(tools,'l3cook_skeleton',player/'PL00M00M.PBD',hex(load(r'..\PLBIN\PL00M00M.PBD')),pout/'main.l3skel')
        subprocess.run([sys.executable,str(Path(__file__).with_name('cook_source_buster_profile.py')),
                        '--exe',str(rock_neo),'--out',str(pout/'buster.l3bp')],check=True)
        manifest['source_buster_profile']={'format':'L3BP/1','status':'SOURCE_BACKED_PARTIAL',
            'selection_count':16,'fresh_lane_initialization':'0x80037534',
            'limits':['weapon selection changes','alternate limb pose bank','projectile callback']}
        run(tools,'l3cook_anim',player/'PL00R00S.PBD',hex(load(r'..\PLBIN\PL00R00S.PBD')),player/'PL00R00M.PBD',hex(load(r'..\PLBIN\PL00R00M.PBD')),pout/'right.l3anim')
        run(tools,'l3cook_anim',player/'PL00L00S.PBD',hex(load(r'..\PLBIN\PL00L00S.PBD')),player/'PL00L00M.PBD',hex(load(r'..\PLBIN\PL00L00M.PBD')),pout/'left.l3anim')
        mout=runtime/'messages'; mout.mkdir()
        run(tools,'l3cook_msg',messages/'st00_01.msg',mout/'st00_01.l3msg')
        run(tools,'l3cook_msg',messages/'st00_02.msg',mout/'st00_02.l3msg')

        # Pointer-free EBD rows plus opaque source resource bytes, retained locally
        # by the v2 cooker. No actor resource semantics are inferred here. ST00_02
        # intentionally reuses ST00_01's EBD payload on the disc, but receives its own
        # area-named runtime directory so area loading stays table-driven.
        eout=runtime/'ebd'; eout.mkdir()
        ebd_internal={
            'ST00_00.BIN':r'..\EMBIN\ST00_00.EBD',
            'ST00_01.BIN':r'..\EMBIN\ST00_01.EBD',
            'ST00_02.BIN':r'..\EMBIN\ST00_01.EBD',
            'ST00_03.BIN':r'..\EMBIN\ST00_03.EBD',
            'ST00_04.BIN':r'..\EMBIN\ST00_04.EBD',
        }
        for area_index,cname in enumerate(ebd_internal):
            cp=dat/cname
            wanted=ebd_internal[cname]
            matches=[r for r in iter_resources(cp) if r.internal == wanted]
            if len(matches)!=1: raise RuntimeError(f'{cname}: expected exactly one {wanted}, got {len(matches)}')
            r=matches[0]
            if r.typ!=0: raise RuntimeError(f'{cname}:{wanted}: expected type 0, got {r.typ}')
            raw=cp.read_bytes()[r.payload_start:r.payload_start+r.size]
            if len(raw)!=r.size: raise RuntimeError(f'{cname}:{wanted}: payload out of bounds')
            ep=td/f'ebd_{area_index:02d}.bin'; ep.write_bytes(raw)
            run(tools,'l3cook_ebd',ep,hex(r.load),eout/f'area_{area_index:02d}.l3ebd')

        if with_material_cache:
            # Build one terrain material cache per tutorial sub-area. PS1 VRAM is stateful,
            # so global/player resources are ordered first, ST00 common resources next, and
            # the selected area container last. The cache cooker uses last-writer semantics.
            mout=runtime/'materials'; mout.mkdir()
            negative_args = []
            if known_negative_material_overlay((stage/'R3_ST00.BIN').read_bytes()):
                negative_path = runtime/'st00.l3nm'
                run(tools,'l3cook_negative_materials',stage/'R3_ST00.BIN',negative_path)
                negative_args = ['--negative-materials',negative_path]
                manifest['negative_materials'] = {'status':'SOURCE_BACKED_PARTIAL',
                    'source_sha256':NEGATIVE_OVERLAY_SHA256,
                    'evidence':'Measured USA overlay; five area-local tables of 32 slots with 25 authored rows'}
            else:
                manifest['negative_materials'] = {'status':'UNKNOWN',
                    'evidence':'Overlay identity does not match the measured table profile; material fallback retained'}
            upload_prefix=[dat/'INIT_DAT.BIN',dat/head_selection['container'],dat/'ARM00R.BIN',dat/'ARM00L.BIN',dat/'LEG00.BIN',dat/'ST00.BIN']
            for area_index in range(5):
                area_container=dat/f'ST00_{area_index:02d}.BIN'
                run(tools,'l3cook_material_cache',stage/'ST00_MAP.MDT',stage/'ST00_MAP.IDX',stage/'ST00.STG',stage/'ST00.HED',
                    str(area_index),mout/f'area_{area_index:02d}.l3mc',*upload_prefix,area_container,*negative_args)
                # Player meshes use their own TPAGE/CLUT words. Cook a parallel
                # per-area cache so stage-local face/palette uploads can replace
                # the common player pages with the same last-writer semantics.
                run(tools,'l3cook_player_material_cache',
                    pout/'body.l3model',pout/'head.l3model',pout/'right.l3model',pout/'left.l3model',pout/'feet.l3model',
                    mout/f'player_area_{area_index:02d}.l3mc',*upload_prefix,area_container)
                run(tools,'l3cook_actor_material_cache',eout/f'area_{area_index:02d}.l3ebd',
                    mout/f'actor_area_{area_index:02d}.l3mc',*upload_prefix,area_container)
            # Independent original E0 material words and ordered initial VRAM
            # snapshot. This is optional/default OFF; it never establishes live
            # animated palette chronology or borrows class20 material rules.
            scene_uploads=[*upload_prefix,dat/'ST00_00.BIN']
            scene_path=mout/'scene_e0_initial.l3sm'
            run(tools,'l3cook_scene_e0_material',runtime/'source_event_phase5.l3p5',
                eout/'area_00.l3ebd',scene_path,*scene_uploads)
            manifest['scene_materials']={'format':1,'optional':True,
                'runtime_gate':'SOURCE_SCENE_TEXTURES=1','default_enabled':False,
                'snapshot':'INITIAL_UPLOAD_BATCH','chronology':'SOURCE_BACKED_PARTIAL',
                'live_palette':'UNKNOWN','path':str(scene_path.relative_to(out)),
                'uploads':[{'name':path.name,'sha256':sha256(path)} for path in scene_uploads]}
            manifest['actor_materials']={'format':1,'optional':True,'runtime_gate':'ACTOR_TEXTURES=1',
                'default_enabled':False,'shade':'UNKNOWN','status':'SOURCE_BACKED_PARTIAL',
                'upload_order':'inherited ordered VRAM resources; live original VRAM comparison pending'}

        if with_indexed:
            iout=runtime/'indexed'; iout.mkdir()
            for r in resources:
                if r.typ not in (1,4,9): continue
                dst=iout/(safe_name(r.container.stem+'__'+r.internal)+('.l3tex' if r.typ==1 else '.l3pal'))
                if r.typ==1: run(tools,'l3cook_texture',r.container,r.internal,dst)
                else: run(tools,'l3cook_palette',r.container,r.internal,dst)
                manifest['resources'].append({'container':r.container.name,'internal':r.internal,'type':r.typ,'cooked':str(dst.relative_to(out))})
    for p in sorted(runtime.rglob('*')):
        if p.is_file(): manifest.setdefault('outputs',[]).append({'path':str(p.relative_to(out)),'bytes':p.stat().st_size,'sha256':sha256(p)})
    (runtime/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('dat_dir',type=Path,help='User-owned extracted CDDATA/DAT directory')
    ap.add_argument('host_build_dir',type=Path,help='Directory containing built l3cook_* tools')
    ap.add_argument('out_dir',type=Path,help='Fresh generated local RomFS/tutorial pack directory')
    ap.add_argument('--with-indexed',action='store_true',help='Also cook individual type-1 textures and type-4/9 palettes from tutorial-loaded containers')
    ap.add_argument('--with-material-cache',action='store_true',help='Cook per-area load-order-aware ST00 material caches for the 3DS renderer')
    a=ap.parse_args()
    try:
        out=build_pack(a.dat_dir,a.host_build_dir,a.out_dir,a.with_indexed,a.with_material_cache)
    except (OSError,ValueError,RuntimeError,subprocess.CalledProcessError) as error:
        ap.exit(1,f'Tutorial pack failed: {error}\n')
    print(f'Pack complete: {out}')
    print(f'Files: {sum(1 for p in (out/"legends3ds/tutorial").rglob("*") if p.is_file())}')

if __name__=='__main__': main()

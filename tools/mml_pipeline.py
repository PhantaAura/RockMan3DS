#!/usr/bin/env python3
"""Incremental owner-side content graph and neutral package cooker.

Edges describe measured containment/byte identity, never inferred gameplay use.
Unknown event, progression, animation and texture-state semantics remain unknown.
No retail payload belongs in a source release.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib
from build_stage_catalog import iter_resources, stage_key
from build_stage_object_catalog import find_tables, cook as cook_objects

VERSION = '01411-visibility-1'
PROFILE = 'mml-usa-10d2008f2837c466'
LOADS = {'idx': 0x8015C000, 'mdt': 0x80164000, 'stg': 0x80194000, 'hed': 0x8013A000}
OPTIONAL_LOADS = {'hd2': 0x80139F00}
NEGATIVE_OVERLAY_SHA256 = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
NEGATIVE_OVERLAY_BYTES = 141056
NEGATIVE_OVERLAY_CRC32 = 0x46628AF1
UNKNOWN = ['actors', 'enemies', 'triggers', 'transitions', 'event_scripts', 'cameras',
           'animations', 'audio_refs', 'progression_state', 'texture_state']


def sha(data):
    return hashlib.sha256(data).hexdigest()


def known_negative_material_overlay(data):
    """Fixed-offset tables are admitted only for the measured USA overlay."""
    return (len(data) == NEGATIVE_OVERLAY_BYTES and
            zlib.crc32(data) == NEGATIVE_OVERLAY_CRC32 and
            sha(data) == NEGATIVE_OVERLAY_SHA256)


def geometry_key(internal):
    # HD2 precedes HED in memory and supplies the bounded bit7 descriptor slots.
    return stage_key(internal[:-3]+'HED') if internal.upper().endswith('.HD2') else stage_key(internal)


def encoded(obj):
    return (json.dumps(obj, sort_keys=True, separators=(',', ':'), allow_nan=False)+'\n').encode()


def atomic(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as f:
        tmp = Path(f.name)
        try:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        except BaseException:
            tmp.unlink(missing_ok=True)
            raise
    try:
        os.replace(tmp, path)
    finally:
        tmp.unlink(missing_ok=True)


def package(sections):
    if len(sections) > 64 or len({k for k, _ in sections}) != len(sections):
        raise ValueError('duplicate/excessive sections')
    offset = 20+16*len(sections)
    table, payload = bytearray(), bytearray()
    for kind, data in sections:
        table += struct.pack('<IIII', kind, offset, len(data), zlib.crc32(data))
        payload += data
        offset += len(data)
    return struct.pack('<IHHIII', 0x324c4d4d, 2, len(sections), offset,
                       zlib.crc32(PROFILE.encode()), zlib.crc32(table))+table+payload


class Cache:
    """Cache immutable tasks by actual inputs AND tool bytes; verify every cache hit."""
    def __init__(self, out, tools, cache_dir=None):
        self.out, self.tools = Path(out), Path(tools)
        self.cache_dir = Path(cache_dir) if cache_dir is not None else self.out/'cache'
        self.hits = self.misses = 0
        self.source_hash = sha(Path(__file__).read_bytes()+
            Path(__file__).with_name('build_stage_catalog.py').read_bytes()+
            Path(__file__).with_name('build_stage_object_catalog.py').read_bytes())

    def run(self, tool, inputs, args):
        exe = self.tools/tool
        key = sha(encoded({'version': VERSION, 'pipeline': self.source_hash, 'tool': sha(exe.read_bytes()),
                           'inputs': {k: sha(v) for k,v in inputs.items()}, 'args': args}))
        dst = self.cache_dir/key/'payload'
        meta = dst.with_name('provenance.json')
        if dst.is_file() and meta.is_file():
            try:
                info = json.loads(meta.read_text())
                data = dst.read_bytes()
                if info['output_sha256'] == sha(data) and info['key'] == key:
                    self.hits += 1
                    return data, info
            except (ValueError, KeyError, TypeError, OSError):
                pass  # Invalid generated cache metadata is rebuilt from inputs.
        with tempfile.TemporaryDirectory(prefix='mml-cook-') as td:
            td = Path(td)
            for name, data in inputs.items():
                (td/name).write_bytes(data)
            cmd = [str(exe.resolve())] + [str(td/x[1:]) if x.startswith('@') else x for x in args]
            p = subprocess.run(cmd, capture_output=True, text=True)
            if p.returncode:
                raise ValueError(tool+': '+p.stderr.strip()[-600:])
            data = (td/'out').read_bytes()
        info = {'key': key, 'cooker': tool, 'cooker_sha256': sha(exe.read_bytes()),
                'pipeline_sha256': self.source_hash, 'version': VERSION, 'profile': PROFILE,
                'dependencies': {k: sha(v) for k,v in inputs.items()}, 'output_sha256': sha(data)}
        atomic(dst, data)
        atomic(meta, encoded(info))
        self.misses += 1
        return data, info


def graph_validate(graph):
    ids = [n['id'] for n in graph['nodes']]
    if len(ids) != len(set(ids)):
        raise ValueError('duplicate graph node')
    ids = set(ids)
    for e in graph['edges']:
        if e['from'] not in ids or e['to'] not in ids:
            raise ValueError('dangling graph edge')
    states = [n for n in graph['nodes'] if n['kind'] == 'gameplay_state']
    if len(states) != len({n['source_container'] for n in states}):
        raise ValueError('gameplay states merged')


def build(dat, tools, out, *, cache_dir=None, container_names=None):
    dat, out = Path(dat).resolve(), Path(out).resolve()
    if not dat.is_dir() or out == dat or dat in out.parents or out in dat.parents:
        raise ValueError('output must be separate from owner source')
    source_root = Path(__file__).resolve().parents[1]
    if out == source_root or source_root in out.parents:
        raise ValueError('owner output must be outside distributable source tree')
    if cache_dir is not None:
        cache_dir = Path(cache_dir).resolve()
        if (cache_dir == dat or dat in cache_dir.parents or cache_dir in dat.parents or
                cache_dir == source_root or source_root in cache_dir.parents or cache_dir in source_root.parents):
            raise ValueError('cache must be separate from owner source and distributable source')
    cache = Cache(out, tools, cache_dir)
    graph = {'format': 'mml-content-graph-3', 'nodes': [], 'edges': []}
    families, entries = set(), []
    if container_names is None:
        containers = sorted(dat.glob('*.BIN'))
    else:
        names = list(container_names)
        if len(names) != len(set(names)) or any(Path(n).name != n or not n.endswith('.BIN') for n in names):
            raise ValueError('container selection must contain unique BIN basenames')
        containers = sorted(dat/n for n in names)
        if any(not p.is_file() for p in containers):
            raise ValueError('selected DAT container is missing')
    if not containers:
        raise ValueError('no DAT containers')
    for cp in containers:
        data = cp.read_bytes()
        cid = 'container:'+cp.name
        state = 'state:'+cp.stem
        graph['nodes'].append({'id': cid, 'kind': 'container', 'sha256': sha(data), 'bytes': len(data)})
        graph['nodes'].append({'id': state, 'kind': 'gameplay_state' if cp.name.startswith('ST') else 'resource_bundle',
                               'source_container': cid})
        graph['edges'].append({'from': state, 'to': cid, 'relation': 'source', 'status': 'MATCHES_MEASURED_FIELDS'})
        resources = list(iter_resources(cp))
        manifest = {'format': 'MMLAreaPackage/1', 'profile': PROFILE, 'cooker_version': VERSION,
                    'source_container': cp.name, 'source_sha256': sha(data), 'state_id': state,
                    'geometry_family': None, 'resources': [], 'components': {}, 'unknown': UNKNOWN[:],
                    'status': 'SOURCE_BACKED_PARTIAL', 'dependencies': [], 'errors': []}
        sections, quartets, negative_overlays = [], {}, []
        stats = {'resources': len(resources), 'geometry_sections': 0, 'object_tables': 0,
                 'object_lists': 0, 'object_records': 0, 'ebd_resources': 0, 'message_resources': 0,
                 'texture_upload_resources': 0, 'palette_upload_resources': 0}
        for r in resources:
            rid = cid+':'+hex(r['offset'])
            ext = r['internal'].upper().rsplit('.',1)[-1]
            kind = {'EBD':'ebd_directory','MSG':'messages'}.get(ext, 'raw_resource')
            record = {k:r[k] for k in ('offset','type','size','load','internal')}
            record.update(id=rid, sha256=sha(r['payload']))
            manifest['resources'].append(record)
            graph['nodes'].append({'id':rid,'kind':kind, **record})
            graph['edges'].append({'from':state,'to':rid,'relation':'contains', 'status':'MATCHES_MEASURED_FIELDS'})
            stats['texture_upload_resources'] += r['type'] == 1
            stats['palette_upload_resources'] += r['type'] in (4,9)
            if r['type'] != 0:
                continue
            key = geometry_key(r['internal'])
            if key:
                slot = {'IDX':'idx','MDT':'mdt','STG':'stg','HED':'hed','HD2':'hd2'}[ext]
                q = quartets.setdefault(key,{})
                if slot in q:
                    raise ValueError('ambiguous duplicate geometry slot in '+cp.name)
                q[slot] = r
            if ext in ('MSG', 'EBD'):
                tool = 'l3cook_msg' if ext == 'MSG' else 'l3cook_ebd'
                args = ['@source', '@out'] if ext == 'MSG' else ['@source',hex(r['load']),'@out']
                try:
                    cooked, provenance = cache.run(tool, {'source':r['payload']}, args)
                    sk = 0x100+len(sections)
                    sections.append((sk,cooked))
                    manifest['dependencies'].append(provenance)
                    record['section_kind'] = sk
                    stats['message_resources' if ext == 'MSG' else 'ebd_resources'] += 1
                except ValueError as e:
                    manifest['errors'].append(str(e))
            if ext == 'BIN' and 'PROGBIN' in r['internal'].upper() and r['load'] == 0x80100000:
                if known_negative_material_overlay(r['payload']):
                    negative_overlays.append((r, record))
                tables = find_tables(r['payload'], r['load'])
                if tables:
                    if any(k == 4 for k,_ in sections):
                        manifest['errors'].append('multiple object overlays: no arbitrary selection')
                    else:
                        sections.append((4,cook_objects(r['load'],tables)))
                        stats['object_tables'] = len(tables)
                        stats['object_lists'] = sum(len(ls) for _,ls in tables)
                        stats['object_records'] = sum(len(recs) for _,ls in tables for _,recs in ls)
                        manifest['components']['objects'] = {'status':'CANDIDATE','section_kind':4,
                            'evidence':'terminated 20-byte lists and consecutive in-overlay pointers; roles unknown'}
        complete = [q for q in quartets.values() if set(LOADS) <= set(q)]
        if len(complete) > 1:
            manifest['errors'].append('multiple geometry quartets: area binding unknown')
        elif complete:
            q = complete[0]
            identity = sha(encoded({k:sha(q[k]['payload']) for k in sorted(q)}))
            family = 'geometry:'+identity
            manifest['geometry_family'] = family
            if family not in families:
                graph['nodes'].append({'id':family,'kind':'geometry_family','identity':'sha256 of ordered geometry resource hashes'})
                families.add(family)
            graph['edges'].append({'from':state,'to':family,'relation':'contains_geometry','status':'MATCHES_MEASURED_FIELDS'})
            expected_loads = {**LOADS, **{k:v for k,v in OPTIONAL_LOADS.items() if k in q}}
            if {k:q[k]['load'] for k in q} != expected_loads:
                manifest['errors'].append('unsupported geometry load layout')
            else:
                for tool, sk, name in [('l3cook_st00',2,'collision'),('l3cook_render',3,'geometry')]:
                    try:
                        args = ['@mdt','@idx','@stg','@hed','@out'] + (['@hd2'] if 'hd2' in q else [])
                        cooked, provenance = cache.run(tool,{k:r['payload'] for k,r in q.items()},args)
                        sections.append((sk,cooked))
                        manifest['dependencies'].append(provenance)
                        manifest['components'][name]={'section_kind':sk,'status':'SOURCE_BACKED_PARTIAL'}
                        if sk == 2:
                            names = ['areas','cells','positive_cells','negative_cells','indices','descriptors',
                                     'contact_lists','contact_groups','collision_records']
                            stats.update(zip(names,struct.unpack_from('<9I',cooked,12)))
                        else:
                            names=['render_descriptors','render_records','render_streams','stream_refs','material_refs','primitives']
                            stats.update(zip(names,struct.unpack_from('<6I',cooked,12)))
                            stats['geometry_sections'] += 1
                    except ValueError as e:
                        manifest['errors'].append(str(e))
                if len(negative_overlays) > 1:
                    manifest['errors'].append('multiple known negative material overlays: no arbitrary selection')
                elif negative_overlays and any(k == 2 for k,_ in sections):
                    overlay, record = negative_overlays[0]
                    try:
                        cooked, provenance = cache.run('l3cook_negative_materials',
                            {'overlay':overlay['payload']}, ['@overlay','@out'])
                        if struct.unpack_from('<H', cooked, 6)[0] != stats['areas']:
                            raise ValueError('negative material table/map area count mismatch')
                        sections.append((32, cooked))
                        record['section_kind'] = 32
                        manifest['dependencies'].append(provenance)
                        manifest['components']['negative_materials'] = {
                            'section_kind':32, 'status':'SOURCE_BACKED_PARTIAL',
                            'source_sha256':NEGATIVE_OVERLAY_SHA256,
                            'evidence':'Measured USA overlay; five area-local tables of 32 slots, with 25 authored rows and canonical unknown rows'}
                        known_rows = sum(cooked[offset + 5] == 1 for offset in range(16, len(cooked), 8))
                        stats.update(negative_material_areas=stats['areas'], negative_material_known_rows=known_rows,
                                     negative_material_unknown_rows=stats['areas'] * 32 - known_rows)
                    except ValueError as e:
                        manifest['errors'].append(str(e))
                if 'negative_materials' not in manifest['components']:
                    manifest['unknown'].append('negative_material_bindings')
        if not manifest['geometry_family']:
            manifest['unknown'].append('geometry_binding')
        manifest['stats'] = stats
        blob = package([(1,encoded(manifest))]+sections)
        atomic(out/'areas'/(cp.stem+'.mmlpkg'),blob)
        manifest['package_bytes'] = len(blob)
        manifest['package_sha256'] = sha(blob)
        atomic(out/'areas'/(cp.stem+'.json'),encoded(manifest))
        entries.append(manifest)
    graph_validate(graph)
    atomic(out/'content_graph.json',encoded(graph))
    corpus = {'version':VERSION,'profile':PROFILE,'entries':[{
        'state_id':e['state_id'],'source_sha256':e['source_sha256'],'geometry_family':e['geometry_family'],
        'stats':e['stats'],'resource_hashes':[r['sha256'] for r in e['resources']], 'errors':e['errors']
        } for e in entries]}
    atomic(out/'regression_corpus.json',encoded(corpus))
    summary={'packages':len(entries),'geometry_families':len(families),'nodes':len(graph['nodes']),
             'edges':len(graph['edges']),'cache_hits':cache.hits,'cache_misses':cache.misses,
             'packages_with_errors':sum(bool(e['errors']) for e in entries)}
    atomic(out/'summary.json',encoded(summary))
    return summary


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('dat',type=Path);p.add_argument('tools',type=Path);p.add_argument('out',type=Path)
    a=p.parse_args()
    result=build(a.dat,a.tools,a.out)
    print(json.dumps(result,indent=2))
    return 1 if result['packages_with_errors'] else 0

if __name__=='__main__':
    raise SystemExit(main())

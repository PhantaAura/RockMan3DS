#!/usr/bin/env python3
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from mml_pipeline import Cache, package, graph_validate
import mml_pipeline as pipeline
from test_cooked_area import synthetic_map, synthetic_render, synthetic_negative_materials
from mml_exe_research import Image, index_function
spec=importlib.util.spec_from_file_location('trace_compare',ROOT/'tools/p2/trace_compare.py')
trace=importlib.util.module_from_spec(spec);spec.loader.exec_module(trace)

class PipelineTests(unittest.TestCase):
    def test_section_wire_format(self):
        b=package([(1,b'{}'),(2,b'abcd')])
        self.assertEqual(struct.unpack_from('<I',b,8)[0],len(b))
        self.assertEqual(struct.unpack_from('<I',b,16)[0],zlib.crc32(b[20:52]))
        self.assertEqual(b[52:],b'{}abcd')
        with self.assertRaises(ValueError):package([(1,b''),(1,b'')])
        with self.assertRaises(ValueError):package([(k,b'') for k in range(65)])

    def test_graph_no_dangling_or_duplicate_states(self):
        g={'nodes':[{'id':'a','kind':'gameplay_state','source_container':'x'},
                    {'id':'b','kind':'geometry_family'}],
           'edges':[{'from':'a','to':'b'}]}
        graph_validate(g)
        g['edges'].append({'from':'b','to':'missing'})
        with self.assertRaises(ValueError):graph_validate(g)
        g['edges'].pop();g['nodes'].append({'id':'c','kind':'gameplay_state','source_container':'x'})
        with self.assertRaises(ValueError):graph_validate(g)

    def test_recursive_trace_and_missing_null(self):
        a=[{'frame':0,'position':[1,2,3],'state':True,'heading':4095}]
        self.assertTrue(trace.compare(a,a)['exact'])
        b=[{'frame':0,'position':[1,4,3],'state':1,'heading':0}]
        r=trace.compare(a,b)
        self.assertFalse(r['exact']);self.assertEqual(r['field_errors']['heading']['max_abs_error'],1)
        self.assertEqual(r['field_errors']['position[1]']['max_abs_error'],2)
        self.assertIn('state',r['mismatch_counts'])
        self.assertFalse(trace.compare([{'x':None}],[{}])['exact'])
        self.assertFalse(trace.compare([{'x':float('nan')}],[{'x':float('nan')}])['exact'])
        self.assertFalse(trace.compare(a,[])['exact'])

    def test_mips_control_flow(self):
        b=bytearray(2048+64);b[:8]=b'PS-X EXE';base=0x80010000
        struct.pack_into('<4I',b,0x10,base,0,base,64)
        # jal base+32; delay slot; jr ra; delay slot
        struct.pack_into('<4I',b,2048,(3<<26)|((base+32)>>2&0x3ffffff),0,0x03e00008,0)
        f=index_function(Image(b),base)
        self.assertEqual(f['visited_instruction_count'],4)
        self.assertEqual(f['callees'][0]['target'],hex(base+32))
        with self.assertRaises(ValueError):Image(b[:2049])

    def test_cache_rebuilds_on_tool_input_or_corruption(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td);(p/'tools').mkdir();(p/'tools/tool').write_bytes(b'version1')
            def fake(cmd,**kwargs):
                Path(cmd[-1]).write_bytes(Path(cmd[1]).read_bytes()+b'cooked')
                class Result:returncode=0;stderr=''
                return Result()
            c=Cache(p/'out',p/'tools')
            with patch('mml_pipeline.subprocess.run',side_effect=fake):
                a,info=c.run('tool',{'src':b'a'},['@src','@out'])
                self.assertEqual(c.misses,1)
                c.run('tool',{'src':b'a'},['@src','@out']);self.assertEqual(c.hits,1)
                (c.out/'cache'/info['key']/'payload').write_bytes(b'corrupt')
                c.run('tool',{'src':b'a'},['@src','@out']);self.assertEqual(c.misses,2)
                c.run('tool',{'src':b'b'},['@src','@out']);self.assertEqual(c.misses,3)
                (p/'tools/tool').write_bytes(b'version2')
                c.run('tool',{'src':b'a'},['@src','@out']);self.assertEqual(c.misses,4)


class VisibilityPipelineTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='mml-visibility-pipeline-')
        self.base = Path(self.temp.name).resolve()
        self.dat = self.base/'owner/CDDATA/DAT'
        self.dat.mkdir(parents=True)
        self.calls = []

    def tearDown(self):
        self.temp.cleanup()

    def container(self, hd2=None, overlay=None, hd2_load=0x80139f00):
        resources = [(r'..\MAP\DATA\ST00_MAP.'+suffix, load, ('synthetic '+suffix).encode())
                     for suffix, load in (('IDX',0x8015c000),('MDT',0x80164000))]
        resources += [(r'..\SCR\DATA\ST00.'+suffix, load, ('synthetic '+suffix).encode())
                      for suffix, load in (('STG',0x80194000),('HED',0x8013a000))]
        if hd2 is not None: resources.append((r'..\SCR\DATA\ST00.HD2',hd2_load,hd2))
        if overlay is not None: resources.append((r'..\PROGBIN\R3_ST00.BIN',0x80100000,overlay))
        blob = bytearray()
        for name, load, payload in resources:
            record = bytearray(4096)
            struct.pack_into('<II', record, 0, 0, len(payload))
            struct.pack_into('<I', record, 12, load)
            record[64:64+len(name)] = name.encode()
            record[2048:2048+len(payload)] = payload
            blob += record
        (self.dat/'ST00.BIN').write_bytes(blob)

    def build(self, name='output'):
        def cook(cache, tool, inputs, args):
            self.calls.append((tool,inputs,args))
            data = {'l3cook_st00':synthetic_map(5), 'l3cook_render':synthetic_render(),
                    'l3cook_negative_materials':synthetic_negative_materials(5)}[tool]
            return data, {'cooker':tool,'dependencies':{k:pipeline.sha(v) for k,v in inputs.items()}}
        out = self.base/name
        with patch.object(Cache,'run',cook):
            result = pipeline.build(self.dat,self.base/'host',out)
        return result, json.loads((out/'areas/ST00.json').read_text())

    def test_optional_hd2_reaches_both_cookers_and_changes_geometry_identity(self):
        self.container(hd2=b'synthetic HD2')
        result, first = self.build()
        self.assertEqual(result['packages_with_errors'],0)
        self.assertEqual([call[0] for call in self.calls],['l3cook_st00','l3cook_render'])
        for tool,inputs,args in self.calls:
            self.assertEqual(args,['@mdt','@idx','@stg','@hed','@out','@hd2'])
            self.assertEqual(inputs['hd2'],b'synthetic HD2')
        self.container(hd2=b'changed synthetic HD2')
        _, second = self.build('second')
        self.assertNotEqual(first['geometry_family'],second['geometry_family'])

    def test_legacy_quartet_and_unknown_overlay_retain_optional_fallback(self):
        self.container(overlay=b'unknown synthetic overlay')
        result, manifest = self.build()
        self.assertEqual(result['packages_with_errors'],0)
        self.assertIn('negative_material_bindings',manifest['unknown'])
        self.assertNotIn('negative_materials',manifest['components'])
        for tool,inputs,args in self.calls:
            self.assertEqual(args,['@mdt','@idx','@stg','@hed','@out'])
            self.assertNotIn('hd2',inputs)

    def test_only_exact_overlay_profile_admits_section_32(self):
        overlay = b'original synthetic profile fixture'
        self.container(overlay=overlay)
        with patch.object(pipeline,'NEGATIVE_OVERLAY_BYTES',len(overlay)), \
             patch.object(pipeline,'NEGATIVE_OVERLAY_CRC32',zlib.crc32(overlay)), \
             patch.object(pipeline,'NEGATIVE_OVERLAY_SHA256',pipeline.sha(overlay)):
            result, manifest = self.build()
            self.assertTrue(pipeline.known_negative_material_overlay(overlay))
            self.assertFalse(pipeline.known_negative_material_overlay(overlay[:-1]+b'!'))
        self.assertEqual(result['packages_with_errors'],0)
        self.assertEqual(self.calls[-1][0],'l3cook_negative_materials')
        self.assertEqual(self.calls[-1][1],{'overlay':overlay})
        self.assertEqual(manifest['components']['negative_materials']['section_kind'],32)
        blob=(self.base/'output/areas/ST00.mmlpkg').read_bytes()
        count=struct.unpack_from('<H',blob,6)[0]
        payloads={kind:blob[offset:offset+size] for kind,offset,size,_ in
                  (struct.unpack_from('<IIII',blob,20+i*16) for i in range(count))}
        self.assertEqual(payloads[32],synthetic_negative_materials(5))
        self.assertEqual(manifest['stats']['negative_material_known_rows'],5)

    def test_bad_hd2_load_is_reported_without_cooker_selection(self):
        self.container(hd2=b'synthetic HD2',hd2_load=0x80139f04)
        result, manifest = self.build()
        self.assertEqual(result['packages_with_errors'],1)
        self.assertEqual(manifest['errors'],['unsupported geometry load layout'])
        self.assertEqual(self.calls,[])

if __name__=='__main__':unittest.main()

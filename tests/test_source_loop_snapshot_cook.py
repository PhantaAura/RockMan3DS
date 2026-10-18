#!/usr/bin/env python3
"""Authored parser/protocol rejection cases; no original checkpoint payload."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import unittest
from unittest.mock import patch
import zlib

SOURCE=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('loop_cooker',SOURCE/'tools/cook_source_loop_snapshot.py')
cooker=importlib.util.module_from_spec(spec);spec.loader.exec_module(cooker)

def fixture(loops=37):
    row={'kind':'scene_constructor','pc':cooker.PC,'ordinal':1,'loops':loops,'vsync':99,
        'scratch0':0,'scratch1':2,'scratch_half2':(loops-1)&65535,'word_80098888':0,
        'main_phase':3,'main_subphase':2,'event_id':1,'event_phase':5,
        'pad_packet':[0,65,255,255,0,0,0,0],'pad_held':0,'pad_pressed':0,
        'read_only':True,'retail_parity':False}
    observation={'status':'MATCHES_MEASURED_FIELDS','scope':cooker.SCOPE,'read_only':True,
        'memory_cards_inserted':False,'native_seed_admitted':False,'trace_sha256':'12'*32,'scene_constructor':[row]}
    profile={'read_only_observer':True,'memory_cards_inserted':False,'retail_parity':False,
        'exe_sha256':cooker.EXE_SHA,'bios_sha256':cooker.BIOS_SHA,'observer_sha256':'34'*32,'profile_sha256':'56'*32}
    return observation,profile
class CookTests(unittest.TestCase):
    def test_authored_roundtrip(self):
        for loops in (1,37,65536,65537,0xffffffff):
            observation,profile=fixture(loops);row=cooker.validate_context(observation,profile)
            package=cooker.package_from_row(row);self.assertEqual(len(package),176)
            cooker.validate_package(package);self.assertEqual(struct.unpack_from('<I',package,20)[0],loops)
    def test_json_duplicate_nonfinite_type(self):
        for data in (b'{"x":1,"x":2}',b'{"x":NaN}',b'[]',b'',b'{}'*600000):
            with self.subTest(data=data[:20]),self.assertRaises((ValueError,UnicodeDecodeError)):cooker.decode_json(data)
    def test_context_domains(self):
        for field,value in [('pc',cooker.PC+4),('loops',0),('loops',True),('loops',0x100000000),
            ('scratch_half2',True),('scratch_half2',65536),('scratch_half2',1),('scratch0',1),('scratch1',1),
            ('event_id',2),('event_phase',6),('main_phase',2),('main_subphase',3),('ordinal',2),
            ('read_only',False),('retail_parity',True),('pad_packet',[0]*7),('pad_packet',[0]*7+[True])]:
            observation,profile=fixture();observation['scene_constructor'][0][field]=value
            with self.subTest(field=field,value=value),self.assertRaises(ValueError):cooker.validate_context(observation,profile)
    def test_context_missing_extra(self):
        observation,profile=fixture()
        for field in list(observation['scene_constructor'][0]):
            bad=copy.deepcopy(observation);del bad['scene_constructor'][0][field]
            with self.subTest(field=field),self.assertRaises(ValueError):cooker.validate_context(bad,profile)
        bad=copy.deepcopy(observation);bad['scene_constructor'][0]['invented_seed']=0
        with self.assertRaises(ValueError):cooker.validate_context(bad,profile)
        for rows in ([],[observation['scene_constructor'][0]]*2):
            bad=copy.deepcopy(observation);bad['scene_constructor']=rows
            with self.assertRaises(ValueError):cooker.validate_context(bad,profile)
    def test_reference_scope(self):
        for field,value in [('status','SOURCE_BACKED'),('scope','retail'),('read_only',False),('memory_cards_inserted',True),('native_seed_admitted',True),('trace_sha256','0'*64)]:
            observation,profile=fixture();observation[field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):cooker.validate_context(observation,profile)
        for field,value in [('read_only_observer',False),('memory_cards_inserted',True),('retail_parity',True),('exe_sha256','12'*32),('bios_sha256','12'*32),('observer_sha256','0'*64),('profile_sha256','bad')]:
            observation,profile=fixture();profile[field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):cooker.validate_context(observation,profile)
    def test_package_tamper(self):
        observation,profile=fixture();package=cooker.package_from_row(cooker.validate_context(observation,profile))
        for offset in range(len(package)):
            bad=bytearray(package);bad[offset]^=1
            if offset>=16:struct.pack_into('<I',bad,12,zlib.crc32(bad[16:])&0xffffffff)
            if offset in (22,23):
                cooker.validate_package(bytes(bad));continue
            with self.subTest(offset=offset),self.assertRaises(ValueError):cooker.validate_package(bytes(bad))
        for bad in (package[:-1],package+b'\0'):
            with self.assertRaises(ValueError):cooker.validate_package(bad)
    def test_cook_rejects_authored_observation_before_owner_reads(self):
        observation,profile=fixture()
        with self.assertRaises(ValueError):cooker.cook(json.dumps(observation).encode(),json.dumps(profile).encode(),b'',b'',Path('/missing'),Path('/missing_area'))
    def test_program_and_area_owners_are_independent(self):
        # Authored resource identities let these guards run without original
        # program/EBD payloads. Production sha() always hashes supplied bytes.
        program={'load':0x80100000,'internal':'PROGBIN','type':0,'payload':b'authored-program'}
        area={'load':0x8016c000,'internal':'EBD','type':0,'payload':b'authored-area'}
        identity=lambda data:{b'authored-program':cooker.OVERLAY_SHA,b'authored-area':cooker.EBD_SHA}.get(data,'0'*64)
        with patch.object(cooker,'sha',side_effect=identity):
            cooker.validate_resource_owners([program],[area])
            for programs,areas in [([], [area]),([program,program],[area]),([program],[]),([program],[area,area]),
                ([program,area],[]),([dict(program,type=1)],[area]),([program],[dict(area,payload=b'wrong')])]:
                with self.subTest(programs=programs,areas=areas),self.assertRaises(ValueError):cooker.validate_resource_owners(programs,areas)
if __name__=='__main__':unittest.main()

#!/usr/bin/env python3
"""Synthetic safety checks; never read or convert supplied retail game data."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import build_tutorial_pack as cooker
import release_source as release
import verify_0146_source as verifier
import add_actor_material_caches as actor_caches
import build_hardware_test as hardware_builder


class TutorialOutputTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='mml-source-tools-')
        self.base = Path(self.tmp.name).resolve()
        self.owner = self.base / 'owner'
        self.dat = self.owner / 'CDDATA/DAT'
        self.dat.mkdir(parents=True)
        # Empty fixtures deliberately cannot pass the retail resource decoder.
        for name in cooker.CORE_CONTAINERS:
            (self.dat / name).write_bytes(b'')
        (self.owner / 'ROCK_NEO.EXE').write_bytes(b'')
        self.tools = self.base / 'host'
        self.tools.mkdir()
        for name in ('st00', 'render', 'map_init', 'st00_flow', 'st00_spawn', 'pbd', 'player_comp', 'anim',
                     'skeleton', 'msg', 'ebd'):
            (self.tools / ('l3cook_' + name)).write_text('synthetic fixture\n')
        self.out = self.base / 'result'

    def tearDown(self):
        self.tmp.cleanup()

    def test_reject_source_owner_and_input_relationships(self):
        paths = (ROOT, ROOT / 'payload', ROOT.parent, self.owner,
                 self.owner / 'generated', self.owner.parent, self.dat,
                 self.dat / 'generated', self.dat.parent, self.tools,
                 self.tools / 'generated')
        with patch.object(cooker, '_cook_pack') as cook:
            for path in paths:
                with self.subTest(path=path), self.assertRaises(ValueError):
                    cooker.build_pack(self.dat, self.tools, path)
            cook.assert_not_called()
        self.assertEqual((self.owner / 'ROCK_NEO.EXE').read_bytes(), b'')

    def test_existing_output_is_preserved(self):
        self.out.mkdir()
        marker = self.out / 'keep.txt'
        marker.write_text('earlier checkpoint')
        with self.assertRaises(ValueError):
            cooker.build_pack(self.dat, self.tools, self.out)
        self.assertEqual(marker.read_text(), 'earlier checkpoint')

    def test_invalid_extraction_does_not_publish_output(self):
        with self.assertRaisesRegex(RuntimeError, 'missing required tutorial resources'):
            cooker.build_pack(self.dat, self.tools, self.out)
        self.assertFalse(self.out.exists())
        self.assertEqual(list(self.base.glob('.result-staging-*')), [])
        self.assertTrue(all((self.dat / name).is_file() for name in cooker.CORE_CONTAINERS))

    def test_missing_input_rejected_before_staging(self):
        (self.dat / cooker.CORE_CONTAINERS[0]).unlink()
        with self.assertRaisesRegex(ValueError, 'missing tutorial container'):
            cooker.build_pack(self.dat, self.tools, self.out)
        self.assertFalse(self.out.exists())
        self.assertEqual(list(self.base.glob('.result-staging-*')), [])

    def test_complete_staged_output_is_published(self):
        def fake_cook(dat, tools, stage, indexed, materials):
            self.assertNotEqual(stage, self.out)
            self.assertEqual(stage.parent, self.out.parent)
            (stage / 'synthetic.txt').write_text('completed fixture')
        with patch.object(cooker, '_cook_pack', side_effect=fake_cook):
            result = cooker.build_pack(self.dat, self.tools, self.out)
        self.assertEqual(result, self.out)
        self.assertEqual((self.out / 'synthetic.txt').read_text(), 'completed fixture')
        self.assertEqual(list(self.base.glob('.result-staging-*')), [])

    def test_concurrent_empty_output_is_not_replaced(self):
        def fake_cook(dat, tools, stage, indexed, materials):
            (stage / 'synthetic.txt').write_text('completed fixture')
            self.out.mkdir()
        with patch.object(cooker, '_cook_pack', side_effect=fake_cook), self.assertRaises(OSError):
            cooker.build_pack(self.dat, self.tools, self.out)
        self.assertEqual(list(self.out.iterdir()), [])
        self.assertEqual(list(self.base.glob('.result-staging-*')), [])

    def test_output_symlink_is_rejected(self):
        self.out.symlink_to(self.base / 'uncreated', target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'symlink'):
            cooker.build_pack(self.dat, self.tools, self.out)
        self.assertTrue(self.out.is_symlink())
        self.assertFalse((self.base / 'uncreated').exists())

    def test_tutorial_hd2_and_negative_table_reach_the_expected_cookers(self):
        resources = []
        for internal in (*cooker.TYPE0_WANTED, *cooker.TYPE0_OPTIONAL):
            load = 0x80139f00 if internal.endswith('.HD2') else 0x80100000
            resources.append((internal,load))
        for area in range(5):
            name = r'..\EMBIN\ST00_01.EBD' if area == 2 else rf'..\EMBIN\ST00_{area:02d}.EBD'
            cp = self.dat/f'ST00_{area:02d}.BIN'
            record = bytearray(4096)
            struct.pack_into('<II',record,0,0,9)
            struct.pack_into('<I',record,12,0x80100000)
            record[64:64+len(name)] = name.encode()
            record[2048:2057] = b'synthetic'
            cp.write_bytes(record)
        common = bytearray()
        for internal,load in resources:
            record = bytearray(4096)
            struct.pack_into('<II',record,0,0,9)
            struct.pack_into('<I',record,12,load)
            record[64:64+len(internal)] = internal.encode()
            record[2048:2057] = b'synthetic'
            common += record
        (self.dat/'ST00.BIN').write_bytes(common)
        for name in ('texture','palette','material_cache','player_material_cache','actor_material_cache','negative_materials','scene_e0_material'):
            (self.tools/('l3cook_'+name)).write_text('synthetic fixture\n')
        calls = []
        output_index = {'l3cook_st00':5,'l3cook_render':5,'l3cook_map_init':7,'l3cook_st00_flow':2,'l3cook_st00_spawn':2,
            'l3cook_pbd':2,'l3cook_player_comp':6,'l3cook_anim':5,'l3cook_skeleton':3,
            'l3cook_msg':2,'l3cook_ebd':3,'l3cook_negative_materials':2,
            'l3cook_material_cache':6,'l3cook_player_material_cache':6,'l3cook_actor_material_cache':2,'l3cook_scene_e0_material':3}
        def fake_run(tools,*args):
            calls.append(args)
            Path(args[output_index[args[0]]]).write_bytes(b'synthetic cooked output')
        def fake_reference(args,**kwargs):
            Path(args[3]).write_bytes(b'synthetic reference table')
            return subprocess.CompletedProcess(args,0)
        # This test routes authored resources through fake host cookers. Keep
        # real metadata preparation/writes/publication, but supply its authored
        # bounded OwnerExe dependency. Exact owner-identity rejection is covered
        # separately in test_cook_actor_combat; no retail fixture is needed here.
        from test_cook_actor_combat import SyntheticOwner
        with patch.object(cooker,'run',side_effect=fake_run), \
             patch.object(cooker.subprocess,'run',side_effect=fake_reference), \
             patch.object(cooker,'known_negative_material_overlay',return_value=True), \
             patch.object(cooker,'OwnerExe',return_value=SyntheticOwner()), \
             patch.object(cooker,'cook_source_fade',return_value=(b'authored fade profile',{'retail_parity':False})), \
             patch.object(cooker,'cook_source_event_phase5',return_value=(b'authored scene profile',{'retail_parity':False})), \
             patch.object(cooker,'cook_source_event_phase6',return_value=(b'authored camera timeline profile',{'retail_parity':False})), \
             patch.object(cooker,'cook_source_scene_view',return_value=b'authored view profile'), \
             patch.object(cooker,'cook_hit_proxy_rules',return_value=b'authored proxy rules'), \
             patch.object(cooker,'cook_class60_06',return_value=b'authored child profile'), \
             patch.object(cooker,'cook_reaction_profile',return_value=b'authored reaction profile'), \
             patch.object(cooker,'cook_actor_angle',return_value=b'authored angle profile'), \
             patch.object(cooker,'select_fresh_player_head',return_value={
                 'container':'HEAD01.BIN','resource':'PL00H01P.PBD','internal':r'..\PLBIN\PL00H01P.PBD',
                 'provenance':{'fresh_only':True}}):
            cooker.build_pack(self.dat,self.tools,self.out,with_material_cache=True)
        for call in calls:
            if call[0] in ('l3cook_st00','l3cook_render'):
                self.assertEqual(Path(call[5]).suffix,'.l3map' if call[0] == 'l3cook_st00' else '.l3render')
                self.assertEqual(Path(call[6]).name,'ST00.HD2')
            elif call[0] == 'l3cook_material_cache':
                self.assertEqual(call[-2],'--ramp-out')
                self.assertEqual(Path(call[-1]).name,f'area_{int(call[5]):02d}.l3cr')
                self.assertEqual(call[-4],'--negative-materials')
                self.assertEqual(Path(call[-3]).name,'st00.l3nm')
                self.assertEqual(Path(call[-5]).name,f'ST00_{int(call[5]):02d}.BIN')
        manifest=json.loads((self.out/'legends3ds/tutorial/manifest.json').read_text())
        self.assertEqual(manifest['negative_materials']['status'],'SOURCE_BACKED_PARTIAL')
        self.assertIn('legends3ds/tutorial/st00.l3nm',{r['path'] for r in manifest['outputs']})
        self.assertIn('legends3ds/tutorial/source_event_phase6.l3p6',{r['path'] for r in manifest['outputs']})
        self.assertFalse(manifest['source_event_phase6']['retail_parity'])
        self.assertIn('legends3ds/tutorial/source_scene_view.l3sv',{r['path'] for r in manifest['outputs']})
        self.assertFalse(manifest['source_scene_view']['default_enabled'])
        self.assertEqual(manifest['source_scene_view']['projection_lifecycle'],'EXPLICIT_FRESH_EXECUTABLE')
        self.assertIn('legends3ds/tutorial/materials/scene_e0_initial.l3sm',{r['path'] for r in manifest['outputs']})
        self.assertFalse(manifest['scene_materials']['default_enabled'])
        self.assertEqual(manifest['scene_materials']['live_palette'],'UNKNOWN')
        scene_calls=[c for c in calls if c[0]=='l3cook_scene_e0_material']
        self.assertEqual(len(scene_calls),1)
        self.assertEqual([Path(p).name for p in scene_calls[0][1:4]],
                         ['source_event_phase5.l3p5','area_00.l3ebd','scene_e0_initial.l3sm'])
        self.assertEqual([Path(p).name for p in scene_calls[0][4:]],
                         ['INIT_DAT.BIN','HEAD01.BIN','ARM00R.BIN','ARM00L.BIN','LEG00.BIN','ST00.BIN','ST00_00.BIN'])
        provenance=manifest['actor_source_metadata']
        self.assertFalse(provenance['runtime_selector_default'])
        for name,record in provenance['outputs'].items():
            self.assertIn('legends3ds/tutorial/'+name,{r['path'] for r in manifest['outputs']})
            self.assertEqual(record['sha256'],cooker.sha256(self.out/record['path']))
        self.assertFalse(manifest['actor_materials']['default_enabled'])
        self.assertTrue(manifest['player_head_selection']['provenance']['fresh_only'])
        combat=manifest['combat_sidecars']
        self.assertTrue(combat['default_enabled'])
        for name in ('hit_proxy_rules.l3pr','class60_06.l3c6','actor_reactions.l3r7','actor_angle.l3ref'):
            self.assertIn('legends3ds/tutorial/'+name,{r['path'] for r in manifest['outputs']})
            self.assertEqual(combat['outputs'][name]['sha256'],cooker.sha256(self.out/combat['outputs'][name]['path']))
        self.assertNotIn('legends3ds/tutorial/source_rng.l3rg',{r['path'] for r in manifest['outputs']})
        self.assertIn('legends3ds/tutorial/source_fade.l3fd',{r['path'] for r in manifest['outputs']})
        self.assertIn('legends3ds/tutorial/source_event_phase5.l3p5',{r['path'] for r in manifest['outputs']})
        for call in calls:
            if call[0] in ('l3cook_material_cache','l3cook_player_material_cache','l3cook_actor_material_cache'):
                self.assertIn('HEAD01.BIN',{Path(str(arg)).name for arg in call})
                self.assertNotIn('HEAD00.BIN',{Path(str(arg)).name for arg in call})
            if call[0]=='l3cook_pbd' and Path(call[2]).name=='head.l3model':
                self.assertEqual(Path(call[1]).name,'PL00H01P.PBD')
        for area in range(5):
            self.assertIn(f'legends3ds/tutorial/materials/actor_area_{area:02d}.l3mc',{r['path'] for r in manifest['outputs']})
        actor_calls=[c for c in calls if c[0]=='l3cook_actor_material_cache']
        self.assertEqual(len(actor_calls),5)
        for area,call in enumerate(actor_calls):
            self.assertEqual(Path(call[1]).name,f'area_{area:02d}.l3ebd')
            self.assertEqual(Path(call[-1]).name,f'ST00_{area:02d}.BIN')


class ActorMaterialPackTests(unittest.TestCase):
    def test_extend_preserves_existing_pack_and_requires_optional_metadata(self):
        with tempfile.TemporaryDirectory(prefix='mml-actor-pack-') as temp:
            root=Path(temp);pack=root/'old';pack.mkdir();dat=root/'owner';dat.mkdir();host=root/'host';host.mkdir()
            tool=host/'l3cook_actor_material_cache';tool.write_text('authored synthetic fixture')
            names={'st00.l3map','st00.l3render','st00.l3flow','reference.l3ref',
                'messages/st00_01.l3msg','messages/st00_02.l3msg','player/player.l3pc','player/main.l3anim','player/main.l3skel','player/buster.l3bp'}
            names.update('player/'+p+'.l3model' for p in ('body','head','right','left','feet'))
            for area in range(5):names.update((f'ebd/area_{area:02d}.l3ebd',f'materials/area_{area:02d}.l3mc',f'materials/player_area_{area:02d}.l3mc'))
            manifest={'format':1,'outputs':[]}
            for name in sorted(names):
                path=pack/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'authored fixture')
                manifest['outputs'].append({'path':'legends3ds/tutorial/'+name,'bytes':path.stat().st_size,'sha256':hardware_builder.sha256(path)})
            (pack/'manifest.json').write_text(json.dumps(manifest));before=hardware_builder.validate_pack(pack)
            for name in (*actor_caches.UPLOAD_PREFIX,*(f'ST00_{a:02d}.BIN' for a in range(5))):(dat/name).write_bytes(b'authored fixture')
            def cook(command,**kwargs):
                Path(command[2]).write_bytes(b'authored new cache')
                return subprocess.CompletedProcess(command,0,'synthetic successful cook','')
            out=root/'new'
            with patch.object(actor_caches.subprocess,'run',side_effect=cook) as runner:
                result=actor_caches.extend_pack(pack,dat,tool,out);self.assertEqual(runner.call_count,5)
            self.assertEqual(hardware_builder.validate_pack(pack),before)
            self.assertTrue(result['input_unchanged']);self.assertEqual(result['new_optional_files'],5)
            after=json.loads((out/'manifest.json').read_text());self.assertFalse(after['actor_materials']['default_enabled'])
            self.assertEqual(after['actor_materials']['previous_manifest_sha256'],before['manifest_sha256'])
            with self.assertRaises(ValueError):actor_caches.extend_pack(pack,dat,tool,out)
            after['actor_materials']['default_enabled']=True;(out/'manifest.json').write_text(json.dumps(after))
            with self.assertRaisesRegex(ValueError,'default-OFF'):hardware_builder.validate_pack(out)
            self.assertEqual(hardware_builder.validate_pack(pack),before)


class SceneRuntimeMetadataTests(unittest.TestCase):
    def setUp(self):
        from test_local_install import make_pack
        self.tmp=tempfile.TemporaryDirectory(prefix='mml-scene-metadata-')
        self.base=Path(self.tmp.name);self.pack=make_pack(self.base/'pack')
        self.manifest=json.loads((self.pack/'manifest.json').read_text())
        self.digest=hashlib.sha256(b'authored source EBD identity').digest()
        phase5=bytearray(620);phase5[:4]=b'L3P5';phase5[64:96]=self.digest
        self.write_payload('source_event_phase5.l3p5',bytes(phase5))
    def tearDown(self):self.tmp.cleanup()
    def write_payload(self,name,data):
        p=self.pack/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        self.manifest['outputs']=[row for row in self.manifest['outputs'] if row['path']!='legends3ds/tutorial/'+name]
        self.manifest['outputs'].append({'path':'legends3ds/tutorial/'+name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    def save(self):(self.pack/'manifest.json').write_text(json.dumps(self.manifest))
    def add_view(self):
        import cook_source_scene_view as view
        from test_source_scene_view_cooker import AuthoredOwner
        with patch.object(view,'OwnerExe',AuthoredOwner):data=view.cook(b'AUTHORED')
        self.write_payload('source_scene_view.l3sv',data)
        self.manifest['source_scene_view']={'format':1,'optional':True,'runtime_gate':'SOURCE_EVENT_BOOT=1',
            'default_enabled':False,'status':'SOURCE_BACKED_PARTIAL','projection_lifecycle':'EXPLICIT_FRESH_EXECUTABLE',
            'owner_exe_sha256':view.OWNER_SHA,'path':'legends3ds/tutorial/source_scene_view.l3sv','retail_parity':False}
        self.save();return data
    def material_bytes(self):
        # One valid unresolved 4-bit material, with no substituted texture/CLUT.
        record=bytearray(40);record[8]=4
        payload=struct.pack('<4sHH4I4H4I',b'L3MC',1,48,3,1,0,1,256,256,40,0,48,88,88,zlib.crc32(record)&0xffffffff)+record
        return struct.pack('<4s5I',b'L3SM',0x00400001,1,64+len(payload),64,zlib.crc32(payload)&0xffffffff)+self.digest+bytes(8)+payload
    def add_material(self):
        data=self.material_bytes();self.write_payload('materials/scene_e0_initial.l3sm',data)
        self.manifest['scene_materials']={'format':1,'optional':True,'runtime_gate':'SOURCE_SCENE_TEXTURES=1',
            'default_enabled':False,'snapshot':'INITIAL_UPLOAD_BATCH','chronology':'SOURCE_BACKED_PARTIAL',
            'live_palette':'UNKNOWN','path':'legends3ds/tutorial/materials/scene_e0_initial.l3sm',
            'uploads':[{'name':name,'sha256':hashlib.sha256(('authored '+name).encode()).hexdigest()}
                for name in ('INIT_DAT.BIN','HEAD01.BIN','ARM00R.BIN','ARM00L.BIN','LEG00.BIN','ST00.BIN','ST00_00.BIN')]}
        self.save();return data
    def test_legacy_and_optional_metadata_preserve_default_off(self):
        self.save();legacy=hardware_builder.validate_pack(self.pack)
        self.add_view();self.add_material();after=hardware_builder.validate_pack(self.pack)
        self.assertEqual(after['manifest_outputs'],legacy['manifest_outputs']+2)
        for key in ('source_scene_view','scene_materials'):
            self.manifest[key]['default_enabled']=True;self.save()
            with self.assertRaisesRegex(ValueError,'default-OFF'):hardware_builder.validate_pack(self.pack)
            self.manifest[key]['default_enabled']=False
        self.manifest['scene_materials']['live_palette']='VERIFIED_1TO1';self.save()
        with self.assertRaisesRegex(ValueError,'UNKNOWN live palette'):hardware_builder.validate_pack(self.pack)
    def test_actor40_initial_material_has_independent_identity_and_default_off(self):
        import copy
        self.add_material()
        data=self.material_bytes();self.write_payload('materials/scene_actor40_initial.l3sm',data)
        row=copy.deepcopy(self.manifest['scene_materials']);row['path']='legends3ds/tutorial/materials/scene_actor40_initial.l3sm'
        self.manifest['scene_actor40_materials']=row;self.save();hardware_builder.validate_pack(self.pack)
        row['default_enabled']=True;self.save()
        with self.assertRaisesRegex(ValueError,'default-OFF'):hardware_builder.validate_pack(self.pack)
        row['default_enabled']=False;row['path']=self.manifest['scene_materials']['path'];self.save()
        with self.assertRaisesRegex(ValueError,'scene material manifest'):hardware_builder.validate_pack(self.pack)
        row['path']='legends3ds/tutorial/materials/scene_actor40_initial.l3sm'
        broken=bytearray(data);broken[24]^=1
        self.write_payload('materials/scene_actor40_initial.l3sm',broken);self.save()
        with self.assertRaisesRegex(ValueError,'EBD identity'):hardware_builder.validate_pack(self.pack)
    def test_loop_snapshot_requires_recorded_scope_and_matching_package_identity(self):
        import cook_source_loop_snapshot as loop
        from test_source_loop_snapshot_cook import fixture
        data=loop.package_from_row(fixture()[0]['scene_constructor'][0])
        self.write_payload('source_loop_snapshot.l3ls',data)
        self.manifest['source_loop_snapshot']={'format':1,'optional':True,'default_enabled':False,
            'runtime_gate':'SOURCE_LOOP_REPLAY=1','status':'MATCHES_MEASURED_FIELDS',
            'scope':'RECORDED_OPENBIOS_CONSTRUCTOR','retail_chronology':False,
            'path':'legends3ds/tutorial/source_loop_snapshot.l3ls'}
        # An otherwise supported replay may not admit another resident EBD.
        self.save()
        with self.assertRaisesRegex(ValueError,'EBD identity'):hardware_builder.validate_pack(self.pack)
        phase5=bytearray(620);phase5[:4]=b'L3P5';phase5[64:96]=bytes.fromhex(loop.EBD_SHA)
        self.write_payload('source_event_phase5.l3p5',phase5);self.save();hardware_builder.validate_pack(self.pack)
        self.manifest['source_loop_snapshot']['retail_chronology']=True;self.save()
        with self.assertRaisesRegex(ValueError,'recorded replay scope'):hardware_builder.validate_pack(self.pack)
    def test_optional_files_require_declared_exact_metadata(self):
        self.add_view();del self.manifest['source_scene_view'];self.save()
        with self.assertRaisesRegex(ValueError,'scene view manifest'):hardware_builder.validate_pack(self.pack)
        self.add_view();self.add_material();self.manifest['scene_materials']['path']='legends3ds/tutorial/materials/other.l3sm';self.save()
        with self.assertRaisesRegex(ValueError,'scene material manifest'):hardware_builder.validate_pack(self.pack)
        self.manifest['scene_materials']['path']='legends3ds/tutorial/materials/scene_e0_initial.l3sm'
        self.manifest['scene_materials']['uploads'].reverse();self.save()
        with self.assertRaisesRegex(ValueError,'upload order'):hardware_builder.validate_pack(self.pack)
    def test_view_malformed_payload_rejected_even_after_inventory_hash_refresh(self):
        good=self.add_view()
        for off in (0,4,6,8,12,16,20,24,28,32,64,68,72,76,2124):
            with self.subTest(offset=off):
                bad=bytearray(good);bad[off]^=1
                if off>=64:struct.pack_into('<I',bad,24,zlib.crc32(bad[64:])&0xffffffff)
                self.write_payload('source_scene_view.l3sv',bad);self.save()
                with self.assertRaisesRegex(ValueError,'scene view'):hardware_builder.validate_pack(self.pack)
        self.write_payload('source_scene_view.l3sv',good);self.save();hardware_builder.validate_pack(self.pack)
    def test_material_malformed_identity_envelope_and_budget_rejected(self):
        good=self.add_material()
        for off in (0,4,8,12,16,20,24,56,60,64,76):
            with self.subTest(offset=off):
                bad=bytearray(good);bad[off]^=1
                if off>=64:struct.pack_into('<I',bad,20,zlib.crc32(bad[64:])&0xffffffff)
                self.write_payload('materials/scene_e0_initial.l3sm',bad);self.save()
                with self.assertRaisesRegex(ValueError,'scene material'):hardware_builder.validate_pack(self.pack)
        self.write_payload('materials/scene_e0_initial.l3sm',good);self.save();hardware_builder.validate_pack(self.pack)
    def test_snapshot_extension_preserves_prior_pack(self):
        import add_scene_material_snapshot as snapshots
        self.save();before=hardware_builder.validate_pack(self.pack)
        dat=self.base/'owner';dat.mkdir();tool=self.base/'host'/'l3cook_scene_e0_material';tool.parent.mkdir();tool.write_text('authored cooker')
        for name in snapshots.UPLOADS:(dat/name).write_bytes(('authored '+name).encode())
        def run(argv,**kwargs):
            Path(argv[3]).write_bytes(self.material_bytes());return subprocess.CompletedProcess(argv,0,'authored successful cook','')
        out=self.base/'extended'
        with patch.object(snapshots.subprocess,'run',side_effect=run):result=snapshots.extend_pack(self.pack,dat,tool,out)
        self.assertTrue(result['input_unchanged']);self.assertEqual(result['new_optional_files'],1)
        self.assertEqual(hardware_builder.validate_pack(self.pack),before)
        self.assertEqual(hardware_builder.validate_pack(out)['manifest_outputs'],before['manifest_outputs']+1)
        with self.assertRaises(ValueError):snapshots.extend_pack(self.pack,dat,tool,out)


class SourcePolicyTests(unittest.TestCase):
    def test_current_cumulative_native_target_and_frozen_shaders(self):
        result = verifier.verify_source(ROOT)
        self.assertEqual(result['status'], 'PASS', result['errors'])
        self.assertTrue(result['native_target'].startswith('Legends3DS_'))
        self.assertEqual(result['owner_payload_hits'], [])

    def test_source_verifier_reports_all_cooked_and_retail_types(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            for name in ('area.mmlpkg', 'texture.l3tex', 'palette.l3pal',
                         'player.l3pc', 'game.BIN', 'game.CHd', 'disc.iso',
                         'ROCK_NEO.EXE', 'save.gme', 'audio.xa', 'video.str', 'stage.l3spawn'):
                (root / name).write_bytes(b'synthetic')
            result = verifier.verify_source(root)
            self.assertEqual(len(result['owner_payload_hits']), 12)
            self.assertEqual(result['status'], 'FAIL')

    def test_release_rejects_payload_suffixes_and_renamed_binary(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            for name, data in [('area.mmlpkg', b'synthetic'), ('data.l3map', b'synthetic'),
                               ('disc.chd', b'synthetic'), ('payload.txt', b'L3MP\x00synthetic'),
                               ('game.BIN', b'synthetic'), ('runtime.3dsx', b'synthetic'),
                               ('unexpected.png', b'synthetic'), ('stage.l3spawn', b'synthetic'),
                               ('renamed_spawn.txt', b'L3SP\x00synthetic'),
                               ('renamed_payload.lua', b'L3SP\x00synthetic')]:
                path = root / name
                path.write_bytes(data)
                with self.subTest(name=name), self.assertRaises(ValueError):
                    release.collect_source_files(root)
                path.unlink()

    def test_release_preserves_textual_stack_usage_evidence(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            stack=root/'native-stack.su'
            stack.write_text('source.cpp:1:1:entry 128 static\n')
            self.assertEqual(release.collect_source_files(root),[stack])
            stack.write_bytes(b'fake\x00binary')
            with self.assertRaisesRegex(ValueError,'binary data'):
                release.collect_source_files(root)

    def test_release_rejects_symlink(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / 'text.md').write_text('original source')
            (root / 'alias.md').symlink_to(root / 'text.md')
            with self.assertRaisesRegex(ValueError, 'symlink'):
                release.collect_source_files(root)

    def test_complete_source_zip_contains_text_and_valid_manifest(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            root = base / 'source'
            (root / 'state').mkdir(parents=True)
            (root / 'state/CURRENT_CHECKPOINT.json').write_text(json.dumps({'version': '0.0.14.9'}))
            (root / 'main.cpp').write_text('int main() { return 0; }\n')
            (root / 'observer.lua').write_text('-- synthetic read-only observer\nreturn 0\n')
            output = base / 'Checkpoint.zip'
            result = release.release_source(root, output)
            self.assertEqual(result['sha256'], hashlib.sha256(output.read_bytes()).hexdigest())
            with zipfile.ZipFile(output) as archive:
                self.assertIsNone(archive.testzip())
                names = archive.namelist()
                self.assertEqual(len(names), 4)
                self.assertTrue(all(name.startswith('Legends3DS_0.0.14.9_Source/') for name in names))
                manifest = archive.read('Legends3DS_0.0.14.9_Source/SOURCE_MANIFEST.sha256').decode()
                self.assertIn(hashlib.sha256((root / 'main.cpp').read_bytes()).hexdigest() + '  main.cpp', manifest)
                self.assertEqual(archive.read('Legends3DS_0.0.14.9_Source/observer.lua'),
                                 (root / 'observer.lua').read_bytes())
            with self.assertRaises(ValueError):
                release.release_source(root, output)


if __name__ == '__main__':
    unittest.main()

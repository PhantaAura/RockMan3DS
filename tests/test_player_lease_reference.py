#!/usr/bin/env python3
"""Reference protocol guards; authored rows are not retail observations."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
TOOL=ROOT/'tools/compare_player_lease_reference.py'
spec=importlib.util.spec_from_file_location('lease_comparator',TOOL)
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def rows():
    before={'kind':'player_lease_enter','ordinal':1,'caller_ra':0x80034ed0,
      'scratch_camera_1f800008':[4096,0,4096,0,4096,0,0,2000],
      'mask_80098798':0x80000100,'owner_pointers_800be478':[0]*24,
      'selected_owners':[],'cp2c':[0]*32,'cp2d':[0]*32,
      'source_identity_matches':True,'retail_parity':False}
    for slot in (0,23):
        pointer=0x800b5000;before['owner_pointers_800be478'][slot]=pointer
        before['selected_owners'].append({'slot':slot,'pointer':pointer,'physical':0xb5000,
                                         'alias_first_slot':0,'words_bc_c0':[0x100020,0x300040]})
    after=copy.deepcopy(before);after['kind']='player_lease_return'
    for owner in after['selected_owners']:owner['words_bc_c0']=[0,0]
    return before,after

class LeaseReferenceTests(unittest.TestCase):
    def test_owner_identity_and_alias(self):
        before,after=rows();normalized=module.normalize(before,after)
        self.assertEqual(len(normalized),258)
        self.assertEqual(normalized[0],1)
        # The distinct virtual slots share one physical owner.
        self.assertEqual(normalized[74],0xb5000)
        self.assertEqual(normalized[74+23],0xb5000)
    def test_reject_unowned_or_mutated_source(self):
        mutations=(lambda b,a:b.update(source_identity_matches=False),
                   lambda b,a:a.update(mask_80098798=0),
                   lambda b,a:a.update(caller_ra=0),
                   lambda b,a:b['selected_owners'].pop(),
                   lambda b,a:a['selected_owners'][0].update(physical=0x1234),
                   lambda b,a:b['selected_owners'][1].update(words_bc_c0=[0,0]),
                   lambda b,a:b['selected_owners'][1].update(alias_first_slot=23),
                   lambda b,a:b['cp2c'].append(0),
                   lambda b,a:b['cp2d'].__setitem__(0,-1))
        for mutate in mutations:
            before,after=rows();mutate(before,after)
            with self.assertRaises((ValueError,KeyError)):module.normalize(before,after)
    def run_guard(self,records,admit_start=True):
        if admit_start:records=[{'kind':'lease_observer_start','source_exe_sha256':module.OWNER_SHA,'source_function':0x80035508}]+records
        with tempfile.TemporaryDirectory(prefix='mml-lease-reference-') as directory:
            root=Path(directory);reference=root/'reference.jsonl'
            reference.write_text(''.join(json.dumps(row)+'\n' for row in records))
            result=subprocess.run([sys.executable,str(TOOL),'--reference',str(reference),
              '--native-checker',str(root/'does-not-exist'),'--out',str(root/'out')],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            proof=json.loads((root/'out/comparison-proof.json').read_text())
            self.assertEqual(proof['status'],'UNKNOWN');self.assertFalse(proof['checker_invoked'])
            self.assertFalse(proof['retail_parity']);self.assertFalse(proof['whole_trace_accepted'])
    def test_empty_trace_is_not_pass(self):self.run_guard([])
    def test_pending_call_is_not_pass(self):self.run_guard([rows()[0]])
    def test_unpaired_return_is_not_pass(self):self.run_guard([rows()[1]])
    def test_guard_rejects_completed_prefix(self):
        before,after=rows();self.run_guard([before,after,{'kind':'lease_observer_guard_failure','reason':'capacity'}])
    def test_absent_provenance_is_not_pass(self):
        self.run_guard(list(rows()),False)

if __name__=='__main__':unittest.main()

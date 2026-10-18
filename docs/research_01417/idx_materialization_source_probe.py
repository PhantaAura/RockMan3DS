#!/usr/bin/env python3
"""Hash-gated original IDX materialization replay; scalar/hash evidence only.

Reads the extracted owner EXE and existing stage resources. No raw instruction,
resource, table, RAM or save bytes are serialized. The synthetic dictionary is
local instrumentation and never writes to a running emulator or owner input.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01414')]
from build_stage_catalog import iter_resources
from player_contact_source_probe import CPU, SUPPORTED_EXE, s16, s32

START, END, WRITER = 0x8003A3B4, 0x8003A460, 0x8003A434
IDX, HED, STACK = 0x8015C000, 0x8013A000, 0x00700000


def sha(data):
    return hashlib.sha256(data).hexdigest()


class MaterializationCPU(CPU):
    """Minimal bounded integer region runner with original branch delay slots.

    Only original code fetched from the hash-gated EXE executes. Region-specific
    instruction semantics are generic integer decode, not embedded retail code.
    """
    def __init__(self, image, base):
        super().__init__(image, base)
        self.writes = []

    def run_region(self):
        pc, pending = START, None
        while pc != END:
            if self.steps >= 3000000:
                raise ValueError('bounded materialization replay exhausted')
            if not START <= pc < END:
                raise ValueError('source region edge escaped: ' + hex(pc))
            # Code is immutable owner EXE; synthetic data cannot replace a fetch.
            w = struct.unpack_from('<I', self.image, 2048 + pc - self.base)[0]
            op, rs, rt, rd, fn = w >> 26, w >> 21 & 31, w >> 16 & 31, w >> 11 & 31, w & 63
            immediate = w & 65535
            si = s16(immediate)
            a, b = self.r[rs], self.r[rt]
            old, pending, value, destination = pending, None, None, rt
            if not w:
                pass
            elif op == 0:
                destination = rd
                if fn == 33: value = a + b
                elif fn == 36: value = a & b
                else: raise ValueError('unsupported region integer operation at ' + hex(pc))
            elif op == 9: value = a + si
            elif op == 10: value = int(s32(a) < si)
            elif op == 13: value = a | immediate
            elif op == 15: value = immediate << 16
            elif op in (35, 37):
                value = self.get((a + si) & 0xFFFFFFFF, 4 if op == 35 else 2)
            elif op in (41, 43):
                address, width = (a + si) & 0xFFFFFFFF, 2 if op == 41 else 4
                self.put(address, width, b)
                if IDX <= address < IDX + 2048 * 12:
                    self.writes.append({'writer_pc': hex(pc), 'index': (address - IDX) // 12,
                                        'field_offset': (address - IDX) % 12, 'width': width,
                                        'value': b & ((1 << (width * 8)) - 1)})
            elif op in (4, 5):
                if (a == b) == (op == 4): pending = pc + 4 + si * 4
            elif op == 2:
                pending = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            else:
                raise ValueError('unsupported region operation at ' + hex(pc))
            if value is not None and destination:
                self.r[destination] = value & 0xFFFFFFFF
            self.r[0] = 0
            self.steps += 1
            pc = old if old is not None else pc + 4


def make_cpu(image, base):
    c = MaterializationCPU(image, base)
    c.r[29] = STACK
    return c


def synthetic_cases(image, base):
    results = []
    # A nonzero row after a null IDX identity remains admitted; IDX null is skip,
    # whereas HED null is an early terminator. Identity2047 is included.
    for target in (0, 1, 15, 16, 78, 94, 123, 2047):
        for premise in ('match_first', 'match_later', 'unmatched', 'early_null', 'masked_match'):
            for initial in (0, 77, 0x8123):
                for seed in ((0, 1, 255, 256, 0x1234, 0xFFFF) if target == 0 and premise == 'match_first' else (0x1234,)):
                    c = make_cpu(image, base)
                    descriptor, header = 0x00010000, 0x00011000
                    c.put(IDX + target * 12 + 6, 2, initial)
                    c.put(IDX + target * 12 + 8, 4, descriptor)
                    c.put(descriptor + 24, 4, header)
                    c.put(header + 2, 2, seed)
                    slot = 0
                    admitted = premise not in ('unmatched', 'early_null')
                    if premise == 'match_first': c.put(HED + 4, 4, descriptor)
                    elif premise == 'match_later':
                        slot = 2
                        c.put(HED + 4, 4, descriptor + 0x100)
                        c.put(HED + 12, 4, descriptor + 0x200)
                        c.put(HED + 20, 4, descriptor)
                    elif premise == 'unmatched': c.put(HED + 4, 4, descriptor + 0x100)
                    elif premise == 'early_null': c.put(HED + 12, 4, descriptor)
                    else: c.put(HED + 4, 4, descriptor | 0x81000000)
                    c.run_region()
                    value = c.get(IDX + target * 12 + 6, 2)
                    assert value == (seed if admitted else initial)
                    assert c.get(IDX + target * 12 + 8, 4) == descriptor
                    assert c.get(IDX + target * 12, 4) == 0
                    assert c.get(IDX + target * 12 + 4, 2) == 0
                    assert len(c.writes) == int(admitted)
                    if admitted:
                        assert c.writes[0] == {'writer_pc': hex(WRITER), 'index': target,
                                              'field_offset': 6, 'width': 2, 'value': seed}
                    results.append({'target_index': target, 'premise': premise, 'initial_raw6': initial,
                                    'seed': seed, 'raw6_after': value, 'write_count': len(c.writes),
                                    'instructions_executed': c.steps})
    # The original HED bound is512 entries, including slot511.
    for slot in (511, 512):
        c = make_cpu(image, base)
        descriptor, header = 0x00010000, 0x00011000
        c.put(IDX + 8, 4, descriptor)
        c.put(IDX + 6, 2, 77)
        c.put(descriptor + 24, 4, header)
        c.put(header + 2, 2, 0x1234)
        for i in range(slot): c.put(HED + i * 8 + 4, 4, descriptor + 0x100)
        c.put(HED + slot * 8 + 4, 4, descriptor)
        c.run_region()
        value = c.get(IDX + 6, 2)
        assert value == (0x1234 if slot == 511 else 77)
        results.append({'target_index': 0, 'premise': 'hed_bound', 'hed_match_slot': slot,
                        'initial_raw6': 77, 'seed': 0x1234, 'raw6_after': value,
                        'write_count': len(c.writes), 'instructions_executed': c.steps})
    return results


def owner_stage(image, base, container):
    resources = list(iter_resources(container))
    layout = {'idx': 0x8015C000, 'mdt': 0x80164000, 'stg': 0x80194000, 'hed': 0x8013A000}
    selected = {}
    for kind, load in layout.items():
        matches = [r for r in resources if r['type'] == 0 and r['load'] == load]
        if len(matches) != 1: raise ValueError('stage canonical resource missing/ambiguous: ' + kind)
        selected[kind] = matches[0]
    c = make_cpu(image, base)
    for r in selected.values():
        for i, byte in enumerate(r['payload']): c.put(r['load'] + i, 1, byte)
    idx_count = len(selected['idx']['payload']) // 12
    assert len(selected['idx']['payload']) % 12 == 0 and idx_count <= 2048
    admitted = []
    hed_count = 0
    for slot in range(512):
        pointer = c.get(HED + slot * 8 + 4, 4)
        if not pointer: break
        hed_count += 1
    before = [c.get(IDX + i * 12 + 6, 2) for i in range(idx_count)]
    for index in range(idx_count):
        descriptor = c.get(IDX + index * 12 + 8, 4)
        if not descriptor: continue
        matches = [slot for slot in range(hed_count)
                   if c.get(HED + slot * 8 + 4, 4) & 0xFFFFFF == descriptor & 0xFFFFFF]
        if not matches: continue
        header = c.get(descriptor + 24, 4)
        seed = c.get(header + 2, 2)
        assert 0x80194000 <= header and header + 4 <= 0x80194000 + len(selected['stg']['payload'])
        admitted.append({'index': index, 'hed_match_slot': matches[0], 'static_raw6': before[index],
                         'initial_raw6': seed, 'descriptor_pointer': hex(descriptor),
                         'header_pointer': hex(header)})
    c.run_region()
    after = [c.get(IDX + i * 12 + 6, 2) for i in range(idx_count)]
    expected = before[:]
    for r in admitted: expected[r['index']] = r['initial_raw6']
    assert after == expected
    assert len(c.writes) == len(admitted)
    assert all(w['writer_pc'] == hex(WRITER) and w['field_offset'] == 6 and w['width'] == 2 for w in c.writes)
    unchanged = []
    for index in range(idx_count):
        start = index * 12
        source = selected['idx']['payload'][start:start + 12]
        assert c.get(IDX + start, 4) == int.from_bytes(source[:4], 'little')
        assert c.get(IDX + start + 4, 2) == int.from_bytes(source[4:6], 'little')
        assert c.get(IDX + start + 8, 4) == int.from_bytes(source[8:12], 'little')
        if after[index] == before[index]: unchanged.append(index)
    return {'container': container.name, 'idx_count': idx_count, 'hed_count_before_null': hed_count,
            'admitted_identity_count': len(admitted), 'write_count': len(c.writes),
            'instructions_executed': c.steps, 'initial_raw6_distribution': dict(Counter(r['initial_raw6'] for r in admitted)),
            'unchanged_identity_indices': unchanged,
            'all_xyz_descriptor_fields_preserved': True,
            'all_original_writes_match_derived_scalar_rule': True,
            'resources': {kind: {'load': hex(r['load']), 'bytes': len(r['payload']), 'sha256': sha(r['payload'])}
                          for kind, r in selected.items()},
            'captured_reference_identities': [r for r in admitted if r['index'] in (78, 94, 123)]}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--skip-synthetic', action='store_true')
    a = p.parse_args()
    if a.out.exists(): raise SystemExit('refusing to replace earlier source evidence')
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    stages = [owner_stage(image, base, a.dat / 'ST00.BIN')]
    synthetic = [] if a.skip_synthetic else synthetic_cases(image, base)
    result = {'schema': 'idx-materialization-source-probe/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'source_exe_sha256': SUPPORTED_EXE, 'routine': hex(START),
              'executed_region': [hex(START), hex(END)],
              'executed_region_sha256': sha(image[2048 + START - base:2048 + END - base]),
              'writer_pc': hex(WRITER), 'identity_limit': 2048, 'hed_search_limit': 512,
              'stages': stages, 'synthetic_case_count': len(synthetic), 'synthetic_cases': synthetic,
              'all_scalar_checks_passed': True,
              'limits': ['Original integer region with explicit static owner resource premises; not live trace timing.',
                         'Later variant routine0x8003AD14 remains a separate0/100/200 mutation route.',
                         'No native initialization, contact fallback, source asset modification or running emulator write.',
                         'Only ST00 owner canonical quartet measured here; other stages and resumed-state mutation remain unmeasured.',
                         'Cooked L3RG retains byte2/byte3 of descriptor primary-near render header; cooked L3MP drops HED admission.']}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'writer_pc': hex(WRITER), 'stages': stages,
                      'synthetic_case_count': len(synthetic), 'all_scalar_checks_passed': True}))


if __name__ == '__main__': main()

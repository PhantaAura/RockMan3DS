#!/usr/bin/env python3
"""Read owner data and emit selected chain metadata/hashes; never emit payload bytes.

This is an address-profile check for the supplied USA executable and ST00 overlay,
not a MIPS emulator or a proof that an actor has run. No external dependency.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'tools'))
from build_stage_catalog import iter_resources
from build_stage_object_catalog import parse_list

EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', required=True, type=Path)
    ap.add_argument('--dat', required=True, type=Path)
    ap.add_argument('--out', required=True, type=Path)
    a = ap.parse_args()
    exe = a.exe.read_bytes()
    st00 = a.dat / 'ST00.BIN'
    assert sha(exe) == EXE_SHA, 'unsupported executable profile'
    base = struct.unpack_from('<I', exe, 0x18)[0]
    overlay = next(r for r in iter_resources(st00)
                   if r['type'] == 0 and r['load'] == 0x80100000
                   and 'PROGBIN' in r['internal'].upper())
    data, obase = overlay['payload'], overlay['load']
    assert sha(data) == OVERLAY_SHA, 'unsupported ST00 overlay profile'

    def ou32(addr):
        return struct.unpack_from('<I', data, addr - obase)[0]

    def code_hash(start, end, image='overlay'):
        block = (data[start-obase:end-obase] if image == 'overlay'
                 else exe[2048+start-base:2048+end-base])
        assert len(block) == end-start
        return {'start': hex(start), 'end_exclusive': hex(end),
                'image': image, 'sha256': sha(block)}

    lists = [parse_list(data, obase, ou32(0x8011AB3C + 4*i)) for i in range(5)]
    assert [len(x) for x in lists] == [1, 16, 20, 11, 1]
    first, actor = lists[0][0], lists[2][18]
    assert tuple(first[:5]) == (1, 0xC0, 3, 0, 0)
    assert tuple(actor[:5]) == (1, 0x20, 0x35, 0, 0)
    # Assert only the selected entries, never serialize an original function table.
    selected = {
        0x8011A650: 0x80119398, 0x8011CF48: 0x801193D4,
        0x8011CF28: 0x8011CEC8, 0x8011A5A4: 0x8011A4A4,
        0x8011A4A4: 0x801068A8, 0x8011C020: 0x80106B10,
        0x801000D4: 0x80106B5C,
    }
    for addr, target in selected.items():
        assert ou32(addr) == target, f'changed dispatch entry {addr:#x}'

    ebds = {}
    for name in ('ST00_00', 'ST00_01', 'ST00_02'):
        cp = a.dat / (name + '.BIN')
        r = next(r for r in iter_resources(cp) if r['load'] == 0x8016C000)
        ebds[name] = r
    ebd = ebds['ST00_02']['payload']
    assert ebd == ebds['ST00_01']['payload']
    rows = [struct.unpack_from('<4B3I', ebd, 4+16*i)
            for i in range(struct.unpack_from('<I', ebd)[0])]
    matches = [(i, r) for i, r in enumerate(rows) if r[0:2] == (0x20, 0x35)]
    assert len(matches) == 1 and matches[0][0] == 3
    row = matches[0][1]
    first_ebd = ebds['ST00_00']['payload']
    assert not any(struct.unpack_from('<4B3I', first_ebd, 4+16*i)[:2] == (0xC0, 3)
                   for i in range(struct.unpack_from('<I', first_ebd)[0]))
    p0, p1, p2 = row[4:]
    offsets = [p-0x8016C000 for p in (p0, p1, p2)]
    assert offsets == [0x6AB4, 0x7224, 0x7528]

    script = ou32(0x8011CF28)
    first_index = struct.unpack_from('<H', data, script-obase)[0]
    next_index = struct.unpack_from('<H', data, script-obase+4)[0]
    result = {
        'schema': 'object-initializer-chain/1',
        'status': 'SOURCE_BACKED_PARTIAL',
        'method': 'Static selected-address trace plus hash-checked owner data; no actor execution claimed',
        'sources': {
            'executable': {'path': str(a.exe.resolve()), 'sha256': sha(exe)},
            'stage_container': {'path': str(st00.resolve()), 'sha256': sha(st00.read_bytes())},
            'overlay': {'load': hex(obase), 'bytes': len(data), 'sha256': sha(data)},
            'ebd': {'path': str((a.dat/'ST00_02.BIN').resolve()),
                    'load': '0x8016c000', 'bytes': len(ebd), 'sha256': sha(ebd),
                    'identical_to_ST00_01': True},
        },
        'first_imported_record': {
            'area': 0, 'index': 0, 'address': '0x8011a704',
            'id': first[0], 'class': '0xc0', 'subtype': 3, 'parameter_3': 0,
            'allocation': {'parser_call': '0x8002ed6c', 'allocator': '0x800316bc',
                           'pool': '0x800a2d08', 'slots': 16, 'stride': 36,
                           'slot': 0, 'condition': 'fresh cleared pool'},
            'fresh_seed': {'work_0': 1, 'work_1': 192, 'work_2': 3,
                           'work_3': 0, 'work_6': 3, 'work_8': 0},
            'update_gate': '(work[0] & work[6] & 1) != 0',
            'scratch_table': '0x1f800068', 'installed_table': '0x8011a644',
            'subtype_entry': '0x8011a650', 'callback': '0x80119398',
            'state_entry': '0x8011cf48', 'initializer': '0x801193d4',
            'selector': 'work[3] indexes u32 table 0x8011cf28',
            'selected_script': hex(script), 'script_step_helper': '0x8011974c',
            'command_base': '0x800ad168', 'command_index_u16_initial': first_index,
            'command_index_u16_after_first_step': next_index,
            'initial_command_pointer': hex(0x800AD168 + 2*first_index),
            'after_step_command_pointer': hex(0x800AD168 + 2*next_index),
            'initializer_calls_ebd': False, 'local_ebd_candidates': 0,
            'boundary': 'Script command storage/consumer semantics and any presentation consequences not decoded',
        },
        'first_unique_actor_record': {
            'label': 'actor record A', 'area': 2, 'index': 18,
            'address': '0x8011a9e8', 'id': actor[0], 'class': '0x20',
            'subtype': 53, 'parameter_3': 0, 'dispatch_byte_4': 0,
            'allocation': {'parser_call': '0x8002ec14', 'allocator': '0x80031418',
                           'pool': '0x8009f108', 'slots': 16, 'stride': 960,
                           'slot': 0, 'condition': 'fresh cleared pool'},
            'update_gate': '(work[0] & work[6] & 1) != 0',
            'scratch_table': '0x1f800058', 'installed_table': '0x8011a4d0',
            'subtype_entry': '0x8011a5a4', 'subtype_target_table': '0x8011a4a4',
            'byte4_entry': '0x8011a4a4', 'callback': '0x801068a8',
            'parameter3_entry': '0x8011c020', 'state_handler': '0x80106b10',
            'fresh_state_entry': '0x801000d4', 'fresh_state_block': '0x80106b5c',
            'initializer_call': '0x80106b5c', 'initializer': '0x80106974',
            'lookup_call': '0x801069a0', 'wrapper': '0x8002f9c4',
            'lookup': '0x8002fa38', 'variant_pointer': None,
            'selector_rule': 'first normalized-class/subtype row; variant ignored',
            'selected_row': 3, 'row_variant': row[2], 'row_extra': row[3],
            'row_resource_offsets': [hex(x) for x in offsets],
            'resource_assignments': [
                {'slot': 0, 'pointer': hex(p0), 'work_pointer_field': '0x88',
                 'additional_use': 'copy 24 bytes from pointer+0x10 into work+0xf8',
                 'consumed_window_offset': hex(offsets[0]+16), 'consumed_window_bytes': 24,
                 'consumed_window_sha256': sha(ebd[offsets[0]+16:offsets[0]+40])},
                {'slot': 1, 'pointer': hex(p1), 'work_pointer_field': '0xa0',
                 'additional_use': 'work+0x9c = u32(pointer); work+0xa0 = pointer+4',
                 'header_u32': struct.unpack_from('<I', ebd, offsets[1])[0],
                 'consumed_window_bytes': 4,
                 'consumed_window_sha256': sha(ebd[offsets[1]:offsets[1]+4])},
                {'slot': 2, 'pointer': hex(p2), 'work_pointer_field': '0x90',
                 'additional_use': 'pointer stored; payload format not decoded'},
            ],
            'return_handling': 'lookup result is not checked before remaining initializer stores',
            'boundary': 'Resource binary roles/decoder and native actor transform/animation/draw binding still unproved',
        },
        'code_regions': [
            code_hash(0x80100324, 0x8010038C),
            code_hash(0x8002E840, 0x8002E88C, 'exe'),
            code_hash(0x8002E980, 0x8002E9CC, 'exe'),
            code_hash(0x80016FD4, 0x80017094, 'exe'),
            code_hash(0x80017478, 0x80017528, 'exe'),
            code_hash(0x80031418, 0x80031498, 'exe'),
            code_hash(0x800316BC, 0x80031734, 'exe'),
            code_hash(0x80119398, 0x80119470),
            code_hash(0x8011974C, 0x80119800),
            code_hash(0x801068A8, 0x80106B10),
            code_hash(0x80106B10, 0x80106B74),
            code_hash(0x8002F9C4, 0x8002F9EC, 'exe'),
            code_hash(0x8002FA38, 0x8002FBF4, 'exe'),
        ],
        'limits': ['static trace only; emulator remains paused before first encounter',
                   'fresh slots depend on empty pools, not asserted current live allocation',
                   '0x80089a78 -> 0x80047318 is not the active ST00 subtype table proved here',
                   'no AI, naming, model format, full initializer translation or draw parity claimed'],
        'payload_policy': 'Selected addresses, scalar selectors, hashes and derived behavior only; no bytes, disassembly or original tables emitted',
    }
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2)+'\n')
    print('verified first C0 script chain and separate class20 actor resource chain; metadata only')


if __name__ == '__main__':
    main()

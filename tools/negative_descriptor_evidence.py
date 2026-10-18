#!/usr/bin/env python3
"""Measure owner-supplied negative terrain binding without emitting game bytes.

The bounded CPU evaluator checks only the original descriptor-selection blocks
with synthetic pointer-table contents. It does not execute the game or prove
gameplay/rendering parity. JSON contains hashes, addresses and aggregates only.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct

from build_stage_catalog import iter_resources

EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
ST00_SHA = '3f46c6bac9261ee1def37bddd8a91377343e5ee1235391b2ed3a18f2cbcc8dc0'
STG_BASE = 0x80194000


def sha(data):
    return hashlib.sha256(data).hexdigest()


def signed16(value):
    return value - 65536 if value & 0x8000 else value


def selection_probe(image):
    base, size = struct.unpack_from('<2I', image, 24)
    details = []
    for start, end in ((0x800219bc, 0x800219ec), (0x80022270, 0x800222a0)):
        if not base <= start < end <= base + size:
            raise ValueError('selection block outside executable')
        passed = 0
        for packed in range(0x8000, 0x10000):
            registers = [0] * 32
            registers[3] = registers[6] = registers[8] = packed
            registers[29] = 0x100000
            slot = (packed << 1) & 0xf8
            table_address = 0x80139f04 + slot
            synthetic_pointer = 0x70000000 + slot
            pc, pending, steps = start, None, 0
            while pc < end:
                if not start <= pc < end or pc & 3 or steps >= 32:
                    raise ValueError('selection block escaped bounded execution')
                previous_target, pending = pending, None
                word = struct.unpack_from('<I', image, pc - base + 2048)[0]
                op, rs, rt = word >> 26, (word >> 21) & 31, (word >> 16) & 31
                imm = word & 0xffff
                dest, value = rt, None
                if op == 0 and word & 63 == 0:
                    dest = (word >> 11) & 31
                    value = registers[rt] << ((word >> 6) & 31)
                elif op == 0 and word & 63 == 33:
                    dest = (word >> 11) & 31
                    value = registers[rs] + registers[rt]
                elif op == 15:
                    value = imm << 16
                elif op == 9:
                    value = registers[rs] + signed16(imm)
                elif op == 12:
                    value = registers[rs] & imm
                elif op == 13:
                    value = registers[rs] | imm
                elif op == 35:
                    address = (registers[rs] + signed16(imm)) & 0xffffffff
                    if address != table_address:
                        raise ValueError('selection reads unexpected synthetic table address')
                    value = synthetic_pointer
                elif op == 1 and rt == 1:
                    if registers[rs] < 0x80000000:
                        pending = pc + 4 + signed16(imm) * 4
                elif op == 2:
                    pending = ((pc + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2)
                else:
                    raise ValueError(f'unsupported selection operation at {pc:x}')
                if value is not None and dest:
                    registers[dest] = value & 0xffffffff
                registers[0] = 0
                pc = previous_target if previous_target is not None else pc + 4
                steps += 1
            expected = synthetic_pointer if packed & 0x80 else STG_BASE
            if registers[4] != expected:
                raise ValueError('original descriptor selection disagrees with packed-bit binding')
            passed += 1
        details.append({'start': hex(start), 'end_exclusive': hex(end),
                        'block_sha256': sha(image[start - base + 2048:end - base + 2048]),
                        'negative_word_cases_passed': passed})
    return {'method': 'Bounded original selection blocks, every negative 16-bit word, synthetic special pointer table, branch delay slots honored',
            'status': 'SOURCE_BACKED_PARTIAL', 'cases_passed': sum(x['negative_word_cases_passed'] for x in details),
            'blocks': details, 'limitation': 'Descriptor selection only; synthetic special pointers do not establish actual table population or retail parity.'}


def measurements(container):
    resources = {r['internal'].upper().rsplit('.', 1)[-1]: r
                 for r in iter_resources(container) if r['type'] == 0
                 and r['internal'].upper().endswith(('.MDT', '.IDX', '.STG', '.HED'))}
    if set(resources) != {'MDT', 'IDX', 'STG', 'HED'}:
        raise ValueError('expected supplied canonical stage quartet')
    stg, mdt, idx, hed = [resources[k]['payload'] for k in ('STG', 'MDT', 'IDX', 'HED')]
    if resources['STG']['load'] != STG_BASE or len(stg) < 48:
        raise ValueError('default descriptor load layout unavailable')
    refs = {struct.unpack_from('<I', hed, o + 4)[0] for o in range(0, len(hed), 8)}
    refs.update(struct.unpack_from('<I', idx, o + 8)[0] for o in range(0, len(idx), 12))
    slots = []
    for slot, pointer in enumerate(struct.unpack_from('<6I', stg, 24)):
        record_offset = pointer - STG_BASE
        if not 0 <= record_offset <= len(stg) - 4:
            raise ValueError('default render record outside supplied STG')
        materials, flags = struct.unpack_from('<2B', stg, record_offset)
        streams = flags & 63
        record_bytes = 4 + materials * 4 + streams * 8
        if record_offset + record_bytes > len(stg):
            raise ValueError('default render record exceeds supplied STG')
        count = 0
        for stream in range(streams):
            uv, geom = struct.unpack_from('<II', stg, record_offset + 4 + materials * 4 + stream * 8)
            if uv == geom == 0:
                continue
            n = geom >> 24
            up, gp = ((uv & 0xffffff) | 0x80000000) - STG_BASE, ((geom & 0xffffff) | 0x80000000) - STG_BASE
            if not 0 <= up <= len(stg) - n * 8 or not 0 <= gp <= len(stg) - n * 16:
                raise ValueError('default primitive range outside supplied STG')
            if (uv >> 24) & 15 >= materials:
                raise ValueError('default primitive material slot unavailable')
            count += n
        slots.append({'descriptor_slot': slot, 'record_address': hex(pointer),
                      'record_sha256': sha(stg[record_offset:record_offset + record_bytes]),
                      'material_count': materials, 'stream_count': streams, 'primitive_count': count})
    areas = []
    for area in range(struct.unpack_from('<I', mdt)[0]):
        _, _, width, height, pointer = struct.unpack_from('<4BI', mdt, 4 + area * 8)
        cells = struct.unpack_from('<' + str(width * height) + 'h', mdt, pointer - resources['MDT']['load'])
        negative = [cell & 0xffff for cell in cells if cell < 0]
        areas.append({'neutral_area': f'AREA_{area}', 'negative_cells': len(negative),
                      'default_negative_cells': sum(not cell & 0x80 for cell in negative),
                      'special_negative_cells': sum(bool(cell & 0x80) for cell in negative),
                      'quarter_turn_counts': dict(sorted(Counter(str(cell & 3) for cell in negative).items()))})
    return {'resources': {k: {'sha256': sha(r['payload']), 'bytes': len(r['payload']), 'load_address': hex(r['load'])}
                          for k, r in resources.items()},
            'default_descriptor_address': hex(STG_BASE), 'default_descriptor_sha256': sha(stg[:48]),
            'default_descriptor_in_inherited_HED_IDX_references': STG_BASE in refs,
            'default_render_slots': slots, 'areas': areas}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--container', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image, container_bytes = args.exe.read_bytes(), args.container.read_bytes()
    if sha(image) != EXE_SHA or sha(container_bytes) != ST00_SHA:
        raise ValueError('supplied USA executable or first-stage input identity differs from established evidence')
    result = {'schema': 'negative-default-source-evidence/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'executable_sha256': sha(image), 'container_sha256': sha(container_bytes),
              'selection_probe': selection_probe(image), 'measurements': measurements(args.container),
              'findings': [
                  {'id': 'DEFAULT_NEGATIVE_STG_BINDING', 'status': 'SOURCE_BACKED_PARTIAL',
                   'meaning': 'For negative cells with packed bit7 clear, both inspected source paths select the STG load base directly. This descriptor is absent from HED/IDX reference collection. Slots0..2 select its primary near/mid/far render records.'},
                  {'id': 'DEFAULT_STG_OWNER_DATA_ORIGIN', 'status': 'SOURCE_BACKED_PARTIAL',
                   'meaning': 'The supplied container type0 STG resource header loads its payload at0x80194000. The first48 payload bytes form the selected descriptor, and all its render pointers and streams are within that same supplied STG payload. No executable geometry is substituted.'}],
              'unknowns': ['Actual special bit7 pointer-table population and lifetime', 'Full retail visibility, alpha and subdivision behavior', 'Physical Old3DS rendering parity'],
              'payload_policy': 'Only hashes, addresses, semantics and aggregate counts; no game byte payloads or disassembly.'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8') as output:
        json.dump(result, output, indent=2)
        output.write('\n')
    print(json.dumps({'output': str(args.output), 'status': result['status'],
                      'selection_cases_passed': result['selection_probe']['cases_passed']}))


if __name__ == '__main__':
    main()

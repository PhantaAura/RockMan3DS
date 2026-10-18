#!/usr/bin/env python3
"""Read-only HEAD UV reset oracle; exports counts/hashes, never asset payloads.

The supplied HEAD container is read in memory only, not extracted or recooked.
The existing cooked head is compared with the original helper's baseline words.
Authored packet buffers prove both double-buffer copies without helper hooks.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01422')]
from cook_actor_combat import OwnerExe, OWNER_SHA
from class60_06_source_probe import FastProbe

HEAD_SHA = '6852fa2944634ecc582cf454e8595647287b9ed454747b89de95d2d99a452a04'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def word(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def copy_fields(cpu, header):
    counts = cpu.get(header, 4)
    triangles, quads = counts & 255, counts >> 8 & 255
    displacement = triangles * 68 + quads * 84
    fields = []
    for count, pointer, stride, offsets in (
            (triangles, cpu.get(header + 4, 4), 68, (12, 20, 28)),
            (quads, cpu.get(header + 8, 4), 84, (12, 20, 28, 36))):
        for face in range(count):
            for buffer in range(2):
                for offset in offsets:
                    target = pointer + face * stride + buffer * ((stride - 4) // 2) + offset
                    fields.append((target, cpu.get(target + displacement, 4)))
    return fields


def run(cpu):
    cpu.r[4], cpu.r[5], cpu.r[6] = 0, 0, 0
    cpu.r[29], cpu.r[31] = 0x700000, 0
    cpu.run(0x8002d004)
    if cpu.return_only_hooks:
        raise ValueError('An original helper was replaced')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'head-container', 'cooked-head', 'out'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    if output.exists() or output == SOURCE or SOURCE in output.parents:
        raise ValueError('Choose a new private output outside source')
    image = args.exe.read_bytes()
    owner = OwnerExe(image)
    container = args.head_container.read_bytes()
    length = word(container, 4)
    if word(container, 0) != 0 or word(container, 12) != 0x800f7800:
        raise ValueError('Unsupported already supplied head resource')
    head = container[2048:2048 + length]
    if len(head) != length or digest(head) != HEAD_SHA:
        raise ValueError('Unsupported HEAD resource identity')
    cooked = args.cooked_head.read_bytes()
    if cooked[:4] != b'L3MD' or struct.unpack_from('<HH', cooked, 4) != (1, 64):
        raise ValueError('Unsupported existing cooked head')
    if word(cooked, 12) != 4 or word(cooked, 44) != len(cooked):
        raise ValueError('Cooked head coverage mismatch')
    cpu = FastProbe(image, owner.base, b'')
    for index, value in enumerate(head):
        cpu.put(0x800f7800 + index, 1, value)
    selected = []
    for part_index in (2, 3):
        nt, nq, _, part_id = head[part_index * 16:part_index * 16 + 4]
        table = word(cooked, 28) + part_index * 32
        tf, tc, qf, qc = struct.unpack_from('<IIII', cooked, table + 12)
        if (cooked[table], tc, qc) != (part_id, nt, nq):
            raise ValueError('Cooked head part identity/count mismatch')
        for count, source_pointer, stride, first, section, record, uv_offset, material_offset in (
                (nt, word(head, part_index * 16 + 4), 68, tf, word(cooked, 36), 20, 6, 12),
                (nq, word(head, part_index * 16 + 8), 84, qf, word(cooked, 40), 24, 8, 16)):
            vertices = 3 if stride == 68 else 4
            for face in range(count):
                source = source_pointer + face * stride
                target = section + (first + face) * record
                for vertex in range(vertices):
                    if cooked[target + uv_offset + vertex * 2:target + uv_offset + vertex * 2 + 2] != bytes((cpu.get(source + 12 + vertex * 8, 1), cpu.get(source + 13 + vertex * 8, 1))):
                        raise ValueError('Cooked UV differs from original baseline')
                if struct.unpack_from('<HH', cooked, target + material_offset) != (cpu.get(source + 14, 2), cpu.get(source + 22, 2)):
                    raise ValueError('Cooked CLUT/tpage differs from original baseline')
        selected.append({'part_index': part_index, 'part_id': part_id,
                         'triangles': nt, 'quads': nq})
    fields = copy_fields(cpu, 0x800f7820) + copy_fields(cpu, 0x800f7830)
    if not all(cpu.get(target, 4) == value for target, value in fields):
        raise ValueError('Loaded head UV words differ from helper templates')
    run(cpu)
    if not all(cpu.get(target, 4) == value for target, value in fields):
        raise ValueError('Original reset does not match baseline')
    source_steps = cpu.steps
    authored_cases = authored_stores = authored_steps = 0
    for case, counts in enumerate(itertools.product(range(3), repeat=4)):
        probe = FastProbe(image, owner.base, b'')
        for part in range(2):
            header = 0x800f7820 + part * 16
            probe.put(header, 4, counts[part * 2] | counts[part * 2 + 1] << 8)
            for bank in range(2):
                base = 0x510000 + part * 0x40000 + bank * 0x20000
                probe.put(header + 4 + bank * 4, 4, base)
                for offset in range(2048):
                    probe.put(base + offset, 1, (case * 37 + offset * 17 + part * 13 + bank * 23) & 255)
        expected = copy_fields(probe, 0x800f7820) + copy_fields(probe, 0x800f7830)
        before = {target: probe.get(target, 4) for target, _ in expected}
        run(probe)
        if not all(probe.get(target, 4) == value for target, value in expected):
            raise ValueError('Authored original reset mismatch')
        authored_stores += sum(before[target] != value for target, value in expected)
        authored_steps += probe.steps
        authored_cases += 1
    result = {
        'schema': 'mml-source-head-uv-reset/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'source_helper': '8002D004', 'copy_helper': '8002D0D0',
        'source_exe_sha256': OWNER_SHA, 'head_resource_sha256': HEAD_SHA,
        'existing_cooked_head_sha256': digest(cooked),
        'selected_parts': selected, 'original_stores_checked': len(fields),
        'original_instructions': source_steps, 'authored_cases': authored_cases,
        'authored_changed_words': authored_stores, 'authored_instructions': authored_steps,
        'helper_hooks': 0, 'instruction_patches': 0,
        'cooked_uv_clut_tpage_match_original_baseline': True,
        'fresh_head_baseline_matches_original_templates': True,
        'asset_payloads_written': False, 'new_extraction_or_cooking': False,
        'retail_parity': False, 'physical_hardware_tested': False,
        'limits': ['Only the zero/zero expression reset used by event1 handoff is admitted.',
                   'Native restore copies represented UV/CLUT/tpage fields, not PS1 packet padding or OT words.',
                   'Live expression scheduling, transition loading and source RNG chronology remain separate owners.']}
    output.mkdir(parents=True)
    (output / 'source-head-uv-reset-proof.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()

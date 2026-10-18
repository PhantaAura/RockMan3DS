#!/usr/bin/env python3
"""Owner-local actor consumer proof; output metadata/hashes, never payload or code.

The small arithmetic evaluator only exercises the bounded unpack block against
synthetic input. It is neither a game emulator nor evidence of retail actor play.
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

EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'


def sha(b):
    return hashlib.sha256(b).hexdigest()


def s16(n):
    return n - 65536 if n & 32768 else n


def synthetic_unpack_check(word):
    """Interpret only 0x80033A68..0x80033D7C; enforce its instruction set/range."""
    channels = [(i * 61 + 2048) & 4095 for i in range(75)]
    packed = bytearray(128)
    for i, value in enumerate(channels):
        bit, byte = i * 12, i * 12 // 8
        value <<= bit % 8
        packed[byte] |= value & 255
        packed[byte + 1] |= value >> 8
    memory = {0x1F800180 + i: v for i, v in enumerate(packed)}
    sp, sentinel = 0x10000000, 0xDEADC0DE
    for i, b in enumerate(struct.pack('<I', sentinel)):
        memory[sp + 16 + i] = b
    regs = [0] * 32
    regs[29] = sp

    def read(a, size):
        if not (0x1F800120 <= a < 0x1F800200 or sp <= a <= sp + 20):
            raise ValueError('synthetic evaluator read outside bounded scratch/stack')
        return sum(memory.get(a + i, 0) << (8 * i) for i in range(size))

    def execute(pc, delay=False):
        w = word(pc)
        op, rs, rt, rd, shift, fn = w >> 26, w >> 21 & 31, w >> 16 & 31, w >> 11 & 31, w >> 6 & 31, w & 63
        imm = w & 65535
        signed = s16(imm)
        branch = None
        if op == 0:
            if fn == 0:
                regs[rd] = regs[rt] << shift
            elif fn == 2:
                regs[rd] = regs[rt] >> shift
            elif fn == 3:
                n = regs[rt] - (1 << 32) if regs[rt] & 0x80000000 else regs[rt]
                regs[rd] = n >> shift
            elif fn == 36:
                regs[rd] = regs[rs] & regs[rt]
            elif fn == 37:
                regs[rd] = regs[rs] | regs[rt]
            elif fn == 8:
                branch = regs[rs]
            else:
                raise ValueError('unexpected unpack arithmetic operation')
        elif op == 9:
            regs[rt] = regs[rs] + signed
        elif op == 12:
            regs[rt] = regs[rs] & imm
        elif op == 13:
            regs[rt] = regs[rs] | imm
        elif op == 15:
            regs[rt] = imm << 16
        elif op in (35, 36, 37):
            regs[rt] = read((regs[rs] + signed) & 0xFFFFFFFF, {35: 4, 36: 1, 37: 2}[op])
        elif op == 41:
            a = (regs[rs] + signed) & 0xFFFFFFFF
            if not 0x1F800120 <= a < 0x1F8001E8:
                raise ValueError('synthetic evaluator write outside destination vectors')
            memory[a], memory[a + 1] = regs[rt] & 255, regs[rt] >> 8 & 255
        elif op == 5:
            if regs[rs] != regs[rt]:
                branch = pc + 4 + signed * 4
        else:
            raise ValueError('unexpected unpack opcode')
        if delay and branch is not None:
            raise ValueError('unexpected nested delay transfer')
        for i in range(32):
            regs[i] &= 0xFFFFFFFF
        regs[0] = 0
        return branch

    pc, steps = 0x80033A68, 0
    while pc != sentinel:
        if not 0x80033A68 <= pc < 0x80033D80 or steps > 5000:
            raise ValueError('synthetic evaluator control flow escaped bounded unpack')
        branch = execute(pc)
        steps += 1
        if branch is not None:
            execute(pc + 4, delay=True)
            steps += 1
            pc = branch
        else:
            pc += 4
    measured = []
    for i in range(75):
        a = 0x1F800120 + i // 3 * 8 + i % 3 * 2
        measured.append(read(a, 2))
    expected = [((v - 4096) if v & 2048 else v) & 65535 if i < 3 else v
                for i, v in enumerate(channels)]
    if measured != expected:
        raise ValueError('retail unpack consumer did not match 75 contiguous channels')
    return {'synthetic_channels': 75, 'matched_fields': 75, 'steps': steps,
            'first_three_signed12': True, 'input_window_bytes': 128,
            'decoded_bits': 900, 'decoded_vectors': 25, 'rotation_vectors': 24,
            'destination_window_bytes': 198,
            'method': 'bounded original arithmetic on authored synthetic bytes'}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    a = p.parse_args()
    exe = a.exe.read_bytes()
    if sha(exe) != EXE_SHA:
        raise ValueError('unsupported executable profile')
    base = struct.unpack_from('<I', exe, 0x18)[0]

    def word(address):
        return struct.unpack_from('<I', exe, 2048 + address - base)[0]

    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN') if r['load'] == 0x80100000)
    if sha(overlay) != OVERLAY_SHA:
        raise ValueError('unsupported overlay profile')
    ebd = next(r['payload'] for r in iter_resources(a.dat / 'ST00_02.BIN') if r['load'] == 0x8016C000)
    row = struct.unpack_from('<4B3I', ebd, 4 + 3 * 16)
    if row[:4] != (0x20, 0x35, 0, 0):
        raise ValueError('selected actor row differs')
    p0, p1, p2 = [v - 0x8016C000 for v in row[4:]]
    descriptors = ebd[p0 + 16:p0 + 112]
    count = descriptors[1]
    tiers = []
    windows = [{'kind': 'work-descriptor-copy', 'offset': p0 + 16, 'bytes': 96,
                'sha256': sha(descriptors)}]
    for tier in range(3):
        m = struct.unpack_from('<I', ebd, p0 + 112 + tier * 4)[0] - 0x8016C000
        vertices, triangles, quads, parts = 0, 0, 0, []
        for i in range(count):
            po = m + 20 + descriptors[i * 4] * 20
            nt, nq, nv, raw3, tp, qp, material, vp = struct.unpack_from('<4B4I', ebd, po)
            for kind, ptr, size in [('triangles', tp, nt * 12), ('quads', qp, nq * 12),
                                    ('vertices', vp, nv * 8), ('vertex-consumer', vp, (nv + 2) // 3 * 24)]:
                if size:
                    off = ptr - 0x8016C000
                    if off < 0 or off + size > len(ebd):
                        raise ValueError('count-sized source window outside payload')
                    windows.append({'kind': kind, 'tier': tier, 'part': i, 'offset': off,
                                    'bytes': size, 'sha256': sha(ebd[off:off + size])})
            for ptr, n, quad in [(tp, nt, False), (qp, nq, True)]:
                for j in range(n):
                    off = ptr - 0x8016C000 + j * 12
                    if any(v >= nv for v in ebd[off + 8:off + (12 if quad else 11)]):
                        raise ValueError('source packet vertex index outside logical count')
            vertices += nv; triangles += nt; quads += nq
            parts.append({'draw_slot': i, 'mesh_index': descriptors[i * 4],
                          'parent': -1 if i == 0 else descriptors[i * 4 + 1],
                          'rotation_translation_index': descriptors[i * 4 + 2],
                          'material_word': hex(material), 'source_part_byte3': raw3,
                          'vertices': nv, 'triangles': nt, 'quads': nq})
        tiers.append({'tier': tier, 'parts': parts, 'vertices': vertices, 'triangles': triangles, 'quads': quads})
    poses = []
    for request, record in [(0, 0), (2, 16)]:
        sequence = struct.unpack_from('<I', ebd, p2 + request * 4)[0] - 0x8016C000 + record * 8
        rec = ebd[sequence:sequence + 8]
        frames = struct.unpack_from('<I', ebd, p1 + 4 + request * 4)[0] - 0x8016C000
        pose = struct.unpack_from('<I', ebd, frames + rec[0] * 4)[0] - 0x8016C000
        if pose < 0 or pose + 128 > len(ebd):
            raise ValueError('selected pose consumer window outside payload')
        poses.append({'request': request, 'record': record, 'frame_index': rec[0],
                      'sequence_offset': sequence, 'sequence_bytes': 8,
                      'sequence_sha256': sha(rec), 'pose_offset': pose, 'pose_bytes': 128,
                      'pose_sha256': sha(ebd[pose:pose + 128]), 'retail_state_selection_verified': False})
    regions = [(0x80017684, 0x800176A4), (0x80028DE0, 0x80028F90),
               (0x8002831C, 0x800289A0), (0x800289A0, 0x80028DE0),
               (0x800322A8, 0x80032304), (0x8003346C, 0x800334C4),
               (0x80033598, 0x80033A28), (0x80033A28, 0x80033D80),
               (0x8007B5A8, 0x8007B834)]
    output = {'schema': 'actor-resource-consumer-evidence/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'source_sha256': sha(exe), 'overlay_sha256': sha(overlay), 'ebd_sha256': sha(ebd),
              'copy_helper': {'address': '0x80017684', 'count_unit': 'u32 words',
                              'prior_01413_24_byte_claim_corrected_to': 96},
              'row': 3, 'resource_offsets': [p0, p1, p2], 'draw_count': count,
              'mesh_tiers': tiers, 'bounded_windows': windows, 'pose_cases': poses,
              'synthetic_unpack': synthetic_unpack_check(word),
              'initializer_profile': {
                  'source_address': '0x80106974',
                  'admitted_selectors': {'class': '0x20', 'subtype': '0x35', 'parameter3': 0, 'dispatch4': 0},
                  'ebd_lookup': '0x8002F9C4; null variant selects first class/subtype match',
                  'scale_q12': [512, 512, 512],
                  'shade_rgb555': '0x4210', 'pre_gte_shade_rgb8': [128, 128, 128],
                  'floor_wrapper': '0x80048E6C',
                  'fresh_floor_query_bounds': [0, 0, 0, 0, 0, 0],
                  'fresh_floor_query_bounds_reason': 'work+0x78 remains null until after floor call; 0x8004B3C0 null branch clears six bounds',
                  'fresh_floor_query_delta_y': 0,
                  'sha256': sha(overlay[0x6974:0x6b10])},
              'code_region_hashes': [{'start': hex(x), 'end_exclusive': hex(y),
                                     'sha256': sha(exe[2048 + x - base:2048 + y - base])} for x, y in regions],
              'limits': ['static consumer proof; no actor gameplay execution',
                         'initial pose choice depends on progression flag/record fields not wired yet',
                         'source identity, AI, damage, audio and animation-request timing unassigned',
                         'native float matrices do not reproduce PS1 integer/GTE rounding'],
              'payload_policy': 'derived counts, addresses, window sizes and hashes only'}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(output, indent=2) + '\n')
    print('verified bounded mesh consumers and 75-channel synthetic unpack; no payload emitted')


if __name__ == '__main__':
    main()

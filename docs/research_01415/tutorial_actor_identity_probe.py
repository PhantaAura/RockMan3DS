#!/usr/bin/env python3
"""Bounded tutorial identity investigation; scalar/hash output, no retail payload.

Reads only already-extracted owner inputs. Synthetic work is not live game state.
"""
import argparse
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01414')]
from build_stage_catalog import iter_resources
from build_stage_object_catalog import parse_list
from player_contact_source_probe import CPU, SUPPORTED_EXE, s16, s32

OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'


def sha(data):
    return hashlib.sha256(data).hexdigest()


class OverlayProbe(CPU):
    """Extend inherited arithmetic probe with little-endian unaligned transfers.

    Patches exist only in this synthetic memory dictionary. Owner files are read.
    Code fetch and data loads are distinct; pointer/geometry data are never
    interpreted as instructions by the instrumentation.
    """
    def __init__(self, image, base, overlay):
        super().__init__(image, base)
        self.mem.update({0x80100000 + i: v for i, v in enumerate(overlay)})
        self.calls = []
        self.indirect = []
        self.return_only_hooks = []

    def hook(self, address, result=0):
        self.put(address, 4, (31 << 21) | 8)
        self.put(address + 4, 4, (9 << 26) | (2 << 16) | (result & 65535))
        self.return_only_hooks.append(hex(address))

    def run(self, start, end=None):
        pc, pending = start, None
        while pc and pc != end:
            if self.steps >= 10000:
                raise ValueError('CPU probe step bound exceeded at ' + hex(pc))
            w = self.get(pc, 4)
            op, rs, rt, rd, sh, fn = w >> 26, w >> 21 & 31, w >> 16 & 31, w >> 11 & 31, w >> 6 & 31, w & 63
            imm, si = w & 65535, s16(w & 65535)
            a, b = self.r[rs], self.r[rt]
            old, pending, value, dest = pending, None, None, rt
            if not w:
                pass
            elif op == 0:
                dest = rd
                if fn == 0: value = b << sh
                elif fn == 2: value = b >> sh
                elif fn == 3: value = s32(b) >> sh
                elif fn == 4: value = b << (a & 31)
                elif fn == 6: value = b >> (a & 31)
                elif fn == 7: value = s32(b) >> (a & 31)
                elif fn == 8: pending = a
                elif fn == 9:
                    pending, value = a, pc + 8
                    self.indirect.append({'site': hex(pc), 'target': hex(a)})
                elif fn == 16: value = self.hi
                elif fn == 18: value = self.lo
                elif fn in (24, 25):
                    product = s32(a) * s32(b) if fn == 24 else a * b
                    self.lo, self.hi = product & 0xFFFFFFFF, product >> 32 & 0xFFFFFFFF
                elif fn in (26, 27):
                    aa, bb = (s32(a), s32(b)) if fn == 26 else (a, b)
                    if not bb: raise ValueError('source division trap at ' + hex(pc))
                    q = abs(aa) // abs(bb)
                    if (aa < 0) != (bb < 0): q = -q
                    self.lo, self.hi = q & 0xFFFFFFFF, (aa - q * bb) & 0xFFFFFFFF
                elif fn in (32, 33): value = a + b
                elif fn in (34, 35): value = a - b
                elif fn == 36: value = a & b
                elif fn == 37: value = a | b
                elif fn == 38: value = a ^ b
                elif fn == 39: value = ~(a | b)
                elif fn == 42: value = int(s32(a) < s32(b))
                elif fn == 43: value = int(a < b)
                else: raise ValueError('unsupported integer operation at ' + hex(pc))
            elif op == 9: value = a + si
            elif op == 10: value = int(s32(a) < si)
            elif op == 11: value = int(a < (si & 0xFFFFFFFF))
            elif op == 12: value = a & imm
            elif op == 13: value = a | imm
            elif op == 14: value = a ^ imm
            elif op == 15: value = imm << 16
            elif op in (32, 33, 35, 36, 37):
                value = self.get((a + si) & 0xFFFFFFFF, {32: 1, 33: 2, 35: 4, 36: 1, 37: 2}[op], op in (32, 33))
            elif op in (40, 41, 43):
                self.put((a + si) & 0xFFFFFFFF, {40: 1, 41: 2, 43: 4}[op], b)
            elif op in (34, 38, 42, 46):
                target = (a + si) & 0xFFFFFFFF
                lane, aligned = target & 3, target & ~3
                prior = self.get(aligned, 4)
                if op == 34: value = (b & (0xFFFFFF >> (lane * 8))) | (prior << ((3 - lane) * 8))
                elif op == 38: value = (b & (0xFFFFFF00 << ((3 - lane) * 8))) | (prior >> (lane * 8))
                elif op == 42: self.put(aligned, 4, (prior & (0xFFFFFF00 << (lane * 8))) | (b >> ((3 - lane) * 8)))
                else: self.put(aligned, 4, (prior & (0xFFFFFF >> ((3 - lane) * 8))) | (b << (lane * 8)))
            elif op in (4, 5):
                if (a == b) == (op == 4): pending = pc + 4 + si * 4
            elif op in (6, 7):
                if (s32(a) <= 0) == (op == 6): pending = pc + 4 + si * 4
            elif op == 1 and rt in (0, 1):
                if (s32(a) < 0) == (rt == 0): pending = pc + 4 + si * 4
            elif op in (2, 3):
                pending = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                if op == 3:
                    self.r[31] = pc + 8
                    self.calls.append({'site': hex(pc), 'target': hex(pending)})
            else: raise ValueError('unsupported opcode at ' + hex(pc))
            if value is not None and dest: self.r[dest] = value & 0xFFFFFFFF
            self.r[0] = 0
            self.steps += 1
            pc = old if old is not None else pc + 4


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', required=True, type=Path)
    ap.add_argument('--dat', required=True, type=Path)
    ap.add_argument('--out', required=True, type=Path)
    a = ap.parse_args()
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    word = lambda address: struct.unpack_from('<I', overlay, address - 0x80100000)[0]
    areas = []
    for area in range(5):
        pointer = word(0x8011AB3C + 4 * area)
        rows = parse_list(overlay, 0x80100000, pointer)
        areas.append({'area': area, 'pointer': hex(pointer), 'count': len(rows),
                      'classes': dict(Counter(hex(row[1]) for row in rows)),
                      'class40_count': sum(row[1] == 0x40 for row in rows)})
    traces = []
    for subtype in (2, 4):
        entry = 0x8011A5B0 + subtype * 4
        dispatch_table = word(entry)
        callback = word(dispatch_table)
        cpu = OverlayProbe(image, base, overlay)
        cpu.r[4], cpu.r[29] = 0x1000, 0x8000
        cpu.put(0x1001, 1, 0x40)
        cpu.put(0x1002, 1, subtype)
        cpu.put(0x1006, 1, 3)
        cpu.hook(0x8007FE50)
        error = None
        try:
            cpu.run(callback, 0x8002F9C4)
        except Exception as ex:
            error = str(ex)
        traces.append({'class': '0x40', 'subtype': subtype,
                       'parameter3': 0, 'dispatch4': 0,
                       'subtype_entry': hex(entry), 'dispatch_table': hex(dispatch_table),
                       'callback': hex(callback), 'steps': cpu.steps,
                       'direct_calls': cpu.calls, 'indirect_calls': cpu.indirect,
                       'error': error, 'wrapper_args': [hex(v) for v in cpu.r[4:8]],
                       'return_only_hooks': cpu.return_only_hooks})
    result = {'schema': 'tutorial-actor-identity/1', 'status': 'INVESTIGATION_PARTIAL',
              'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
              'static_lists': areas, 'synthetic_dispatch_traces': traces,
              'class40_enemy_identity_proved': False,
              'limits': ['Class identity and EBD presence do not prove enemy role.',
                         'Synthetic fresh work is not an observed encounter.',
                         'No retail code, data, original function table or assets exported.']}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(traces, indent=2))


if __name__ == '__main__':
    main()

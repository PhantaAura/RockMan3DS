#!/usr/bin/env python3
"""Original normalization/interpolation differential with authored lookup.

No original lookup records, code or assets are exported. Only authored scalar
contexts and outputs are serialized. Restricted integer/GTE interpreter models
only the exact command fields reached by these source helpers.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path
SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01419'))
from actor_detection_source_probe import DetectionProbe, SUPPORTED_EXE, s16


def signed32(x):
    x &= 0xffffffff
    return x - 0x100000000 if x & 0x80000000 else x


class MathProbe(DetectionProbe):
    def cop2(self, w):
        rs, rt, rd = w >> 21 & 31, w >> 16 & 31, w >> 11 & 31
        if rs == 4:
            value = self.r[rt]
            if rd in (8, 9, 10, 11): value = s16(value) & 0xffffffff
            self.data2[rd] = value
            if rd == 30:
                leading = 0
                bits = value if value & 0x80000000 == 0 else (~value & 0xffffffff)
                while leading < 32 and bits & 0x80000000 == 0:
                    leading += 1; bits = bits << 1 & 0xffffffff
                self.data2[31] = leading
            return
        if rs < 16: return super().cop2(w)
        fn, sf = w & 63, w >> 19 & 1
        ir = [s16(self.data2[9 + i]) for i in range(3)]
        if fn == 0x28:
            assert sf == 0
            macs = [value * value for value in ir]
        elif fn == 0x3d:
            assert sf == 0
            factor = s16(self.data2[8]); macs = [factor * value for value in ir]
        elif fn == 0x0c:
            assert sf == 1
            diagonal = [s16(self.control2[i]) for i in (0, 2, 4)]
            macs = [(diagonal[1] * ir[2] - diagonal[2] * ir[1]) >> 12,
                    (diagonal[2] * ir[0] - diagonal[0] * ir[2]) >> 12,
                    (diagonal[0] * ir[1] - diagonal[1] * ir[0]) >> 12]
        elif fn == 0x12:
            mx, v, cv = w >> 17 & 3, w >> 15 & 3, w >> 13 & 3
            assert (sf, mx, v, cv) in ((1, 0, 0, 3), (1, 0, 0, 0))
            matrix = []
            for packed in self.control2[:5]: matrix.extend((s16(packed), s16(packed >> 16)))
            vector = [s16(self.data2[0]), s16(self.data2[0] >> 16), s16(self.data2[1])]
            translation = [0, 0, 0] if cv == 3 else [signed32(v) for v in self.control2[5:8]]
            macs = [(sum(matrix[i * 3 + j] * vector[j] for j in range(3)) + translation[i] * 4096) >> 12 for i in range(3)]
        else: raise ValueError('Unproved GTE helper command ' + hex(fn))
        lm = w >> 10 & 1
        for i, mac in enumerate(macs):
            assert -0x80000000 <= mac < 0x80000000
            self.data2[25 + i] = mac & 0xffffffff
            self.data2[9 + i] = max(0 if lm else -32768, min(32767, mac)) & 0xffffffff
        self.control2[31] = 0
        self.gte_commands += 1


# Originalnormalizer ADDI and opcode50 IRloads are reached here; preserve the
# inherited probe dispatch and specialize only these authored interpreter lines.
import inspect
import textwrap
from tutorial_actor_identity_probe import OverlayProbe
run_source = textwrap.dedent(inspect.getsource(OverlayProbe.run))
needle = 'elif op in (34, 38, 42, 46):'
replacement = '''elif op == 18:
    self.cop2(w)
elif op == 50:
    self.data2[rt] = self.get((a + si) & 0xffffffff, 4)
    if rt in (8, 9, 10, 11): self.data2[rt] = s16(self.data2[rt]) & 0xffffffff
elif op == 58:
    self.put((a + si) & 0xffffffff, 4, self.data2[rt])
''' + needle
run_source = run_source.replace(needle, textwrap.indent(replacement, '        ').lstrip(), 1)
run_source = run_source.replace('elif op == 9:', 'elif op in (8, 9):')
namespace = {}; exec(run_source, OverlayProbe.run.__globals__, namespace)
MathProbe.run = namespace['run']


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    image = a.exe.read_bytes(); assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    if a.out.exists(): raise SystemExit('Refusing to overwrite math evidence')
    a.out.mkdir(parents=True)
    normalized, interpolated, steps, commands = [], [], 0, 0

    def fresh(factor):
        c = MathProbe(image, base); c.r[29] = 0x700000; c.r[28] = 0x80097864
        if factor is not None:
            for i in range(192): c.put(0x80092d38 + i * 2, 2, factor)
        return c

    vectors = [(0, 0, 0), (1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, 0, -1),
               (100, -96, -200), (26750, 26750, 26750), (-26750, 26750, -26750),
               (32767, 0, 0), (-32768, 0, 0), (65536, 1, -65536),
               (2147483647, -2147483648, 65535)]
    vectors += [(x, y, z) for x, y, z in itertools.product((-16000, -4096, 0, 4096, 16000), repeat=3)]
    for factor, vector in itertools.product((0, 1, 4096, 16384, 32767, -1, -32768), vectors):
        c = fresh(factor); c.r[4], c.r[5] = 0x1000, 0x2000
        for i, value in enumerate(vector): c.put(0x1000 + i * 4, 4, value)
        c.run(0x8007a694)
        out = [c.get(0x2000 + i * 2, 2, True) for i in range(3)]
        normalized.append([factor] + list(vector) + out)
        steps += c.steps; commands += c.gte_commands
    nvalues = [(0, 0, -4096), (4096, 0, 0), (0, 4096, 0), (100, -200, -4000),
               (-4096, 0, 0), (0, 0, 0), (32767, 0, 0)]
    pvalues = [(0, 0, -4096), (0, 0, 4096), (100, -96, -200), (4096, 0, 0),
               (-4096, 0, 0), (0, 0, 0), (0, -26750, -26750)]
    for factor, n, previous in itertools.product((0, 4096, 16384, 32767), nvalues, pvalues):
        c = fresh(factor); c.r[4], c.r[5], c.r[6], c.r[7] = 0x3000, 0x1000, 0x2000, 357
        c.put(c.r[29] + 16, 4, 0x0ff069da)
        for i, value in enumerate(n): c.put(0x1000 + i * 2, 2, value)
        for i, value in enumerate(previous): c.put(0x2000 + i * 2, 2, value)
        c.run(0x80033f04)
        out = [c.get(0x3000 + i * 2, 2, True) for i in range(3)]
        interpolated.append([factor] + list(n) + list(previous) + out)
        steps += c.steps; commands += c.gte_commands
    (a.out / 'authored-normalization.tsv').write_text(''.join('\t'.join(map(str, row)) + '\n' for row in normalized))
    (a.out / 'authored-interpolation.tsv').write_text(''.join('\t'.join(map(str, row)) + '\n' for row in interpolated))
    owner_normalized, owner_interpolated = [], []
    for vector in vectors:
        c = fresh(None); c.r[4], c.r[5] = 0x1000, 0x2000
        for i, value in enumerate(vector): c.put(0x1000 + i * 4, 4, value)
        c.run(0x8007a694)
        out = [c.get(0x2000 + i * 2, 2, True) for i in range(3)]
        owner_normalized.append(list(vector) + out)
        steps += c.steps; commands += c.gte_commands
    for n, previous in itertools.product([tuple(row[3:]) for row in owner_normalized], pvalues):
        c = fresh(None); c.r[4], c.r[5], c.r[6], c.r[7] = 0x3000, 0x1000, 0x2000, 357
        c.put(c.r[29] + 16, 4, 0x0ff069da)
        for i, value in enumerate(n): c.put(0x1000 + i * 2, 2, value)
        for i, value in enumerate(previous): c.put(0x2000 + i * 2, 2, value)
        c.run(0x80033f04)
        out = [c.get(0x3000 + i * 2, 2, True) for i in range(3)]
        owner_interpolated.append(list(n) + list(previous) + out)
        steps += c.steps; commands += c.gte_commands
    # Original table stays in source memory. These owner-local files contain
    # authored vectors and compared scalar outputs, not copied table records.
    (a.out / 'owner-normalization.tsv').write_text(''.join('\t'.join(map(str, row)) + '\n' for row in owner_normalized))
    (a.out / 'owner-interpolation.tsv').write_text(''.join('\t'.join(map(str, row)) + '\n' for row in owner_interpolated))
    result = {'schema': 'actor-detection-math-source-proof/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'exe_sha256': SUPPORTED_EXE, 'normalization_cases': len(normalized), 'interpolation_cases': len(interpolated),
        'owner_normalization_cases': len(owner_normalized), 'owner_interpolation_cases': len(owner_interpolated),
        'instructions_executed': steps, 'restricted_gte_commands': commands,
        'lookup': {'source_base': '0x80092D38', 'records': 192, 'record_bytes': 2, 'package': 'L3NRv1'},
        'original_table_exported': False, 'retail_parity': False,
        'limits': ['Fixtures overwrite reciprocal-sqrt lookup with authored uniform scalar records.',
                   'Supported GTE commands are SQR, GPF, OP and MVMVA with original command fields.',
                   'General flags/projection and overflow-trap domains remain outside this proof.']}
    (a.out / 'source-evidence.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))

if __name__ == '__main__': main()

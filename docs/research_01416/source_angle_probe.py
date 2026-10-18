#!/usr/bin/env python3
"""Execute original owner angle code; export scalar cases/hashes, never table data."""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE

TABLE = 0x80092b64


def digest(data):
    return hashlib.sha256(data).hexdigest()


def synthetic_steps(kind):
    if kind == 0:
        return [0] * 12
    if kind == 1:
        return list(range(1, 13))
    if kind == 2:
        return [(i + 1) * (-1 if i & 1 else 1) for i in range(12)]
    if kind == 3:
        return [((i + 1) * 0x13579bdf) & 0xffffffff for i in range(12)]
    raise ValueError('unknown synthetic table')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    image = a.exe.read_bytes()
    if digest(image) != SUPPORTED_EXE:
        raise SystemExit('Unsupported owner executable')
    if a.out.exists():
        raise SystemExit('Refusing to overwrite angle evidence')
    a.out.mkdir(parents=True)
    base = struct.unpack_from('<I', image, 24)[0]
    cases = {False: [], True: []}
    instructions = 0
    for kind in [-1, 0, 1, 2, 3]:
        for ratio in [-2147483648, -2147483647, -1000000000, -65536, -8192,
                      -4097, -4096, -4095, -17, -1, 0, 1, 17, 4095, 4096,
                      4097, 8192, 65536, 1000000000, 2147483646, 2147483647]:
            c = OverlayProbe(image, base, b'')
            c.r[4], c.r[29] = ratio & 0xffffffff, 0x8000
            if kind >= 0:
                for i, value in enumerate(synthetic_steps(kind)):
                    c.put(TABLE + 4 * i, 4, value)
            c.run(0x8007a414)
            cases[kind < 0].append([0, kind, 0, 0, 0, 0, ratio, c.r[2]])
            instructions += c.steps
        origins = (-32768, 0, 32767)
        targets = (-32768, -32767, -1, 0, 1, 32766, 32767)
        for ax, az, tx, tz in itertools.product(origins, origins, targets, targets):
            c = OverlayProbe(image, base, b'')
            c.r[4], c.r[5], c.r[29] = 0x1000, 0x1100, 0x8000
            for pointer, x, z in [(0x1000, ax, az), (0x1100, tx, tz)]:
                c.put(pointer, 2, x)
                c.put(pointer + 4, 2, z)
            if kind >= 0:
                for i, value in enumerate(synthetic_steps(kind)):
                    c.put(TABLE + 4 * i, 4, value)
            c.run(0x80031198)
            cases[kind < 0].append([1, kind, ax, az, tx, tz, 0, c.r[2]])
            instructions += c.steps
    columns = 'operation\ttable_kind\torigin_x\torigin_z\ttarget_x\ttarget_z\tratio\tresult_bits\n'
    for retail, rows in cases.items():
        name = 'source-angle-owner-cases.tsv' if retail else 'source-angle-synthetic-cases.tsv'
        f = a.out / name
        f.write_text(columns + ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    def region(start, end):
        return {'start': hex(start), 'end_exclusive': hex(end),
                'sha256': digest(image[start-base+0x800:end-base+0x800])}
    result = {'schema': 'source-angle-evidence/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': digest(image), 'hooks': [],
              'regions': [region(0x80031198, 0x80031268), region(0x8007a414, 0x8007a4e8)],
              'owner_table': {'address': hex(TABLE), 'words': 12,
                              'sha256': digest(image[TABLE-base+0x800:TABLE-base+0x800+48]),
                              'payload_exported': False},
              'synthetic_cases': len(cases[False]), 'owner_cases': len(cases[True]),
              'instructions_executed': instructions,
              'synthetic_fixture_sha256': digest((a.out/'source-angle-synthetic-cases.tsv').read_bytes()),
              'owner_fixture_sha256': digest((a.out/'source-angle-owner-cases.tsv').read_bytes()),
              'synthetic_overrides': 'Only12 angle-step words in the isolated Python memory dictionary; four explicitly authored tables.',
              'source_inputs': 'signed16 X/Z raw coordinates, including subtraction and SLL wrapping; original catan table for owner cases.',
              'limits': ['No table payload, code bytes or game assets exported.',
                         'Synthetic original-code execution; not live retail trace or full AI parity.',
                         'Source helper input-table residency/profile/cooking remains external.',
                         'No owner file or original emulator RAM is written.']}
    (a.out/'source-angle-evidence.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: result[k] for k in ['synthetic_cases','owner_cases','instructions_executed']}, indent=2))


if __name__ == '__main__':
    main()

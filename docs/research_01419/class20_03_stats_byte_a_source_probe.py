#!/usr/bin/env python3
"""Read-only fresh class20/03 byteA identity check; values stay owner-local.

Default output contains hashes/addresses/invariance, not original table values.
Optional --owner-local-values adds a private sidecar; never place it in source.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--owner-local-values', type=Path)
    args = p.parse_args()
    if args.out.exists() or (args.owner_local_values and args.owner_local_values.exists()):
        raise SystemExit('Refusing to overwrite byteA evidence')
    image = args.exe.read_bytes()
    assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    c = OverlayProbe(image, base, b'')
    rows, values = [], []
    for table in (0x8008edc4, 0x8008f060):
        pointer = c.get(table + 3 * 4, 4)
        for variant in (0, 1):
            record = pointer + variant * 16
            value = c.get(record + 10, 1)
            rows.append({'candidate_table': hex(table), 'subtype_index': 3,
                         'record_pointer': hex(pointer), 'variant_32e': variant,
                         'record': hex(record), 'offset': '0x0a',
                         'record_sha256': hashlib.sha256(bytes(c.get(record + i, 1) for i in range(16))).hexdigest()})
            values.append(value)
    result = {'schema': 'class20-03-combat-byte-a-invariance/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': SUPPORTED_EXE, 'source_consumer': '0x80102d10..0x80102d3c',
              'rows': rows, 'candidate_contexts': len(rows),
              'all_candidate_contexts_equal': len(set(values)) == 1,
              'same_variant_cross_table_equal': [values[0] == values[2], values[1] == values[3]],
              'selected_context_owned': False,
              'limits': ['Only the two established statistics-table candidates and initializer variants0/1.',
                         'Actual80098844 selector ownership/timing is not established here.',
                         'A source-byteA difference prohibits an invariant native default.',
                         'No record bytes or byteA values in this source-safe evidence.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    if args.owner_local_values:
        args.owner_local_values.parent.mkdir(parents=True, exist_ok=True)
        args.owner_local_values.write_text(json.dumps({'schema': 'owner-local-class20-03-byte-a-values/1',
            'rows': [dict(row, byte_a=value) for row, value in zip(rows, values)]}, indent=2) + '\n')
    print(json.dumps({'candidate_contexts': len(rows), 'all_equal': result['all_candidate_contexts_equal'],
                      'same_variant_cross_table_equal': result['same_variant_cross_table_equal']}, indent=2))


if __name__ == '__main__':
    main()

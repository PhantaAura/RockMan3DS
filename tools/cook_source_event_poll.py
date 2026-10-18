#!/usr/bin/env python3
"""Cook the hash-guarded owner's ST00 flag/event poll table into neutral L3EP.

No executable code, actor assets or saves are emitted. The cooked owner-derived
table belongs only in the private local pack, never a code-only source release.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

from build_stage_catalog import iter_resources

OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
TABLE_OFFSET = 0x1ADF8
MAXIMUM_RECORDS = 256


def cook(overlay):
    if hashlib.sha256(overlay).hexdigest() != OVERLAY_SHA:
        raise ValueError('Unsupported owner ST00 overlay identity')
    records = bytearray()
    for index in range(MAXIMUM_RECORDS + 1):
        offset = TABLE_OFFSET + index * 4
        if offset + 4 > len(overlay):
            raise ValueError('Truncated source event poll table')
        flag = struct.unpack_from('<H', overlay, offset)[0]
        if flag == 65535:
            break
        if index == MAXIMUM_RECORDS or flag >= 2048:
            raise ValueError('Source event record count/flag budget rejected')
        records.extend(overlay[offset:offset + 4])
    else:
        raise ValueError('Missing source event poll sentinel')
    count = len(records) // 4
    header = struct.pack('<4sHH6I32s', b'L3EP', 1, 64, 1, 0, count,
                         64 + len(records), zlib.crc32(records), 4,
                         bytes.fromhex(OVERLAY_SHA))
    assert len(header) == 64
    return header + records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite cooked source event profile')
    overlay = next(record['payload'] for record in iter_resources(args.dat / 'ST00.BIN')
                   if record['load'] == 0x80100000 and 'PROGBIN' in record['internal'].upper())
    blob = cook(overlay)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(blob)
    print(json.dumps({'schema': 'source-event-poll-cook/1', 'status': 'SOURCE_BACKED_PARTIAL',
                      'decoder': 'ST00_poll80100598', 'source_stage': 0,
                      'records': (len(blob) - 64) // 4, 'cooked_bytes': len(blob),
                      'sha256': hashlib.sha256(blob).hexdigest(),
                      'owner_payload_release_permitted': False, 'runtime_exercised': False}))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Compare original owner integer materialization with native neutral metadata.

Expected mutable flag scalars pass only through the local probe stdin, are
compared one by one, and are not retained. Saved evidence contains hashes,
counts, bounded code addresses and aggregate comparison fields only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib
from idx_materialization_source_probe import make_cpu, iter_resources, SUPPORTED_EXE, START, END, IDX


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'dat', 'map', 'render', 'init', 'native_probe', 'out'):
        p.add_argument('--' + name.replace('_', '-'), required=True, type=Path)
    a = p.parse_args()
    if a.out.exists(): raise SystemExit('refusing to replace earlier owner comparison')
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    c = make_cpu(image, base)
    resources = [r for r in iter_resources(a.dat / 'ST00.BIN')
                 if r['type'] == 0 and r['load'] in (0x8015C000, 0x80164000, 0x80194000, 0x8013A000)]
    assert len(resources) == 4
    for r in resources:
        for i, byte in enumerate(r['payload']): c.put(r['load'] + i, 1, byte)
    idx_resource = next(r for r in resources if r['load'] == IDX)
    count = len(idx_resource['payload']) // 12
    c.run_region()
    assert len(c.writes) == 994
    flags = [c.get(IDX + i * 12 + 6, 2) for i in range(count)]
    completed = subprocess.run([str(a.native_probe.resolve()), str(a.map.resolve()), str(a.render.resolve()),
                                str(a.init.resolve())], input='\n'.join(map(str, flags)) + '\n',
                               capture_output=True, text=True, check=True)
    native = json.loads(completed.stdout)
    assert native['all_original_idx_flags_matched'] and native['idx_records_compared'] == count
    assert native['flags_crc32'] == zlib.crc32(b''.join(struct.pack('<H', v) for v in flags)) & 0xFFFFFFFF
    files = {name: getattr(a, name).read_bytes() for name in ('map', 'render', 'init')}
    result = {'schema': 'idx-map-initialization-owner-comparison/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'source_exe_sha256': SUPPORTED_EXE, 'original_region': [hex(START), hex(END)],
              'original_region_sha256': sha(image[2048 + START - base:2048 + END - base]),
              'original_instructions_executed': c.steps, 'original_write_count': len(c.writes),
              'native_comparison': native,
              'cooked_files': {name: {'bytes': len(data), 'sha256': sha(data),
                                      'full_crc32': zlib.crc32(data) & 0xFFFFFFFF}
                               for name, data in files.items()},
              'owner_resource_hashes': [{'load': hex(r['load']), 'bytes': len(r['payload']), 'sha256': sha(r['payload'])}
                                        for r in resources],
              'limits': ['Original static owner ST00 materialization premises; live load timing is separate.',
                         'Stage0 startup only; later stage variants and resumed mutation are not reconstructed.',
                         'Original resource/instruction bytes and expected flag arrays are not retained.']}
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(native))


if __name__ == '__main__': main()

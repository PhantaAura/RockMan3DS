#!/usr/bin/env python3
"""Prepare a hash-guarded read-only Redux observer; does not launch an emulator."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

BIOS_SHA = '713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'


def quote(path):
    text = str(path.resolve())
    if ']=]' in text:
        raise ValueError('Unsupported Lua path delimiter')
    return '[=[' + text + ']=]'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('bios', 'exe', 'st00', 'trace', 'bootstrap'):
        p.add_argument('--' + name, required=True, type=Path)
    p.add_argument('--max-rows', type=int, default=16384)
    p.add_argument('--enable-http-status', action='store_true',
                   help='Opt in to the unverified HTTP bridge; file capture needs no HTTP')
    p.add_argument('--scope', choices=('CURRENT_INSTALLATION', 'MID_SESSION'),
                   default='CURRENT_INSTALLATION')
    a = p.parse_args()
    observer = Path(__file__).with_name('redux_rng_reference_observer.lua')
    metadata = a.bootstrap.with_suffix('.json')
    if any(path.exists() for path in (a.bootstrap, metadata, a.trace)):
        raise SystemExit('Refusing to overwrite a bootstrap, metadata or trace')
    if not 16 <= a.max_rows <= 65536:
        raise SystemExit('Observer row limit outside fixed supported bounds')
    hash_bytes = lambda data: hashlib.sha256(data).hexdigest()
    bios, exe = a.bios.read_bytes(), a.exe.read_bytes()
    if hash_bytes(bios) != BIOS_SHA or hash_bytes(exe) != EXE_SHA:
        raise SystemExit('Unsupported installed BIOS or supplied owner EXE')
    # The supported tutorial overlay is the first archive resource. Read only;
    # the payload stays in the owner input and is never written out.
    with a.st00.open('rb') as file:
        header = file.read(2048)
        size = struct.unpack_from('<I', header, 4)[0]
        if struct.unpack_from('<I', header, 0)[0] or size != 141056 or \
                struct.unpack_from('<I', header, 12)[0] != 0x80100000:
            raise SystemExit('Unsupported owner tutorial archive header')
        overlay = file.read(size)
    if hash_bytes(overlay) != OVERLAY_SHA:
        raise SystemExit('Unsupported owner tutorial overlay')
    lines = [
        'MML_RNG_BIOS_PATH=' + quote(a.bios),
        "MML_RNG_BIOS_SHA256='" + BIOS_SHA + "'",
        'MML_OWNER_EXE_PATH=' + quote(a.exe),
        "MML_OWNER_EXE_SHA256='" + EXE_SHA + "'",
        'MML_RNG_ST00_PATH=' + quote(a.st00),
        "MML_RNG_OVERLAY_SHA256='" + OVERLAY_SHA + "'",
        'MML_RNG_TRACE_PATH=' + quote(a.trace),
        'MML_RNG_MAX_ROWS=' + str(a.max_rows),
        'MML_RNG_ENABLE_HTTP_STATUS=' + ('true' if a.enable_http_status else 'false'),
        "MML_RNG_CAPTURE_SCOPE='" + a.scope + "'",
        'dofile(' + quote(observer) + ')',
    ]
    a.bootstrap.parent.mkdir(parents=True, exist_ok=True)
    a.trace.parent.mkdir(parents=True, exist_ok=True)
    a.bootstrap.write_text('\n'.join(lines) + '\n')
    result = {
        'schema': 'mml-rng-observer-preparation/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'bios_sha256': BIOS_SHA, 'owner_exe_sha256': EXE_SHA,
        'owner_tutorial_overlay_sha256': OVERLAY_SHA,
        'observer_source_sha256': hash_bytes(observer.read_bytes()),
        'bootstrap_sha256': hash_bytes(a.bootstrap.read_bytes()),
        'bootstrap_path': str(a.bootstrap.resolve()),
        'future_trace_path': str(a.trace.resolve()), 'max_scalar_rows': a.max_rows,
        'http_status_enabled': a.enable_http_status,
        'capture_scope': a.scope,
        'emulator_launched': False, 'observer_exercised': False,
        'seed_measured': False, 'owner_gameplay_changed': False,
        'retail_parity': False,
    }
    metadata.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()

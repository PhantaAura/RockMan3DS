#!/usr/bin/env python3
"""Read the hash-guarded original fresh player head selection.

No retail table, geometry, palette or instructions are embedded or written.
Only the fresh initialization branch is supported; resumed/gear mutation is not.
"""
from pathlib import Path
import hashlib
import re
import struct
from cook_actor_combat import OwnerExe, OWNER_SHA

FRESH_STORE = 0x8003BF58
HEAD_TABLE = 0x80089200
FILE_DIRECTORY = 0x80082CD0
FILE_NAMES = 0x80083870
MAX_HEAD_CONTAINER = 1 << 20


def _decode_fresh_head(owner, dat_dir):
    # Decode the actual fresh sb zero, +0x16A(s0) rather than storing a
    # retail selector table in this tool. The profile hash guards its caller.
    word = owner.word(FRESH_STORE)
    if (word >> 26, (word >> 21) & 31, (word >> 16) & 31, word & 65535) != (40, 16, 0, 0x16A):
        raise ValueError('Original fresh head store is unsupported')
    selector = 0  # The decoded store reads the architectural zero register.
    file_id = struct.unpack('<h', owner.read(HEAD_TABLE + selector * 2, 2))[0]
    if not 0 <= file_id < 256:
        raise ValueError('Fresh head file identity is outside the supported directory')
    name_record = owner.read(FILE_NAMES + file_id * 64, 64)
    end = name_record.find(b'\0')
    if end < 0 or any(name_record[end:]):
        raise ValueError('Original filename record is malformed')
    try:
        original_path = name_record[:end].decode('ascii')
    except UnicodeDecodeError as exc:
        raise ValueError('Original filename record is not ASCII') from exc
    parts = original_path.split('\\')
    if len(parts) < 3 or parts[-3:-1] != ['CDDATA', 'DAT'] or not re.fullmatch(r'HEAD[0-9]{2}\.BIN', parts[-1]):
        raise ValueError('Fresh selection is not a supported owner head container')
    container = parts[-1]
    lba, file_bytes, tag = struct.unpack('<III', owner.read(FILE_DIRECTORY + file_id * 12, 12))
    if tag != 0 or not 2048 <= file_bytes <= MAX_HEAD_CONTAINER or file_bytes % 2048:
        raise ValueError('Original head directory record is unsupported')
    with (Path(dat_dir) / container).open('rb') as handle:
        data = handle.read(file_bytes + 1)
    if len(data) != file_bytes:
        raise ValueError('Owner head container length differs from original directory')
    found = []
    for offset in range(0, len(data) - 2047, 2048):
        start = offset + 64
        end = data.find(b'\0', start, min(start + 256, len(data)))
        if end < 0:
            continue
        try:
            internal = data[start:end].decode('ascii')
        except UnicodeDecodeError:
            continue
        if not re.fullmatch(r'\.\.\\PLBIN\\PL[0-9]{2}H[0-9]{2}P\.PBD', internal):
            continue
        kind, count = struct.unpack_from('<II', data, offset)
        load = struct.unpack_from('<I', data, offset + 12)[0]
        first, limit = offset + 2048, offset + 2048 + count
        if kind != 0 or count == 0 or limit > len(data) or load & 3 or not 0x80000000 <= load < load + count <= 0x80200000:
            raise ValueError('Selected owner head resource is malformed or unsupported')
        found.append((internal, load, hashlib.sha256(data[first:limit]).hexdigest()))
    if len(found) != 1:
        raise ValueError('Selected owner head container must have exactly one head mesh')
    internal, load, resource_sha = found[0]
    return {'selector': selector, 'file_id': file_id, 'container': container,
            'internal': internal, 'resource': internal.rsplit('\\', 1)[-1],
            'load': load, 'resource_sha256': resource_sha,
            'container_sha256': hashlib.sha256(data).hexdigest(),
            'provenance': {'status': 'SOURCE_BACKED_PARTIAL', 'fresh_only': True,
                           'executable_sha256': OWNER_SHA, 'store': hex(FRESH_STORE),
                           'loader': '0x8001F5E4', 'selector_byte': 'Player_work+0x16A',
                           'head_table': hex(HEAD_TABLE), 'file_id': file_id,
                           'filename_record': hex(FILE_NAMES + file_id * 64),
                           'directory_record': hex(FILE_DIRECTORY + file_id * 12),
                           'original_path': original_path, 'lba': lba,
                           'directory_bytes': file_bytes, 'resumed_or_gear_selection': 'UNKNOWN'}}


def select_fresh_player_head(image: bytes, dat_dir: Path) -> dict:
    """Return original filename/resource identities; never extract or cook them."""
    return _decode_fresh_head(OwnerExe(image), dat_dir)

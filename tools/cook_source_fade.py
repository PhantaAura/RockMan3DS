#!/usr/bin/env python3
"""Create a NEW private type2 fade startup profile from the guarded owner EXE."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib
from cook_actor_combat import OwnerExe, OWNER_SHA

SOURCE = Path(__file__).resolve().parents[1]

def cook(image, include_handoff=False, steps=False):
    owner = OwnerExe(image)
    if steps:
        # v3: durations 0x80097A44/46 and the step table 0x80097A48..4E.
        payload = owner.read(0x80097A44, 12)
        version = 3
    else:
        payload = owner.read(0x80097A44, 2) + owner.read(0x80097A4C, 2)
        if include_handoff:
            payload += owner.read(0x80097A46, 2) + bytes(2)
        version = 2 if include_handoff else 1
    package = struct.pack('<4sHH6I', b'L3FD', version, 64, version, 0, 1, 64+len(payload),
                          zlib.crc32(payload) & 0xffffffff, len(payload)) + bytes.fromhex(OWNER_SHA) + payload
    evidence = {'schema': 'mml-source-fade-startup-cook/1', 'owner_exe_sha256': OWNER_SHA,
                'source_addresses': ['0x80097a44', '0x80097a4c'],
                'profile_sha256': hashlib.sha256(package).hexdigest(),
                'retail_values_embedded_in_source': False, 'retail_parity': False,
                'pending_scheduler': '80012E10(slot2,callback80017E94) remains unconsumed'}
    if steps:
        evidence['source_addresses'] = ['0x80097a44..0x80097a4f']
        evidence['schema'] = 'mml-source-fade-startup-cook/3'
    elif include_handoff:
        evidence['source_addresses'].append('0x80097a46')
        evidence['schema'] = 'mml-source-fade-startup-cook/2'
    return package, evidence

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--handoff', action='store_true', help='Also retain distinct source type10 duration in L3FDv2')
    args = parser.parse_args()
    output = args.out.resolve()
    if output.exists() or output.is_relative_to(SOURCE) or output == args.exe.resolve():
        raise ValueError('Choose a NEW private output directory outside source and inputs')
    package, evidence = cook(args.exe.read_bytes(), args.handoff)
    output.mkdir(parents=True, exist_ok=False)
    (output / 'source_fade.l3fd').write_bytes(package)
    (output / 'cook-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))

if __name__ == '__main__':
    main()

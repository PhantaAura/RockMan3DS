#!/usr/bin/env python3
"""Retain the owner executable's collision mode table as neutral local metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib
from cook_actor_combat import OwnerExe, OWNER_SHA


def build_metadata(image):
    owner = OwnerExe(image)
    payload = owner.read(0x80097D00, 8)
    header = struct.pack('<4sHHIIII', b'L3PR', 1, 24, 32, 8, zlib.crc32(payload), 0)
    return header + payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1]
    if args.out.exists() or args.out.resolve().is_relative_to(source):
        raise ValueError('Choose a NEW owner-local output outside source')
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != OWNER_SHA:
        raise ValueError('Unsupported owner executable')
    package = build_metadata(image)
    args.out.mkdir(parents=True, exist_ok=False)
    path = args.out / 'hit_proxy_rules.l3pr'
    path.write_bytes(package)
    evidence = {'schema':'mml-hit-proxy-cook/1','status':'SOURCE_BACKED_PARTIAL',
                'source_sha256':OWNER_SHA,'source_table':'0x80097D00',
                'unsigned_table_entries':8,'package_bytes':len(package),
                'package_sha256':hashlib.sha256(package).hexdigest(),
                'retail_payload_embedded_in_source':False,'retail_parity':False}
    (args.out/'hit_proxy_rules_evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps(evidence))


if __name__ == '__main__': main()

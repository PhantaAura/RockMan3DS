#!/usr/bin/env python3
"""Bounded owner-local actor metadata; no retail values embedded in source.

Reads only an exact already supplied EXE. Exclusively creates a NEW directory
outside source with sparse combat/detector metadata and original normalization
lookup. These tiny owner-derived sidecars remain excluded from source ZIPs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

SOURCE = Path(__file__).resolve().parents[1]
OWNER_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
NORMALIZATION_ADDRESS = 0x80092D38
DETECTOR_BOUNDS_ADDRESS = 0x80097D0C
DETECTION_PROFILE_TABLE = 0x800899AC
STATS_TABLES = {1: (0x8008EDC4, 0), 2: (0x8008F060, 1)}
SOURCE_CLASS, SOURCE_SUBTYPE = 0x20, 3


def sha(data):
    return hashlib.sha256(data).hexdigest()


def crc(data):
    return zlib.crc32(data) & 0xFFFFFFFF


def header(magic, count, entry_bytes, payload, version=1):
    return struct.pack('<4sHH6I', magic, version, 32, 1, count, entry_bytes,
                       32 + len(payload), crc(payload), 0)


class OwnerExe:
    def __init__(self, image):
        if sha(image) != OWNER_SHA or len(image) < 2048 or image[:8] != b'PS-X EXE':
            raise ValueError('Unsupported owner executable profile')
        self.image = image
        self.base, self.size = struct.unpack_from('<II', image, 0x18)
        if self.base & 3 or self.size > len(image) - 2048 or self.base + self.size > 0x100000000:
            raise ValueError('Invalid owner executable segment')

    def read(self, address, size):
        if size < 0 or address < self.base or address + size > self.base + self.size:
            raise ValueError('Metadata address outside owner executable segment')
        offset = 2048 + address - self.base
        return self.image[offset:offset + size]

    def word(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]


def build_metadata(owner):
    rows, profile_ids, origins = [], set(), []
    for selector, (table, variant) in sorted(STATS_TABLES.items()):
        # Original80064718 selects the table. Original class03 initializer and
        # action3 index subtype03's pointer, then source record index0C=0.
        slot = table + SOURCE_SUBTYPE * 4
        record = owner.word(slot)
        if record & 3:
            raise ValueError('Selected source record is unaligned')
        fields = owner.read(record, 16)
        byte08, byte09, byte0a, byte0c, byte0f = fields[8], fields[9], fields[10], fields[12], fields[15]
        rows.append(bytes((SOURCE_CLASS, SOURCE_SUBTYPE, selector, variant, 0, 0,
                           byte08, byte0a, byte0f, byte0c, 7, byte09)) + fields[:4])
        profile_ids.add(byte0f)
        origins.append({'raw_stats_selector': selector, 'source_variant': variant,
                        'source_class': SOURCE_CLASS, 'subtype': SOURCE_SUBTYPE,
                        'record_index0C': 0, 'context_high0F': 0,
                        'source_table_pointer_slot': hex(slot), 'source_record': hex(record),
                        'retained_offsets': [0, 8, 9, 10, 12, 15],
                        'action4_variant32E': 0,
                        'retained_values_in_evidence': False})
    if profile_ids != {4}:
        raise ValueError('Selected detector profile outside current proved profile4 contract')
    combat_payload = b''.join(rows)
    combat = header(b'L3CB', len(rows), 16, combat_payload, version=4) + combat_payload
    detection_payload = b''.join(bytes((profile,)) +
        owner.read(DETECTION_PROFILE_TABLE + profile * 4, 3) for profile in sorted(profile_ids))
    # Preserve the already established L3DP layout exactly (field16 is32).
    detection = struct.pack('<4sHH6I', b'L3DP', 1, 32, 1, len(profile_ids), 32,
                            32 + len(detection_payload), crc(detection_payload), 0) + detection_payload
    normalization_payload = owner.read(NORMALIZATION_ADDRESS, 384)
    normalization = struct.pack('<4sHH6I', b'L3NR', 1, 32, 416, 192,
                                crc(normalization_payload), 0, 0, 0) + normalization_payload
    bounds_payload = owner.read(DETECTOR_BOUNDS_ADDRESS, 12)
    bounds = struct.unpack('<6h', bounds_payload)
    if any(bounds[i] >= bounds[i + 1] for i in (0, 2, 4)):
        raise ValueError('Original detector bounds pairs unordered')
    bounds_package = struct.pack('<4sHH6I', b'L3DB', 1, 32, 0, DETECTOR_BOUNDS_ADDRESS,
                                6, 12, crc(bounds_payload), 44) + bounds_payload
    packages = {'actor_combat.l3cb': combat, 'actor_detection.l3dp': detection,
                'actor_normalization.l3nr': normalization,
                'actor_detection_bounds.l3db': bounds_package}
    evidence = {'schema': 'mml-actor-local-metadata-cook/3', 'status': 'SOURCE_BACKED_PARTIAL',
                'combat_format_version': 4,
                'owner_exe_sha256': OWNER_SHA, 'selection_origins': origins,
                'initialization_contract': 'Raw selector1 is known only after explicit source fresh boot initialization; unknown restoration invalidates it.',
                'source_profile_ids': sorted(profile_ids),
                'normalization_source': hex(NORMALIZATION_ADDRESS),
                'detector_bounds_source': hex(DETECTOR_BOUNDS_ADDRESS),
                'retail_values_embedded_in_source': False, 'retail_parity': False,
                'outputs': {name: {'bytes': len(data), 'sha256': sha(data)}
                            for name, data in packages.items()},
                'limits': ['Only class20/subtype03 with index0C0/context0F0 and raw selector1/2 is retained.',
                           'No generic selector, object, restored game or alternate-context fallback.',
                           'Sparse records preserve original word00 and unsigned fields08/09/A/C/F only.',
                           'Action4 byteC is selected for actor variant32E0 only; source_variant tags stats table selection, not actor variant32E.',
                           'All owner-derived sidecars and this local output directory are excluded from source releases.']}
    return packages, evidence


def cook(image):
    return build_metadata(OwnerExe(image))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path,
                        help='New owner-local output directory outside source')
    args = parser.parse_args()
    output = args.out.resolve()
    if output.is_relative_to(SOURCE) or output.exists() or output == args.exe.resolve():
        raise ValueError('Choose a NEW owner-local output directory outside source and inputs')
    packages, evidence = cook(args.exe.read_bytes())
    output.mkdir(parents=True, exist_ok=False)
    for name, data in packages.items():
        with (output / name).open('xb') as file:
            file.write(data)
    evidence['output_directory'] = str(output)
    with (output / 'cook-evidence.json').open('x') as file:
        file.write(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()

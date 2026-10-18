#!/usr/bin/env python3
"""Original Action4 byteC selection versus sparse owner-local v2 cooking.

Only already supplied owner inputs are read. The original scalar selection and
phase4 expiry execute with authored actor state; no live retail parity claim.
No package payload or owner record values are written to source/evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from cook_actor_combat import OwnerExe, build_metadata
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

WORK = 0x1000
TABLE = 0x80098844
SELECTOR = 0x800C1B6B


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Probe(OverlayProbe):
    def __init__(self, image, base, overlay):
        super().__init__(image, base, overlay)
        self.watched = set()
        self.reads = []

    def get(self, address, size, signed=False):
        value = super().get(address, size, signed)
        if address in self.watched:
            self.reads.append({'address': hex(address), 'bytes': size, 'signed': signed})
        return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    if output.exists() or output.is_relative_to(SOURCE):
        raise ValueError('Choose NEW owner-local evidence outside source')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise ValueError('Unsupported owner executable')
    owner = OwnerExe(image)
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise ValueError('Unsupported original tutorial overlay')
    # Build transiently in memory for source-vs-cooker comparison. No cooked
    # payload output belongs to this probe or to a source checkpoint.
    packages, _ = build_metadata(owner)
    combat = packages['actor_combat.l3cb']
    assert len(combat) == 56 and struct.unpack_from('<H', combat, 4)[0] == 2
    cases = []
    total_steps = 0
    for selector in (1, 2):
        for variant, authored in [(0, None), (1, None)] + [(0, v) for v in (0, 1, 127, 128, 255)]:
            cpu = Probe(image, owner.base, overlay)
            cpu.r[28], cpu.r[29], cpu.r[31] = 0x80097864, 0x7000, 0
            cpu.put(SELECTOR, 1, selector)
            cpu.run(0x80064718)
            table = cpu.get(TABLE, 4)
            record = cpu.get(table + 3 * 4, 4)
            assert table == (0x8008F060 if selector == 2 else 0x8008EDC4)
            selected = record + variant * 16 + 12
            expected = owner.read(selected, 1)[0] if authored is None else authored
            if authored is not None:
                cpu.put(selected, 1, authored)
            cpu.watched = {record + 12, record + 16 + 12}
            cpu.r[4], cpu.r[31] = WORK, 0
            for offset, width, value in ((1, 1, 0x20), (2, 1, 3), (9, 1, 4),
                    (10, 1, 4), (12, 1, 0), (0x32C, 2, 0),
                    (0x32E, 2, variant), (0x35C, 2, 17)):
                cpu.put(WORK + offset, width, value)
            cpu.run(0x80102FC0)
            assert cpu.get(WORK + 0x35C, 2, True) == expected
            assert cpu.get(WORK + 0x328, 2) & 1
            assert cpu.reads == [{'address': hex(selected), 'bytes': 1, 'signed': False}]
            assert not cpu.calls
            cooked_row = combat[32 + (selector - 1) * 12:44 + (selector - 1) * 12]
            assert cooked_row[:6] == bytes((0x20, 3, selector, int(selector == 2), 0, 0))
            assert cooked_row[10:] == bytes((1, 0))
            if authored is None and variant == 0:
                assert cooked_row[9] == expected
            if variant == 1:
                assert selected != record + 12
            cases.append({'raw_stats_selector': selector, 'source_table_tag': int(selector == 2),
                          'actor_variant32E': variant, 'source_record_base': hex(record),
                          'selected_byte_address': hex(selected), 'authored_byteC': authored,
                          'source_reads': cpu.reads, 'instructions': cpu.steps,
                          'matches_original_unsigned_field': True,
                          'matches_cooked_field': authored is None and variant == 0,
                          'nonzero_actor_variant_not_retained': variant != 0})
            total_steps += cpu.steps
    result = {'schema': 'mml-action4-byteC-source/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
              'transient_v2_package_sha256': sha(combat), 'cases': cases,
              'owner_base_record_cases': 2, 'authored_unsigned_boundary_cases': 10,
              'alternate_record_rejection_cases': 2, 'instructions_executed': total_steps,
              'selection': {'phase4_expiry_entry': '0x80102FC0',
                            'subtype_load': '0x80103158', 'table_global': hex(TABLE),
                            'variant_load': '0x8010316C', 'unsigned_byteC_load': '0x80103180',
                            'work35C_store': '0x80103190'},
              'original_code_region_sha256': sha(overlay[0x3158:0x3194]),
              'limits': ['Actor phase, timer and context are authored, not a live encounter.',
                         'Only subtype03/context0F0/index0C0 and raw selector1/2 are retained.',
                         'Actor variant32E must be0; legacy source_variant metadata tags table selection.',
                         'Original unsigned byteC semantics beyond source work35C store are not named.',
                         'No retail payload or owner record values exported; PHYSICS_1TO1 remains false.'],
              'retail_parity': False, 'retail_payload_exported': False}
    output.mkdir(parents=True, exist_ok=False)
    (output / 'source-evidence.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'cases': len(cases), 'instructions': total_steps,
                      'owner_base_records_match': True, 'actor_variant1_not_retained': True}))


if __name__ == '__main__':
    main()

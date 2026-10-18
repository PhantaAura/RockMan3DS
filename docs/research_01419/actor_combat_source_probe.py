#!/usr/bin/env python3
"""Bounded original initializer/action3 versus cooked sparse fields.

Owner files are read only. Original instructions execute in synthetic memory;
floor/matrix dependencies are explicit return-only hooks, not retail gameplay.
Scalar expected output and all evidence go to a NEW owner-local directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from mml_exe_research import Image
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

WORK = 0x1000
TABLE_GLOBAL = 0x80098844
SELECTOR = 0x800C1B6B


def sha(data):
    return hashlib.sha256(data).hexdigest()


def package_rows(data):
    if len(data) != 56 or data[:8] != struct.pack('<4sHH', b'L3CB', 1, 32):
        raise ValueError('Expected exact two-row L3CB package')
    header = struct.unpack_from('<6I', data, 8)
    if header != (1, 2, 12, 56, zlib.crc32(data[32:]) & 0xFFFFFFFF, 0):
        raise ValueError('Invalid L3CB header/CRC')
    rows = {}
    for offset in (32, 44):
        row = data[offset:offset + 12]
        selector = row[2]
        if row[:6] != bytes((0x20, 3, selector, int(selector == 2), 0, 0)) or selector not in (1, 2):
            raise ValueError('Unsupported L3CB selection identity')
        if row[9:] != bytes(3) or selector in rows:
            raise ValueError('Invalid L3CB sparse row')
        rows[selector] = row[6:9]
    if list(rows) != [1, 2]:
        raise ValueError('L3CB rows unordered')
    return rows


class CombatProbe(OverlayProbe):
    def __init__(self, image, base, overlay):
        super().__init__(image, base, overlay)
        self.selected_reads = []
        self.watch = set()

    def get(self, address, size, signed=False):
        value = super().get(address, size, signed)
        if address in self.watch:
            self.selected_reads.append({'address': hex(address), 'bytes': size, 'signed': signed})
        return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--package', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    if output.exists() or output.is_relative_to(SOURCE):
        raise ValueError('Choose a NEW owner-local evidence directory outside source')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise ValueError('Unsupported owner executable')
    exe = Image(image)
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise ValueError('Unsupported original tutorial overlay')
    ebd = next(r['payload'] for r in iter_resources(args.dat / 'ST00_01.BIN')
               if r['load'] == 0x8016C000)
    cooked = args.package.read_bytes()
    rows = package_rows(cooked)
    total_steps = 0
    expected = []
    cases = []

    def cpu_for(selector, authored=None):
        cpu = CombatProbe(image, exe.base, overlay)
        cpu.mem.update({0x8016C000 + i: value for i, value in enumerate(ebd)})
        cpu.r[28], cpu.r[29], cpu.r[31] = 0x80097864, 0x7000, 0
        cpu.put(SELECTOR, 1, selector)
        cpu.run(0x80064718)
        selection_steps = cpu.steps
        table = cpu.get(TABLE_GLOBAL, 4)
        record = cpu.get(table + 3 * 4, 4)
        assert table == (0x8008F060 if selector == 2 else 0x8008EDC4)
        assert exe.base <= record and record + 16 <= exe.base + len(image) - 2048
        if authored is not None:
            for offset, value in zip((8, 10, 15), authored):
                cpu.put(record + offset, 1, value)
        cpu.watch = {record + 8, record + 10, record + 15}
        cpu.steps = 0
        cpu.r[4], cpu.r[31] = WORK, 0
        for offset, value in ((0, 1), (1, 0x20), (2, 3), (6, 3), (0xA, 0), (0xC, 0), (0xF, 0)):
            cpu.put(WORK + offset, 1, value)
        return cpu, record, selection_steps

    for selector in (1, 2):
        for authored in (None, (0, 0, 4), (1, 1, 4), (127, 127, 4), (128, 128, 4), (255, 255, 4)):
            fields = rows[selector] if authored is None else bytes(authored)
            initializer, record, selection_steps = cpu_for(selector, authored)
            initializer.hook(0x80048E6C, 0)  # Ground consumer excluded.
            initializer.hook(0x8003346C, 0)  # Matrix/pose consumer excluded.
            initializer.run(0x80102410)
            hp = initializer.get(WORK + 0x6E, 2, True)
            detector_profile = initializer.get(WORK + 0x352, 1)
            assert hp == fields[0] * 16 and detector_profile == fields[2]
            assert {'address': hex(record + 8), 'bytes': 1, 'signed': False} in initializer.selected_reads
            assert {'address': hex(record + 15), 'bytes': 1, 'signed': False} in initializer.selected_reads
            action, action_record, action_selection_steps = cpu_for(selector, authored)
            action.run(0x80102C9C)
            work330 = action.get(WORK + 0x330, 2, True)
            assert work330 == fields[1] and action.get(WORK + 0xA, 1) == 1
            assert action.get(WORK + 0x32C, 2) == 4 and not action.calls
            assert {'address': hex(action_record + 10), 'bytes': 1, 'signed': False} in action.selected_reads
            total_steps += initializer.steps + action.steps + selection_steps + action_selection_steps
            if authored is None:
                expected.append((selector, hp, work330, detector_profile))
            cases.append({'raw_selector': selector, 'source_variant': int(selector == 2),
                          'source_class': 0x20, 'source_subtype': 3,
                          'record_index0C': 0, 'context_high0F': 0,
                          'source_record': hex(record), 'authored_fields': authored,
                          'initializer_instructions': initializer.steps,
                          'initializer_return_only_hooks': initializer.return_only_hooks,
                          'initializer_selected_reads': initializer.selected_reads,
                          'action3_phase0_instructions': action.steps,
                          'action3_phase0_calls': action.calls,
                          'action3_selected_reads': action.selected_reads,
                          'original_vs_cooked_or_authored_fields': 'MATCHES_MEASURED_FIELDS'})
    expected_data = ''.join('%d\t%d\t%d\t%d\n' % row for row in expected).encode()
    result = {'schema': 'mml-class03-combat-fields-source/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': sha(image), 'overlay_sha256': sha(overlay), 'ebd_sha256': sha(ebd),
              'cooked_package_sha256': sha(cooked), 'owner_expected_sha256': sha(expected_data),
              'owner_record_cases': len(expected), 'authored_unsigned_boundary_cases': len(cases) - len(expected),
              'instructions_executed': total_steps, 'cases': cases,
              'retail_parity': False, 'universal_selector_default': False,
              'limits': ['Only original class03 selected record index0C0/context0F0 is exercised.',
                         'Source initializer floor/matrix dependencies are authored return-only hooks.',
                         'Action3 phase0 has no helper calls; later phases are not covered.',
                         'Synthetic CPU execution is neither live gameplay nor Old3DS hardware evidence.',
                         'Scalar expected TSV remains owner-local and excluded from source release.']}
    output.mkdir(parents=True, exist_ok=False)
    with (output / 'expected.tsv').open('xb') as file:
        file.write(expected_data)
    with (output / 'source-evidence.json').open('x') as file:
        file.write(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'status': result['status'], 'owner_cases': len(expected),
                      'authored_cases': len(cases) - len(expected), 'instructions': total_steps,
                      'retail_parity': False, 'evidence_sha256': sha((output / 'source-evidence.json').read_bytes())}))


if __name__ == '__main__':
    main()

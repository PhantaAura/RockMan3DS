#!/usr/bin/env python3
"""Original scalar sequence/admission cases and owner-local request0 closure."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--dat', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    ebd = next(r['payload'] for r in iter_resources(a.dat / 'ST00_01.BIN') if r['load'] == 0x8016c000)
    if a.out.exists():
        raise SystemExit('Refusing to overwrite evidence directory')
    a.out.mkdir(parents=True)
    cases = []
    steps = 0
    for index, count, control in itertools.product((0, 1, 127, 128, 254),
            (0, 1, 2, 127, 128, 255), (0, 1, 127, 128, 129, 252, 254, 255)):
        c = OverlayProbe(image, base, b'')
        c.r[4], c.r[29] = 0x1000, 0x8000
        for i in range(256):
            record = bytes(((i * 3) & 255, (i * 7) & 255, i ^ 0x55, 0, 1, 2, 3, 4))
            for j, value in enumerate(record):
                c.put(0x3000 + i * 8 + j, 1, value)
        c.put(0x3000 + index * 8 + 3, 1, control)
        c.put(0x1094, 4, 0x3000)
        c.put(0x1098, 4, 0x3000 + index * 8)
        c.put(0x10a4, 4, 42 | count << 8 | 77 << 16 | 13 << 24)
        c.hook(0x80033a28)
        c.hook(0x8003346c)
        c.run(0x80032488)
        actual = [(c.get(0x1098, 4) - 0x3000) // 8,
                  *(c.get(0x10a4 + j, 1) for j in range(4))]
        cases.append([index, count, control, *actual])
        steps += c.steps
    fixture = a.out / 'actor-sequence-source-cases.tsv'
    fixture.write_text('index\tcountdown\tcontrol\tafter_index\tafter_frame\tafter_countdown\tafter_A6\tafter_A7\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in cases))

    admission = []
    for flags0, flags6 in itertools.product((0, 1, 2, 3, 128, 255), repeat=2):
        c = OverlayProbe(image, base, b'')
        c.r[29] = 0x8000
        c.put(0x1f800058, 4, 0x4000)
        c.put(0x400c, 4, 0x4100)
        c.put(0x4100, 4, 0x2000)
        # Authored callback increments a scalar counter. The original16-slot
        # admission loop and dynamic dispatch execute without a hook.
        for offset, word in enumerate((0x8c023000, 0x24420001, 0xac023000, 0x03e00008, 0)):
            c.put(0x2000 + offset * 4, 4, word)
        c.put(0x8009f108, 1, flags0)
        c.put(0x8009f10a, 1, 3)
        c.put(0x8009f10c, 1, 0)
        c.put(0x8009f10e, 1, flags6)
        c.run(0x80016fd4)
        actual = c.get(0x3000, 4)
        assert actual == int(bool(flags0 & flags6 & 1))
        admission.append({'work0': flags0, 'work6': flags6, 'callback_count': actual})

    ebd_base = 0x8016c000
    row = struct.unpack_from('<4B3I', ebd, 4)
    assert row[:4] == (32, 3, 0, 0)
    sequence = struct.unpack_from('<I', ebd, row[-1] - ebd_base)[0]
    index, seen, edges = 0, set(), []
    while index not in seen:
        assert 0 <= index < 256
        seen.add(index)
        offset = sequence - ebd_base + index * 8
        assert 0 <= offset <= len(ebd) - 8
        control = ebd[offset + 3]
        next_index = index + 1 if control < 128 else index if control == 255 else control & 127
        edges.append({'record_index': index, 'next_record_index': next_index})
        index = next_index
    assert seen == set(range(8)) and index == 0
    source_region = lambda start, end: sha(image[2048 + start - base:2048 + end - base])
    evidence = {'schema': 'actor-sequence-source-cases/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'fixture_sha256': sha(fixture.read_bytes()), 'case_count': len(cases),
        'scalar_output_fields': 5, 'instructions_executed': steps,
        'sources': {'exe_sha256': sha(image), 'overlay_sha256': sha(overlay), 'ebd_sha256': sha(ebd)},
        'sequence_function': {'address': '0x80032488', 'end_exclusive': '0x80032538',
                              'sha256': source_region(0x80032488, 0x80032538)},
        'sequence_hooks': ['0x80033a28 pose consumer excluded', '0x8003346c matrix consumer excluded'],
        'callback_ownership': {'callback': '0x8010226c',
            'old_request_snapshot': '0x80102280..0x80102288: workAC ->workAD',
            'request_compare': '0x8010231c..0x80102328',
            'request_changed': '0x80102334 ->0x800322a8 resets request using record0',
            'request_unchanged': '0x80102344 ->0x80032488 advances',
            'initializer': '0x801025d8 ->0x800322a8 selects request0',
            'allocation_tick': 'If oldAC==0, wrapper also advances after initializer reset; allocation zeroing/cadence external.'},
        'scheduler': {'dispatcher': '0x80016fd4', 'gate': '(signed work0 & unsigned work6 &1) !=0',
            'pool_base': '0x8009f108', 'stride': 960, 'capacity': 16,
            'scratch_subtype_table': '0x1f800058', 'dispatch_site': '0x80017054',
            'admission_cases': admission,
            'gameplay_update': '0x80015db4', 'distance_before_dispatch': '0x80015ddc ->8003606c',
            'dispatch_call': '0x80015df4 ->80016fd4', 'additional_caller': '0x800137f4',
            'cadence': 'CANDIDATE', 'pause_ownership': 'UNKNOWN'},
        'owner_request0': {'class': 32, 'subtype': 3, 'row_index': 0, 'request': 0,
            'sequence_pointer': hex(sequence), 'reachable_record_count': len(seen),
            'highest_reachable_index': max(seen), 'edges': edges, 'cycle_entry_index': index,
            'sequencing_bytes_sha256': sha(ebd[sequence - ebd_base:sequence - ebd_base + 64]),
            'selected_by_initializer': True},
        'limits': ['Original integer sequence branches and callback admission tested with synthetic authored records/work.',
                   'Matrix/GTE consumer rounding and live gameplay frame cadence not proved.',
                   'Native gameplay-clock scheduling must remain a default-off diagnostic candidate gate.',
                   'Class20/35 diagnostic request0 is not eligible for source-owned animation.',
                   'Full AI request changes, event pause, original allocation-frame timing and cutscene ownership unknown.',
                   'No original sequencing bytes, pose/model data or function tables serialized.']}
    (a.out / 'actor-sequence-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(f'{len(cases)} original sequence cases; {len(admission)} callback admission cases; owner request0 closes at8records')


if __name__ == '__main__':
    main()

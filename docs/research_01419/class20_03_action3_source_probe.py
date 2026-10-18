#!/usr/bin/env python3
"""Bounded action3 differential: authored scalars, owner inputs read-only.

Original 80102C9C and turn/accumulator instructions execute. Selected combat
byteA, target angle, motion delta, wall/floor, sound and allocator owner remain
explicit external premises. No owner instruction/table/payload bytes exported.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
sys.path.insert(0, str(SOURCE / 'docs/research_01417'))
from class20_03_callback_source_probe import authored_store_hook, signed16, signed32
sys.path.insert(0, str(SOURCE / 'docs/research_01418'))
from class20_03_transaction_source_probe import FIELDS as BASE_FIELDS, sound_hook

WORK = 0x1000
FIELDS = BASE_FIELDS + [(100, 2, True), (13, 1, True)]


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def fields(cpu):
    return [cpu.get(WORK + off, size, signed) for off, size, signed in FIELDS]


def write_array(cpu, offset, width, values):
    for i, value in enumerate(values):
        cpu.put(WORK + offset + i * width, width, value)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    args = p.parse_args()
    image = args.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    if args.out.exists():
        raise SystemExit('Refusing to overwrite action3 evidence')
    args.out.mkdir(parents=True)
    rows, steps = [], 0
    counts = {'timer_entry': 0, 'sound_delay': 0, 'motion': 0, 'release_tail': 0,
              'unsupported_phase_noop': 0}

    def fresh():
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[29] = WORK, 0x8000
        for off, size, value in [(0, 1, 3), (1, 1, 0x20), (2, 1, 3), (3, 1, 0),
                                 (8, 1, 1), (9, 1, 3), (10, 1, 0), (11, 1, 19),
                                 (13, 1, -7), (14, 1, 4), (86, 2, 4095), (100, 2, 16),
                                 (102, 2, 33), (110, 2, 128), (172, 1, 7), (173, 1, 99),
                                 (176, 2, 128), (178, 2, 127), (350, 2, 13),
                                 (808, 2, 0), (812, 2, 99), (814, 2, 0), (816, 2, 22),
                                 (850, 1, 4), (851, 1, 2), (859, 1, -6), (864, 2, 4)]:
            c.put(WORK + off, size, value)
        write_array(c, 20, 2, [101, -7, -202, 1234])
        write_array(c, 28, 4, [0x7ffffffe, -7 * 65536 + 12345,
                              -202 * 65536 - 2222, 0x24681357])
        write_array(c, 84, 2, [11, 4095, 33, 44])
        write_array(c, 44, 2, [103, -7, -205, -1234])
        write_array(c, 52, 4, [17, -17, 0x7fffffff, -0x80000000])
        write_array(c, 92, 2, [51, 52, 53, 54])
        write_array(c, 384, 2, [61, 62, 63, 64])
        write_array(c, 820, 2, [71, 72, 73, 74])
        sound_hook(c)
        # Explicit authored selected record0 and byteA value. No original table
        # semantics or entry selection is exported or inferred by this probe.
        c.put(0x80098844, 4, 0x6000)
        c.put(0x6000 + 3 * 4, 4, 0x6100)
        authored_store_hook(c, 0x80031824, [(4, 0, 1, 0)])
        return c

    def append(c, category, profile=0, target=-1, floor=-7, contact=0, delta=None):
        nonlocal steps
        packet = c.get(WORK + 828, 4)
        c.put(0x6100 + 10, 1, profile)
        prepared = None
        if target >= 0:
            c.hook(0x80031198, target)
        if delta is not None:
            # Actual original turn helper supplies the prepared heading: it is
            # independent of the native implementation under differential test.
            t = OverlayProbe(image, base, overlay)
            t.r[4], t.r[5], t.r[6], t.r[7], t.r[29] = WORK + 20, 0x800b51c4, c.get(WORK + 86, 2), (
                256 if c.get(WORK + 10, 1) == 3 and c.get(WORK + 812, 2, True) < 4 else c.get(WORK + 100, 2, True)) & 0xffffffff, 0x8000
            t.hook(0x80031198, target)
            t.run(0x80031bec)
            heading = (c.get(WORK + 86, 2) + t.r[2]) & 4095
            steps += t.steps
            authored_store_hook(c, 0x80030058, [(4, i * 4, 4, v) for i, v in enumerate(delta)])
            authored_store_hook(c, 0x8004b414, [], contact)
            authored_store_hook(c, 0x80048e6c, [], floor)
            fixed = [signed32(c.get(WORK + 28 + i * 4, 4) + delta[i]) for i in range(3)]
            fixed += [c.get(WORK + 40, 4, True)]
            raw = [signed16((v >> 16) + ((v >> 15) & 1)) for v in fixed[:3]]
            raw += [c.get(WORK + 26, 2, True)]
            prepared = raw + fixed + [floor, contact, heading]
        before = fields(c)
        c.run(0x80102c9c)
        sounds = c.get(0x9008, 4)
        assert sounds <= 1
        sound = c.get(0x9000, 4) if sounds else -1
        release = int(any(call['target'] == '0x80031824' for call in c.calls))
        prep = [1] + prepared if prepared is not None else [0] + [0] * 11
        rows.append([packet, profile, target, 1, 1] + prep + before + fields(c) + [sound, release])
        steps += c.steps
        counts[category] += 1

    for phase, timer, packet, profile in itertools.product(
            (0, 1), (-32768, -1, 0, 1, 5, 32767), (0, 0x02000000), (0, 1, 255)):
        c = fresh()
        for off, size, value in [(10, 1, phase), (812, 2, timer), (828, 4, packet)]: c.put(WORK + off, size, value)
        append(c, 'timer_entry', profile)
    for timer, packet, flags, delay in itertools.product(
            (-32768, -1, 0, 1, 2, 32767), (0, 0x02000000), (0, 0x20, 0x30), (-128, -1, 0, 1, 45, 127)):
        c = fresh()
        for off, size, value in [(10, 1, 2), (812, 2, timer), (828, 4, packet), (808, 2, flags), (13, 1, delay)]:
            c.put(WORK + off, size, value)
        append(c, 'sound_delay')
    for phase, timer, packet, target, floor_delta, contact in itertools.product(
            (3, 4), (-32768, -1, 0, 1, 3, 4, 45), (0, 0x02000000),
            (0, 2048, 4095), (-33, -32, 0, 32, 33), (0, 0x100, 0x1000)):
        c = fresh()
        for off, size, value in [(10, 1, phase), (812, 2, timer), (828, 4, packet)]: c.put(WORK + off, size, value)
        # Wrapped negative timer -32768 becomes32767, so it still needs motion.
        next_timer = signed16(timer - 1)
        moving = next_timer >= 0 and not (packet & 0x02000000)
        append(c, 'motion', target=target if moving else -1, floor=-7 + floor_delta,
               contact=contact, delta=[53, -16385, 65535] if moving else None)
    for timer, packet, parameter, animation in itertools.product(
            (-32768, -1, 0, 1, 10, 32767), (0, 0x02000000), (0, 0x20, 0x60), (2, 255)):
        c = fresh()
        for off, size, value in [(10, 1, 5), (812, 2, timer), (828, 4, packet), (3, 1, parameter), (172, 1, animation)]:
            c.put(WORK + off, size, value)
        append(c, 'release_tail')
    for phase, packet in itertools.product((6, 7, 255), (0, 0x02000000)):
        c = fresh();c.put(WORK + 10, 1, phase);c.put(WORK + 828, 4, packet)
        append(c, 'unsupported_phase_noop')

    fixture = args.out / 'class20-03-action3-source-cases.tsv'
    fixture.write_text('inputs5 prepared12 before67 after67 intents2\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    evidence = {'schema': 'class20-03-action3-source-evidence/1', 'status': 'SOURCE_BACKED_PARTIAL',
                'source_entry': '0x80102c9c', 'code_region_end_exclusive': '0x80102fc0',
                'code_region_sha256': sha(overlay[0x2c9c:0x2fc0]),
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
                'total_cases': len(rows), 'case_counts': counts, 'state_fields': len(FIELDS),
                'prepared_motion_cases': sum(row[5] for row in rows),
                'sound_intent_cases': sum(row[-2] >= 0 for row in rows),
                'release_intent_cases': sum(row[-1] for row in rows),
                'instructions_executed': steps, 'fixture_sha256': sha(fixture.read_bytes()),
                'external_premises': ['Authored selected combat-record byteA; selected record0 fixture context only.',
                    'Explicit target angle; original80031BEC executes.',
                    'Authored three-lane motion delta, wall return and floor result; original fixed accumulator/raw conversion executes.',
                    'Sound engine excluded; instrumented intent180 captured.',
                    'Pool release bitmap excluded; authored work00 clear models only its proved scalar store.'],
                'limits': ['Standalone action body, not detection/hit/common tail/whole AI.',
                    'Forward768 is sourced at80102E6C; motion fixture delta is an external premise, not a faithful motion measurement.',
                    'Release requires real native allocation/lifetime owner before runtime commit.',
                    'Authored fixtures are not live retail parity.',
                    'No owner payload, original instruction listing or table bytes exported.']}
    (args.out / 'class20-03-action3-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'total_cases': len(rows), 'case_counts': counts, 'state_fields': len(FIELDS),
                'prepared_motion_cases': sum(row[5] for row in rows),
                'sound_intent_cases': sum(row[-2] >= 0 for row in rows),
                'release_intent_cases': sum(row[-1] for row in rows),
                      'instructions_executed': steps, 'fixture_sha256': evidence['fixture_sha256']}, indent=2))


if __name__ == '__main__':
    main()

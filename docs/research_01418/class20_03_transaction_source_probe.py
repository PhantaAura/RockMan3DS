#!/usr/bin/env python3
"""Differential scalar fixtures for the bounded class20/03 transaction.

Reads only established owner EXE/ST00 inputs. Output stays owner-local: no
original instructions, tables, assets or payload bytes are serialized. BIOS,
admitted detection and consumers are explicitly authored synthetic premises.
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

WORK = 0x1000
# Corresponds exactly to test_class20_03_callback.cpp values/read_state. All
# values are authored work scalars; optional unknown native ownership is tested
# separately without pretending the original memory holds an optional value.
FIELDS = [(3, 1, False), (9, 1, False), (10, 1, False), (172, 1, False),
          (808, 2, False), (812, 2, True), (816, 2, True), (86, 2, False),
          (102, 2, True), (876, 2, True), (0, 1, False), (8, 1, False),
          (11, 1, False), (12, 1, False), (14, 1, False), (173, 1, False)]
for offset, width, signed in [(20, 2, True), (28, 4, True), (84, 2, False),
                              (44, 2, True), (52, 4, True), (92, 2, False),
                              (384, 2, True), (820, 2, True)]:
    FIELDS += [(offset + i * width, width, signed) for i in range(4)]
FIELDS += [(110, 2, True), (176, 2, True), (178, 2, True), (350, 2, False),
           (810, 2, False), (814, 2, True), (818, 2, True), (828, 4, False),
           (850, 1, False), (851, 1, False), (859, 1, True), (860, 2, True),
           (862, 2, True), (864, 2, True), (866, 2, True), (880, 4, False),
           (884, 4, False)]


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def fields(cpu):
    return [cpu.get(WORK + offset, width, signed) for offset, width, signed in FIELDS]


def set_array(cpu, offset, width, values):
    for i, value in enumerate(values):
        cpu.put(WORK + offset + i * width, width, value)


def sound_hook(cpu):
    # Authored hook records the sound argument in synthetic scratch. These
    # encodings are authored instrumentation, not harvested owner code.
    for i, word in enumerate([(15 << 26) | (8 << 16) | 1,
                              (35 << 26) | (8 << 21) | (9 << 16) | 0x9008,
                              0, (9 << 16) | (10 << 11) | (2 << 6),
                              (10 << 21) | (8 << 16) | (10 << 11) | 33,
                              (43 << 26) | (10 << 21) | (4 << 16) | 0x9000,
                              (9 << 26) | (9 << 21) | (9 << 16) | 1,
                              (43 << 26) | (8 << 21) | (9 << 16) | 0x9008,
                              (31 << 21) | 8, 0]):
        cpu.put(0x8001a050 + i * 4, 4, word)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--dat', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    image = args.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    if args.out.exists():
        raise SystemExit('Refusing to overwrite transaction evidence directory')
    args.out.mkdir(parents=True)
    rows = []
    counts = {'callback': 0, 'state1': 0, 'tail': 0, 'initializer': 0, 'prepared_action2': 0}
    steps = 0

    def fresh():
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[29] = WORK, 0x8000
        for offset, width, value in [(0, 1, 1), (1, 1, 0x20), (2, 1, 3),
                                     (8, 1, 1), (11, 1, 19), (14, 1, 4),
                                     (110, 2, 128), (172, 1, 7), (173, 1, 99),
                                     (176, 2, 128), (178, 2, 127), (350, 2, 13),
                                     (850, 1, 4), (851, 1, 2), (859, 1, -6),
                                     (864, 2, 4)]:
            c.put(WORK + offset, width, value)
        set_array(c, 20, 2, [123, -456, 789, -12345])
        set_array(c, 28, 4, [1234567, -7654321, 0x7fffffff, -0x80000000])
        set_array(c, 84, 2, [11, 4095, 33, 44])
        set_array(c, 44, 2, [-1, -2, -3, -4])
        set_array(c, 52, 4, [-5, -6, -7, -8])
        set_array(c, 92, 2, [51, 52, 53, 54])
        set_array(c, 384, 2, [61, 62, 63, 64])
        set_array(c, 820, 2, [71, 72, 73, 74])
        for address in [0x800322a8, 0x80032488, 0x8002c8cc, 0x800341bc]:
            c.hook(address)
        sound_hook(c)
        return c

    def append(c, kind, inp, init=(65536, 256, 4294967296), prepared=None):
        nonlocal steps
        sample, detection, range_byte, distance, packet, predicate, target = inp
        if sample >= 0:
            c.put(WORK + 810, 2, sample)
            c.hook(0x8007fe50, sample)
        if detection >= 0:
            c.hook(0x8003648c, detection)
        if distance >= 0:
            c.put(WORK + 128, 4, distance)
        if packet >= 0:
            c.put(WORK + 828, 4, packet)
        if predicate >= 0:
            c.hook(0x8001da58, predicate)
        if target >= 0:
            c.hook(0x80031198, target)
        before = fields(c)
        if kind == 2:
            c.r[16], c.r[17] = WORK, WORK + 808
        c.run([0x8010226c, 0x80102600, 0x801026a0, 0x80102410][kind])
        calls = [call['target'] for call in c.calls]
        animation = 0
        effect = proxy = 0
        if kind == 0:
            animation = 1 if '0x800322a8' in calls else 2
            effect = int('0x8002c8cc' in calls)
            proxy = int('0x800341bc' in calls)
        sound_count = c.get(0x9008, 4)
        assert sound_count <= 2
        sound = c.get(0x9000, 4) if sound_count else -1
        secondary_sound = c.get(0x9004, 4) if sound_count == 2 else -1
        prepared_values = [1] + prepared if prepared is not None else [0] + [0] * 10 + [256]
        rows.append([kind] + list(inp) + [1, 1, 1, 1] + list(init) + prepared_values + before + fields(c)
                    + [animation, effect, proxy, sound, secondary_sound])
        counts[['callback', 'state1', 'tail', 'initializer'][kind]] += 1
        steps += c.steps

    # Genuine state1/action0/1 instructions with authored external detection,
    # explicit packet/sample/heading, and source consumers excluded by hooks.
    for action, phase, parameter, detection, timer, sample in itertools.product(
            (0, 1), (0, 1, 2), (0, 0x20, 0x80), (0, 1), (-1, 0, 1), (0, 14)):
        c = fresh()
        for offset, width, value in [(3, 1, parameter), (9, 1, action), (10, 1, phase),
                                     (808, 2, 0), (812, 2, timer), (102, 2, 16),
                                     (860, 2, -32768), (862, 2, -1), (818, 2, 1)]:
            c.put(WORK + offset, width, value)
        append(c, 1, (sample, detection, -1, 0xffffffff, 0, 0, 1))

    # The original detection range rejection itself executes: no detect hook.
    for phase, distance in itertools.product((0, 1, 2), (0x00f00000, 0xffffffff)):
        c = fresh()
        c.put(WORK + 10, 1, 1); c.put(WORK + 812, 2, 10)
        c.put(WORK + 851, 1, phase)
        append(c, 1, (-1, -1, 15, distance, 0, -1, -1))

    # Whole callback snapshots, request comparison, effect/proxy ordering and
    # packet finalization; high packet bit with zero payload is harmless.
    for work0, flags, phase, damage in itertools.product(
            (0, 1, 2, 3), (0, 2, 4, 6), (0, 1), (0, 1)):
        c = fresh()
        for offset, width, value in [(0, 1, work0), (3, 1, 0x60), (10, 1, phase),
                                     (808, 2, flags), (812, 2, 10)]:
            c.put(WORK + offset, width, value)
        append(c, 0, (0x1234, -1, -1, -1, 0x20000000 | damage, -1, -1))

    # Stat-independent hit routes retain original low-halfword subtraction,
    # including authored 24-bit damage inputs that wrap to positive HP.
    for packet in (1, 0x01000011, 0x20000001, 0x20000010, 0x20010000, 0x20ffff81):
        c = fresh()
        c.put(WORK + 3, 1, 0x60); c.put(WORK + 10, 1, 1)
        c.put(WORK + 808, 2, 2); c.put(WORK + 812, 2, 10)
        append(c, 0, (0x1234, -1, -1, -1, packet, -1, -1))

    # Action0 phase2 start uses original distance inclusive threshold,
    # nonzero packet payload, or the explicit predicate when parameter20.
    for parameter, distance, packet, predicate in itertools.product(
            (0, 0x20), (0x0031ffff, 0x00320000), (0, 0x01000001), (0, 1)):
        c = fresh()
        c.put(WORK + 3, 1, parameter); c.put(WORK + 10, 1, 2)
        append(c, 1, (-1, 0, -1, distance, packet, predicate, -1))

    # Common tail covers restore action, health51 edge, detection priority,
    # all16 random selectors and signed/wrapped nonzero decrements.
    for action, detection, hp, sample, timer in itertools.product(
            (1, 2, 6, 9), (0, 0x10), (50, 51), range(16), (-32768, -1, 0, 1)):
        c = fresh()
        for offset, width, value in [(3, 1, 0xc0), (9, 1, action), (10, 1, 3),
                                     (808, 2, detection | 1), (110, 2, hp),
                                     (818, 2, timer), (860, 2, timer), (862, 2, timer)]:
            c.put(WORK + offset, width, value)
        append(c, 2, (sample, -1, -1, -1, -1, -1, -1))

    # Explicit owner stats-selector0 premise is used only for these original
    # initializer cases, never as a native default. The native helper takes
    # already-owned HP/profile scalars and leaves absent ownership unknown.
    for parameter, flags0c, floor in itertools.product((0, 0x20, 0x40, 0x60), (0, 0x10), (-1, 32768)):
        c = fresh()
        c.put(WORK + 3, 1, parameter); c.put(WORK + 12, 1, flags0c)
        c.put(0x80098844, 4, 0x8008f060)
        c.hook(0x8002f9c4); c.hook(0x80048e6c, floor)
        # Hook's signed16 immediate is the explicit floor result here.
        source_floor = floor if floor < 32768 else floor - 65536
        record = c.get(0x8008f060 + 3 * 4, 4) + (16 if flags0c & 0x10 else 0)
        initial_hp = c.get(record + 8, 1) * 16
        profile = c.get(record + 15, 1)
        append(c, 3, (-1, -1, -1, -1, -1, -1, -1), (initial_hp, profile, source_floor))

    # Prepared action2: original accumulator/raw conversion execute with an
    # authored motion delta. Wall correction is excluded here; root's separate
    # contact proof supplies actual corrected work/velocity/byte74 ownership.
    for phase, detection, floor_delta, contact, heading, turn, cue in itertools.product(
            (0, 1, 2), (0, 1), (-33, -32, 0, 32, 33),
            (0, 0x100, 0x1000, 0x80000000), (0, 4095), (-16, 16), (0, 1)):
        c = fresh()
        raw = [101, -7, -202, 1234]
        fixed = [0x7ffffffe, -7 * 65536 + 12345, -202 * 65536 - 2222, 0x24681357]
        delta = [53, -16385, 65535, 0x11111111]
        set_array(c, 20, 2, raw); set_array(c, 44, 2, raw)
        set_array(c, 28, 4, fixed); set_array(c, 52, 4, fixed)
        for offset, width, value in [(9, 1, 2), (10, 1, phase), (166, 1, cue),
                                     (86, 2, heading), (102, 2, turn), (816, 2, 99)]:
            c.put(WORK + offset, width, value)
        authored_store_hook(c, 0x80030058, [(4, i * 4, 4, v) for i, v in enumerate(delta)])
        authored_store_hook(c, 0x8004b414, [], contact)
        authored_store_hook(c, 0x80048e6c, [], raw[1] + floor_delta)
        moved_fixed = [signed32(fixed[i] + delta[i]) for i in range(3)] + fixed[3:]
        moved_raw = [signed16((v >> 16) + ((v >> 15) & 1)) for v in moved_fixed[:3]] + raw[3:]
        append(c, 1, (-1, detection, -1, -1, 0, -1, -1),
               prepared=moved_raw + moved_fixed + [raw[1] + floor_delta, contact, cue])
        counts['prepared_action2'] += 1

    # Original full callback with a bounded hit followed by action2 cue1:
    # both sound intents must survive in order before outer packet clear.
    c = fresh()
    set_array(c, 20, 2, raw); set_array(c, 28, 4, fixed)
    for offset, width, value in [(3, 1, 0x20), (9, 1, 2), (10, 1, 0),
                                 (166, 1, 1), (808, 2, 2)]:
        c.put(WORK + offset, width, value)
    authored_store_hook(c, 0x80030058, [(4, i * 4, 4, v) for i, v in enumerate(delta)])
    authored_store_hook(c, 0x8004b414, [], 0)
    authored_store_hook(c, 0x80048e6c, [], raw[1])
    append(c, 0, (0x1234, -1, -1, -1, 0x20000001, -1, -1),
           prepared=moved_raw + moved_fixed + [raw[1], 0, 1])
    counts['prepared_action2'] += 1

    fixture = args.out / 'class20-03-transaction-source-cases.tsv'
    fixture.write_text('kind inputs14 prepared12 before65 after65 consumer_intents5\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    evidence = {'schema': 'class20-03-transaction-source-cases/1',
                'status': 'SOURCE_BACKED_PARTIAL', 'case_counts': counts,
                'total_cases': len(rows),
                'state_fields_per_case': len(FIELDS), 'instructions_executed': steps,
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
                'fixture_sha256': sha(fixture.read_bytes()),
                'hooks': ['Explicit BIOS sample and admitted detection scalar when supplied.',
                          'Animation/effect/proxy consumers excluded; source call order is examined.',
                          'Authored sound instrumentation records a call argument; sound engine excluded.',
                          'Explicit target heading/predicate when supplied; their solvers excluded.',
                          'Initializer resource setup/floor/pose excluded; explicit stats-selector0 context.'],
                'limits': ['Scalar authored premises, not a live encounter or whole retail AI.',
                           'Detection rejection runs original EXE instructions and reads profile4 range byte.',
                           'Admitted detection, BIOS seed/state, hit delivery and consumer side effects remain external.',
                           'Action2 fixtures use authored motion/contact premises; actual native ownership is independently gated.',
                           'Profile-dependent/lethal hits, actions3..10 and defeat lifetime are held by native reducer.',
                           'No original bytes, instruction listings, profile tables or assets exported.']}
    (args.out / 'class20-03-transaction-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'case_counts': counts, 'state_fields_per_case': len(FIELDS),
                      'instructions_executed': steps, 'fixture_sha256': evidence['fixture_sha256']}, indent=2))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Bounded original enemy callback cases with explicit external scalar inputs.

No original bytes, assets, table payloads or instruction listings are emitted.
Motion/GTE, contact, detection, BIOS RNG and animation consumers are declared
hooks. Original callback ordering, action2 arithmetic and transition tails run.
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

WORK = 0x1000


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def signed16(v):
    v &= 65535
    return v - 65536 if v & 32768 else v


def signed32(v):
    v &= 0xffffffff
    return v - 0x100000000 if v & 0x80000000 else v


def set_words(c, offset, values):
    for i, value in enumerate(values):
        c.put(WORK + offset + i * 4, 4, value)


def set_shorts(c, offset, values):
    for i, value in enumerate(values):
        c.put(WORK + offset + i * 2, 2, value)


def authored_store_hook(c, address, writes, return_value=0):
    """Authored MIPS stub in synthetic memory, preserving owner files.

    Each write is (base register, byte offset, width, literal value). The
    script contains authored encodings only, never harvested instructions.
    """
    words = []
    for register, offset, width, value in writes:
        words += [(15 << 26) | (8 << 16) | ((value >> 16) & 65535),
                  (13 << 26) | (8 << 21) | (8 << 16) | (value & 65535),
                  ({1: 40, 2: 41, 4: 43}[width] << 26) |
                  (register << 21) | (8 << 16) | (offset & 65535)]
    words += [(15 << 26) | (2 << 16) | ((return_value >> 16) & 65535),
              (13 << 26) | (2 << 21) | (2 << 16) | (return_value & 65535),
              (31 << 21) | 8, 0]
    for i, word in enumerate(words):
        c.put(address + i * 4, 4, word)


def selected_fields(c):
    fields = [('parameter03', 3, 1, False), ('action09', 9, 1, False),
              ('phase0A', 10, 1, False), ('aux0B', 11, 1, False),
              ('restore0E', 14, 1, False), ('animationAC', 172, 1, False),
              ('flags328', 808, 2, False), ('timer32C', 812, 2, True),
              ('work330', 816, 2, True), ('work332', 818, 2, True),
              ('work35C', 860, 2, True), ('work35E', 862, 2, True),
              ('heading56', 86, 2, False), ('contact370', 880, 4, False),
              ('packet33C', 828, 4, False)]
    result = {name: c.get(WORK + off, size, signed)
              for name, off, size, signed in fields}
    result['raw'] = [c.get(WORK + 20 + i * 2, 2, True) for i in range(4)]
    result['fixed'] = [c.get(WORK + 28 + i * 4, 4, True) for i in range(4)]
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--dat', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    if a.out.exists() and any(a.out.iterdir()):
        raise SystemExit('Refusing to overwrite evidence directory')
    a.out.mkdir(parents=True, exist_ok=True)
    steps = 0
    results = {}

    def fresh():
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[29] = WORK, 0x8000
        return c

    action2 = []
    for phase, detection, floor_delta, contact, heading, turn in itertools.product(
            (0, 1, 2), (0, 0x10), (-33, -32, 0, 32, 33, 32767),
            (0, 0x100, 0x1000, 0x80000000), (0, 4095), (-16, 16)):
        c = fresh()
        raw = [101, -7, -202, 1234]
        fixed = [0x7ffffffe, -7 * 65536 + 12345, -202 * 65536 - 2222, 0x24681357]
        delta = [53, -16385, 65535, 0x11111111]
        c.put(WORK + 9, 1, 2); c.put(WORK + 10, 1, phase)
        c.put(WORK + 11, 1, 17); c.put(WORK + 14, 1, 12)
        c.put(WORK + 172, 1, 7); c.put(WORK + 166, 1, 1)
        c.put(WORK + 808, 2, detection); c.put(WORK + 816, 2, 99)
        c.put(WORK + 86, 2, heading); c.put(WORK + 102, 2, turn)
        set_shorts(c, 20, raw); set_shorts(c, 44, raw)
        set_words(c, 28, fixed); set_words(c, 52, fixed)
        authored_store_hook(c, 0x80030058,
                            [(4, i * 4, 4, v) for i, v in enumerate(delta)])
        c.hook(0x8004b414, contact)
        # A return-only hook has a signed16 immediate; use full scalar stub
        # for contact high bits and floor sentinel inputs.
        authored_store_hook(c, 0x8004b414, [], contact)
        authored_store_hook(c, 0x80048e6c, [], raw[1] + floor_delta)
        c.hook(0x8001a050)
        c.run(0x80102ae0)
        actual = selected_fields(c)
        expected = {'phase0A': phase, 'aux0B': 17, 'restore0E': 12,
                    'animationAC': 7, 'work330': 99, 'heading56': heading,
                    'raw': raw[:], 'fixed': fixed[:], 'action09': 2,
                    'flags328': detection, 'contact370': 0}
        if phase <= 1:
            if phase == 0:
                expected.update(phase0A=1, animationAC=1, work330=0)
            expected['heading56'] = (heading + turn) & 4095
            expected['fixed'] = [signed32(fixed[i] + delta[i]) for i in range(3)] + fixed[3:]
            expected['raw'] = [signed16((v >> 16) + ((v >> 15) & 1))
                               for v in expected['fixed'][:3]] + raw[3:]
            floor = signed16(raw[1] + floor_delta)
            expected['raw'][1] = floor
            expected['fixed'][1] = signed32((raw[1] + floor_delta) << 16)
            expected['contact370'] = contact
            rollback = abs(floor - raw[1]) >= 33
            if rollback:
                expected['raw'], expected['fixed'] = raw[:], fixed[:]
            if rollback or contact & 0xf00:
                expected.update(action09=1, phase0A=0, aux0B=0,
                                restore0E=4 if detection else 2)
            elif detection:
                expected['flags328'] |= 1
        for field, value in expected.items():
            assert actual[field] == value, (field, value, actual[field])
        action2.append({'input': {'phase': phase, 'detection': detection,
                       'floor_delta': floor_delta, 'contact': contact,
                       'heading': heading, 'turn': turn}, 'after': actual})
        steps += c.steps
    results['action2'] = action2

    transitions = []
    for action, detection, hp, random, timer in itertools.product(
            (1, 2, 6, 9), (0, 0x10), (50, 51), range(16), (-1, 0, 1)):
        c = fresh(); c.r[16], c.r[17] = WORK, WORK + 808
        c.put(WORK + 3, 1, 0xc0); c.put(WORK + 9, 1, action)
        c.put(WORK + 10, 1, 3); c.put(WORK + 11, 1, 9)
        c.put(WORK + 14, 1, 4); c.put(WORK + 808, 2, detection | 1)
        c.put(WORK + 110, 2, hp); c.put(WORK + 810, 2, random)
        for offset in (818, 860, 862): c.put(WORK + offset, 2, timer)
        c.run(0x801026a0)
        actual = selected_fields(c)
        selected = 2 if random < 14 else 0
        expected_action = 4 if action == 1 else 5 if action == 6 and hp < 51 else 9 if detection else selected
        assert actual['action09'] == expected_action
        assert actual['parameter03'] == 0x80
        assert actual['phase0A'] == 0 and actual['aux0B'] == 0
        assert actual['flags328'] == detection
        for field in ('work332', 'work35C', 'work35E'):
            assert actual[field] == (timer - 1 if timer else 0)
        transitions.append({'input': {'action': action, 'detection': detection,
                            'hp': hp, 'random': random, 'timer': timer}, 'after': actual})
        steps += c.steps
    results['state1_transition_tail'] = transitions

    callbacks = []
    for work0, parameter, phase, flags, timer in itertools.product(
            (0, 1, 2, 3), (0, 0x20), (0, 1), (0, 4), (0, 1)):
        c = fresh()
        c.put(WORK, 1, work0); c.put(WORK + 1, 1, 0x20)
        c.put(WORK + 2, 1, 3); c.put(WORK + 3, 1, parameter)
        c.put(WORK + 8, 1, 1); c.put(WORK + 9, 1, 0)
        c.put(WORK + 10, 1, phase); c.put(WORK + 172, 1, 7)
        c.put(WORK + 808, 2, flags); c.put(WORK + 812, 2, timer)
        c.put(WORK + 828, 4, 0x02000000)
        raw = [123, -456, 789, -12345]
        fixed = [1234567, -7654321, 0x7fffffff, -0x80000000]
        angles = [11, 22, 33, 44]
        set_shorts(c, 20, raw); set_words(c, 28, fixed); set_shorts(c, 84, angles)
        set_shorts(c, 384, [51, 52, 53, 54])
        c.hook(0x8007fe50, 0x1234)
        c.hook(0x8003648c, 0)
        for address in (0x800322a8, 0x80032488, 0x8002c8cc, 0x800341bc): c.hook(address)
        c.run(0x8010226c)
        targets = [x['target'] for x in c.calls]
        reset = phase == 0
        assert ('0x800322a8' in targets) == reset
        assert ('0x80032488' in targets) == (not reset)
        assert ('0x8002c8cc' in targets) == bool(work0 & 2)
        assert ('0x800341bc' in targets) == bool(work0 & 1 and flags & 4)
        assert ('0x8003648c' in targets) == (parameter == 0)
        assert c.get(WORK + 828, 4) == 0
        assert c.get(WORK + 810, 2) == 0x1234
        assert c.get(WORK + 173, 1) == 7
        assert [c.get(WORK + 44 + i * 2, 2, True) for i in range(4)] == raw
        assert [c.get(WORK + 52 + i * 4, 4, True) for i in range(4)] == fixed
        assert [c.get(WORK + 92 + i * 2, 2, True) for i in range(4)] == angles
        if work0 & 1 and flags & 4:
            assert [c.get(WORK + 820 + i * 2, 2) for i in range(4)] == [51, 52, 53, 54]
        callbacks.append({'input': {'work0': work0, 'parameter': parameter,
                          'phase': phase, 'flags': flags, 'timer': timer},
                          'after': selected_fields(c), 'direct_calls': c.calls,
                          'indirect_calls': c.indirect})
        steps += c.steps
    results['callback'] = callbacks

    detection_cases = []
    for profile_range, phase, distance in itertools.product(
            (0, 1, 15, 255), (0, 1, 2), (0, 1, 0x0fffff, 0x100000, 0xffffffff)):
        c = fresh(); c.r[4], c.r[5] = WORK + 844, WORK
        c.put(WORK + 850, 1, 0); c.put(WORK + 851, 1, phase)
        # Deliberately authored profile range byte, not exported owner table.
        c.put(0x800899ae, 1, profile_range)
        c.put(WORK + 128, 4, distance)
        if distance >= profile_range << 20:
            c.run(0x8003648c)
            assert c.r[2] == 0 and c.get(WORK + 851, 1) == 0
            detection_cases.append({'profile_range': profile_range, 'phase': phase,
                                    'work80': distance, 'result': 0, 'after_phase': 0})
            steps += c.steps
    results['detection_range_reject'] = detection_cases

    attacks = []
    for alternate, allocated in itertools.product((0, 1), (0, 0x5000)):
        c = fresh(); c.put(WORK + 814, 2, alternate)
        set_words(c, 28, [1234567, -7654321, 22334455, 0])
        c.put(WORK + 86, 2, 1023)
        c.hook(0x80031518, allocated); c.hook(0x8001a050)
        # Motion displacement helper is excluded at the child-offset call.
        c.hook(0x8002ffc4)
        c.run(0x80104078)
        child = None
        if allocated:
            child = {'work0': c.get(allocated, 1), 'subtype2': c.get(allocated + 2, 1),
                     'parameter3': c.get(allocated + 3, 1),
                     'fixed': [c.get(allocated + 28 + i * 4, 4, True) for i in range(3)],
                     'raw_before_displacement': [c.get(allocated + 20 + i * 2, 2, True) for i in range(3)],
                     'heading64': c.get(allocated + 100, 2),
                     'work9C': c.get(allocated + 156, 4), 'workA0': c.get(allocated + 160, 4)}
            assert child['subtype2'] == (9 if alternate else 6)
            assert child['parameter3'] == (3 if alternate else 0)
            assert child['fixed'] == [1234567, -7654321 - (80 if alternate else 60) * 65536, 22334455]
            assert child['heading64'] == 1023
        assert c.get(WORK + 884, 4) == allocated
        attacks.append({'work32E': alternate, 'allocator_result': bool(allocated),
                        'child': child, 'direct_calls': c.calls})
        steps += c.steps
    results['attack_child_wrapper'] = attacks

    casefile = a.out / 'class20-03-callback-source-cases.json'
    casefile.write_text(json.dumps(results, indent=2) + '\n')
    regions = [(0x8010226c, 0x80102410), (0x80102600, 0x80102884),
               (0x80102ae0, 0x80102c9c), (0x80104078, 0x801041b0)]
    evidence = {'schema': 'class20-03-callback-source-cases/1',
                'status': 'SOURCE_BACKED_PARTIAL',
                'case_counts': {k: len(v) for k, v in results.items()},
                'instructions_executed': steps, 'exe_sha256': sha(image),
                'overlay_sha256': sha(overlay), 'fixture_sha256': sha(casefile.read_bytes()),
                'regions': [{'start': hex(lo), 'end_exclusive': hex(hi),
                             'sha256': sha(overlay[lo - 0x80100000:hi - 0x80100000])}
                            for lo, hi in regions],
                'hooks': {'action2': ['80030058 supplied authored fixed delta;8002FEA4 accumulator and80030150 execute original instructions',
                                    '8004B414 supplied wall status; no wall correction in these premises',
                                    '80048E6C supplied floor scalar', '8001A050 sound excluded'],
                          'callback': ['8007FE50 supplied BIOS RNG scalar', '8003648C supplied false detection',
                                       '800322A8/80032488 animation consumers excluded',
                                       '8002C8CC/800341BC proxy/effect consumers excluded'],
                          'state1_transition_tail': ['Entry801026A0 bypasses detection/hit/action; original selector executes'],
                          'detection_range_reject': ['No call hooks; one authored range profile byte in synthetic memory'],
                          'attack_child_wrapper': ['80031518 supplied allocation pointer', '8001A050 sound excluded',
                                                   '8002FFC4 child displacement excluded;80030150 original conversion executes']},
                'limits': ['Authored premises, not retail live traces or complete gameplay parity.',
                           'GTE/BIOS/geometry helpers are not emulated or replaced by claimed retail behavior.',
                           'Contact wrapper has position side effects beyond status bits.',
                           'Source range/detection profile and combat tables are not serialized.',
                           'No owner code bytes, table payloads, model/animation assets exported.']}
    (a.out / 'class20-03-callback-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'case_counts': evidence['case_counts'], 'instructions_executed': steps,
                      'fixture_sha256': evidence['fixture_sha256']}, indent=2))


if __name__ == '__main__':
    main()

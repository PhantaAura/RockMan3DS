#!/usr/bin/env python3
"""Original action0/1 and selector scalar fixtures; no game payload exported."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


FIELDS = [('parameter03', 3, 1, False), ('action09', 9, 1, False),
          ('phase0A', 10, 1, False), ('animationAC', 172, 1, False),
          ('flags328', 808, 2, False), ('timer32C', 812, 2, True),
          ('work330', 816, 2, True), ('heading56', 86, 2, False),
          ('turn66', 102, 2, True), ('work36C', 876, 2, True)]


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


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
    if a.out.exists():
        raise SystemExit('Refusing to overwrite evidence directory')
    a.out.mkdir(parents=True)
    rows = []
    instructions = 0

    def run(kind, state, work80=0, packet=0, predicate=0, target=0, random=0):
        nonlocal instructions
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[29] = 0x1000, 0x8000
        for value, (_, offset, width, _) in zip(state, FIELDS):
            c.put(0x1000 + offset, width, value)
        c.put(0x1080, 4, work80)
        c.put(0x133c, 4, packet)
        c.put(0x132a, 2, random)
        if kind == 0:
            c.hook(0x8001da58, predicate)
            c.run(0x80102884 if state[1] == 0 else 0x801029b4)
        elif kind == 1:
            c.run(0x80103fac)
        else:
            raise ValueError('bad operation')
        after = [c.get(0x1000 + offset, width, signed)
                 for _, offset, width, signed in FIELDS]
        rows.append([kind, *state, work80, packet, predicate, target, random, *after])
        instructions += c.steps

    initial = [0, 0, 0, 7, 14, 99, 77, 2048, 33, 22]
    for parameter, phase, timer, flags in itertools.product(
            (0, 0x80, 0xa0), (0, 1, 3), (-32768, -1, 0, 1, 29, 32767), (0, 14, 0x20)):
        state = initial.copy()
        state[0], state[2], state[5], state[4] = parameter, phase, timer, flags
        run(0, state)
    for parameter, distance, packet, flag in itertools.product(
            (0, 0x80, 0xa0, 0x20), (0, 0x31fffe, 0x31ffff, 0x320000, 0xffffffff),
            (0, 1, 0x02000000, 0x08000010), (0, 1)):
        state = initial.copy()
        state[0], state[2] = parameter, 2
        run(0, state, distance, packet, flag)

    # Preserve the original turn implementation. Only80031198 (world-position
    # angle construction) is hooked with a declared scalar target angle.
    def turn_run(state, target):
        nonlocal instructions
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[29] = 0x1000, 0x8000
        for value, (_, offset, width, _) in zip(state, FIELDS):
            c.put(0x1000 + offset, width, value)
        c.hook(0x80031198, target)
        c.run(0x801029b4)
        after = [c.get(0x1000 + offset, width, signed)
                 for _, offset, width, signed in FIELDS]
        rows.append([0, *state, 0, 0, 0, target, 0, *after])
        instructions += c.steps

    for phase, heading, target, limit, timer in itertools.product(
            (0, 1, 2), (0, 1, 1024, 2048, 4095),
            (0, 1, 1024, 2048, 2049, 4095), (-16, 0, 16, 256), (-1, 0, 1, 8)):
        state = initial.copy()
        state[1], state[2], state[7], state[8], state[5] = 1, phase, heading, limit, timer
        turn_run(state, target)
    for random in range(256):
        run(1, initial, random=random)

    fixture = a.out / 'class20-03-action-source-cases.tsv'
    columns = ['kind', *('before_' + f[0] for f in FIELDS), 'work80', 'packet33C',
               'predicate', 'target_heading', 'random32A', *('after_' + f[0] for f in FIELDS)]
    fixture.write_text('\t'.join(columns) + '\n' +
                       ''.join('\t'.join(map(str, r)) + '\n' for r in rows))
    regions = [(0x80102884, 0x801029b4), (0x801029b4, 0x80102ae0),
               (0x80103fac, 0x80104078)]
    evidence = {'schema': 'class20-03-action-source-cases/1',
                'status': 'SOURCE_BACKED_PARTIAL', 'fixture_sha256': sha(fixture.read_bytes()),
                'case_count': len(rows), 'scalar_output_fields': len(FIELDS),
                'instructions_executed': instructions,
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
                'code_regions': [{'start': hex(lo), 'end_exclusive': hex(hi),
                                  'sha256': sha(overlay[lo - 0x80100000:hi - 0x80100000])}
                                 for lo, hi in regions],
                'hooks': {'action0_phase2_parameter20': '8001DA58 supplied explicit boolean',
                          'action1': '80031198 supplied explicit scalar target heading;80031BEC executes original code'},
                'limits': ['Action0/1 dispatch functions only, not full callback or state1 tick.',
                           'Detection helper8003648C and hit packet lifecycle excluded.',
                           'Work80 arithmetic has separate675-case evidence; full callback ownership is not established here.',
                           'World-position atan helper and RNG owner/seeding not ported.',
                           'Generic animation advance, motion/collision, action2+ and attack child excluded.',
                           'Fixtures are synthetic state premises, not live retail parity traces.',
                           'No original bytes, assets or source function tables serialized.']}
    (a.out / 'class20-03-action-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    distances = []
    for actor_x, actor_z, player_x, player_z in itertools.product(
            (-32768, -32767, -3, -1, 0, 1, 3, 32766, 32767),
            (-32768, -1, 0, 1, 32767), (-32768, -1, 0, 1, 32767), (-32768, 0, 32767)):
        c = OverlayProbe(image, base, b'')
        c.r[5], c.r[9], c.r[10], c.r[11], c.r[29] = 0x1000, 0, 0xffffffff, 0x10000000, 0x8000
        c.put(0x1014, 2, actor_x)
        c.put(0x1018, 2, actor_z)
        c.put(0x8000, 2, player_x)
        c.put(0x8004, 2, player_z)
        c.run(0x800360f0, 0x80036148)
        distances.append([actor_x, actor_z, player_x, player_z, c.get(0x1080, 4)])
    distance_fixture = a.out / 'class20-03-distance-source-cases.tsv'
    distance_fixture.write_text('actor_x\tactor_z\tplayer_x\tplayer_z\twork80\n' +
                                ''.join('\t'.join(map(str, r)) + '\n' for r in distances))
    (a.out / 'class20-03-distance-source-evidence.json').write_text(json.dumps({
        'schema': 'class20-03-distance-source-cases/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'case_count': len(distances), 'fixture_sha256': sha(distance_fixture.read_bytes()),
        'exe_sha256': sha(image), 'source_owner': '0x8003606c',
        'region_start': '0x800360f0', 'end_exclusive': '0x80036148',
        'class20_pool': '0x8009f108', 'work_stride': 960,
        'source_inputs': 'signed16 raw X/Z; player slot0 first loop',
        'hooks': [], 'limits': ['Arithmetic store subregion only; synthetic authored coordinates.',
                                'Full generic update/callback ordering remains a separate parity gate.']
    }, indent=2) + '\n')
    print(f'{len(rows)} original-code action/selector cases; {len(FIELDS)} fields')
    print(f'{len(distances)} original-code distance store cases')


if __name__ == '__main__':
    main()

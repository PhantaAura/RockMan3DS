#!/usr/bin/env python3
"""Read-only original action3 contact preparation and pointed sound producer.

GTE delta/target angle are explicit premises. Original turn, accumulator/raw
rounding, contact/floor and source sound queue writer execute. Owner-stage
fixtures remain private; released fixtures contain authored contexts only.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01415'), str(SOURCE / 'docs/research_01417'), str(SOURCE / 'docs/research_01418')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
from class20_03_callback_source_probe import authored_store_hook, signed16, signed32
from contact_source_probe import synthetic_stage

WORK, BOUNDS = 0x1000, 0x2000
AUTHORED_BOUNDS = [-60, 60, -60, 60, -128, 0]


def sha(b): return hashlib.sha256(b).hexdigest()


def write_array(c, off, width, values):
    for i, value in enumerate(values): c.put(WORK + off + i * width, width, value)


def state(c):
    return [c.get(WORK + 20 + i * 2, 2, True) for i in range(4)] + \
           [c.get(WORK + 28 + i * 4, 4, True) for i in range(4)] + \
           [c.get(WORK + 68 + i * 2, 2, True) for i in range(3)] + [c.get(WORK + 116, 1)]


def seed_work(c, raw, fixed, phase, timer, limit, heading):
    for off, width, value in [(0, 1, 3), (1, 1, 0x20), (2, 1, 3), (3, 1, 0),
                              (8, 1, 1), (9, 1, 3), (10, 1, phase), (11, 1, 77),
                              (86, 2, heading), (100, 2, limit), (116, 1, 1),
                              (120, 4, BOUNDS), (812, 2, timer), (828, 4, 0)]:
        c.put(WORK + off, width, value)
    for off, width, values in [(20, 2, raw), (28, 4, fixed), (44, 2, raw), (52, 4, fixed),
                               (68, 2, [321, -654, 987])]: write_array(c, off, width, values)


def preparation(c, raw, fixed, phase, timer, limit, heading, target, sin, cos, bounds):
    seed_work(c, raw, fixed, phase, timer, limit, heading)
    for i, value in enumerate(bounds): c.put(BOUNDS + i * 2, 2, value)
    c.r[4], c.r[29], c.r[31] = WORK, 0x700000, 0
    c.hook(0x80031198, target)
    # This declared motion delta owns the GTE boundary. The original helper
    # consumes it and executes the complete accumulator/raw conversion itself.
    delta = [-sin * 768, 0, -cos * 768]
    authored_store_hook(c, 0x80030058, [(4, i * 4, 4, value) for i, value in enumerate(delta)])
    c.run(0x80102c9c, 0x80102ea4)  # after floor return, before rawY/fixedY stores
    return [signed32(c.get(WORK + 880, 4)), signed32(c.r[2])] + state(c) + \
           [c.get(WORK + 86, 2), c.get(WORK + 876, 2, True)]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--dat', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    args = p.parse_args()
    if args.out.exists(): raise SystemExit('Refusing to overwrite action3 contact proof')
    args.out.mkdir(parents=True)
    image = args.exe.read_bytes(); assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    resources = list(iter_resources(args.dat / 'ST00.BIN'))
    overlay = next(r['payload'] for r in resources if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    rows, stage_rows, sounds, steps = [], [], [], 0
    points = [(-180, -60, 0), (180, -60, 0), (0, -60, -180), (0, -60, 180),
              (-120, -60, 0), (0, -60, 0), (0, -128, 0), (0, 256, 0)]
    for phase, timer, limit, trig, point, code in itertools.product(
            (3, 4), (1, 3, 4, -32768), (0, 16), ((0, 4096), (4096, 0), (-3072, 2048)),
            points, (1, 3, 8)):
        c = OverlayProbe(image, base, overlay); synthetic_stage(c, 1, code)
        raw = list(point) + [1234]
        fixed = [v * 65536 + (12345 if i != 1 else 0) for i, v in enumerate(raw)]
        heading, target = 4095, 2048
        result = preparation(c, raw, fixed, phase, timer, limit, heading, target, *trig, AUTHORED_BOUNDS)
        rows.append([phase, timer, limit, heading, target, *trig, code, 0, 0] + AUTHORED_BOUNDS +
                    raw + fixed + [321, -654, 987, 1] + result)
        steps += c.steps

    # Original producers and original8001A050 queue writer execute together.
    # Only action2's later motion/contact are excluded: its position must be
    # queued before those stores. A deliberately different post-motion position
    # proves which point was copied, including the fourth short lane.
    for action, raw, full in itertools.product((2, 3),
            ([101, -202, 303, 444], [-32768, 32767, -1, -12345], [0, 0, 0, -32768]), (0, 1)):
        c = OverlayProbe(image, base, overlay)
        fixed = [v * 65536 for v in raw]
        seed_work(c, raw, fixed, 1 if action == 2 else 2, 1, 16, 0)
        c.put(WORK + 9, 1, action); c.put(WORK + 166, 1, 1)
        c.put(0x80098938, 4, 0x800be6d8 if full else 0x9000)
        for off, value in [(0, 0x12345678), (4, 0x11223344), (8, 0x55667788), (12, 0x10203040), (16, 0x50607080)]:
            c.put(0x9000 + off, 4, value)
        authored_store_hook(c, 0x80030058, [(4, 0, 4, 655360), (4, 4, 4, 0), (4, 8, 4, -655360)])
        authored_store_hook(c, 0x8004b414, [], 0); authored_store_hook(c, 0x80048e6c, [], raw[1])
        c.r[4], c.r[29], c.r[31] = WORK, 0x700000, 0
        c.run(0x80102ae0 if action == 2 else 0x80102c9c)
        expected_id = 176 if action == 2 else 180
        if not full:
            assert c.get(0x9000, 1) == 1 and c.get(0x9002, 2) == expected_id
            assert c.get(0x9004, 4) == (raw[0] & 65535) | ((raw[1] & 65535) << 16)
            assert c.get(0x9008, 4) == (raw[2] & 65535) | ((raw[3] & 65535) << 16)
            assert c.get(0x900c, 4) == 0x10203040 and c.get(0x9010, 4) == 0x50607080
            assert c.get(0x80098938, 4) == 0x9014
        else:
            assert c.get(0x9000, 4) == 0x12345678 and c.get(0x80098938, 4) == 0x800be6d8
        sounds.append([action, full] + raw + [c.get(0x9000, 1), c.get(0x9002, 2), c.get(0x9004, 4),
                      c.get(0x9008, 4), c.get(0x80098938, 4)] + [c.get(WORK + 20 + i * 2, 2, True) for i in range(4)])
        steps += c.steps

    selected = {}
    for name, load in [('idx', 0x8015c000), ('mdt', 0x80164000), ('stg', 0x80194000), ('hed', 0x8013a000)]:
        selected[name] = next(r for r in resources if r['type'] == 0 and r['load'] == load)
    idx, mdt, stg, hed = [selected[n]['payload'] for n in ('idx', 'mdt', 'stg', 'hed')]
    pointers = []
    for slot in range(512):
        ptr = struct.unpack_from('<I', hed, slot * 8 + 4)[0]
        if not ptr: break
        pointers.append(ptr & 0xffffff)
    admitted = []
    for identity in range(len(idx) // 12):
        ptr = struct.unpack_from('<I', idx, identity * 12 + 8)[0]
        if not ptr or ptr & 0xffffff not in pointers: continue
        header = struct.unpack_from('<I', stg, ptr - 0x80194000 + 24)[0]
        admitted.append((identity, struct.unpack_from('<H', stg, header - 0x80194000 + 2)[0]))
    assert len(admitted) == 994
    sx, sz, width, height, cells = struct.unpack_from('<4BI', mdt, 12)
    owner_bounds = list(struct.unpack_from('<6h', overlay, 0x1bed4))
    for record, phase, timer, target in itertools.product((0x8011a274, 0x8011a28c, 0x8011a2a4), (3, 4), (1, 4), (0, 2048)):
        c = OverlayProbe(image, base, overlay)
        for resource in selected.values():
            for off, value in enumerate(resource['payload']):
                c.put(resource['load'] + off, 1, value); c.put((resource['load'] & 0xffffff) + off, 1, value)
        for identity, seed in admitted:
            c.put(0x8015c006 + identity * 12, 2, seed); c.put(0x15c006 + identity * 12, 2, seed)
        for address, size, value in [(0x1f800040, 1, sx), (0x1f800041, 1, sz), (0x1f800044, 2, width),
                                    (0x1f800046, 2, height), (0x1f800048, 4, cells), (0x1f80004c, 4, 0x8015c000)]:
            c.put(address, size, value)
        raw = list(struct.unpack_from('<4h', overlay, record - 0x80100000 + 12))
        fixed = [v * 65536 + (8192 if i != 1 else 0) for i, v in enumerate(raw)]
        heading = (struct.unpack_from('<H', overlay, record - 0x80100000 + 18)[0] << 4) & 0xffff
        limit = 0
        effective = 256 if phase == 3 and timer < 4 else limit
        t = OverlayProbe(image, base, overlay); t.r[4:8] = [WORK + 20, 0x800b51c4, heading, effective]
        t.r[29] = 0x700000; t.hook(0x80031198, target); t.run(0x80031bec)
        moved_heading = (heading + t.r[2]) & 4095
        trig = struct.unpack_from('<2h', image, 0x80092ec4 - base + 2048 + moved_heading * 4)
        result = preparation(c, raw, fixed, phase, timer, limit, heading, target, *trig, owner_bounds)
        stage_rows.append([phase, timer, limit, heading, target] + owner_bounds + raw + fixed + [321, -654, 987, 1] + result)
        steps += c.steps + t.steps

    fixture = args.out / 'actor-action3-contact-authored.tsv'
    fixture.write_text('phase timer limit heading target sin cos code flags raw6 bounds6 raw4 fixed4 velocity3 state74 wall floor afterraw4 afterfixed4 aftervelocity3 afterstate74 afterheading turn\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    sound_fixture = args.out / 'actor-sound-pointed-authored.tsv'
    sound_fixture.write_text('action full pre_raw4 type id word0 word1 queue_pointer after_raw4\n' +
                             ''.join('\t'.join(map(str, row)) + '\n' for row in sounds))
    private = args.out / 'actor-action3-contact-owner-stage.tsv'
    private.write_text('phase timer limit heading target ownerbounds6 raw4 fixed4 velocity3 state74 wall floor afterraw4 afterfixed4 aftervelocity3 afterstate74 afterheading turn\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in stage_rows))
    evidence = {'schema': 'actor-action3-contact-source/1', 'status': 'SOURCE_BACKED_PARTIAL',
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay), 'authored_contact_cases': len(rows),
                'owner_stage_contact_cases': len(stage_rows), 'sound_producer_cases': len(sounds),
                'instructions_executed': steps, 'authored_fixture_sha256': sha(fixture.read_bytes()),
                'sound_fixture_sha256': sha(sound_fixture.read_bytes()), 'owner_stage_fixture_sha256': sha(private.read_bytes()),
                'source_block_hashes': [{'start': hex(lo), 'end_exclusive': hex(hi), 'sha256': sha(overlay[lo - 0x80100000:hi - 0x80100000])}
                    for lo, hi in [(0x80102b28, 0x80102b44), (0x80102d68, 0x80102dd0), (0x80102e50, 0x80102ea4)]],
                'sound_queue_writer': {'entry': '0x8001a050', 'end_exclusive': '0x8001a0a8',
                    'sha256': sha(image[0x8001a050 - base + 2048:0x8001a0a8 - base + 2048]),
                    'producer_argument': 'a1=actor_work+0x14 before action motion', 'copied_payload': 'two original32bitwords at a1+0 and a1+4',
                    'full_queue_case': 'current pointer==800BE6D8 suppresses enqueue'},
                'external_premises': ['Target80031198 result authored; original80031BEC executes.',
                    'Motion/GTE fixed delta authored or calculated from resident owner trig; original accumulator/rounding executes.',
                    'Synthetic or994-identity materialized stage state; original wall/floor wrappers execute.',
                    'Original176/180 producer and8001A050 queue writer execute; action2 later movement/contact excluded in sound-onlycases.'],
                'limits': ['Contact preparation proves only admitted branches/families; source reducer owns floorstores/rollback.',
                    'No live encounter, whole AI, original audio playback or physicalhardware claim.',
                    'Ownerstage bounds/positions/trig remain outside sourcecheckpoint.']}
    (args.out / 'actor-action3-contact-source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'authored_contact_cases': len(rows), 'owner_stage_contact_cases': len(stage_rows),
                      'sound_producer_cases': len(sounds), 'instructions_executed': steps}, indent=2))


if __name__ == '__main__': main()

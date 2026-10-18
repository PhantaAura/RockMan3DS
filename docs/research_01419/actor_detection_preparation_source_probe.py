#!/usr/bin/env python3
"""Original full detector composition with real helper bodies and contact data.

Authored fixture lookup/profile/trig/geometry values stay source-safe. Owner
materialized stage cases stay private. No executable bytes, asset payloads,
original tables or instruction listings are written to outputs.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path
SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01419'), str(SOURCE / 'docs/research_01418'), str(SOURCE / 'docs/research_01415')]
import actor_detection_math_source_probe as math_probe
from actor_detection_math_source_probe import MathProbe, signed32
from actor_detection_source_probe import detector_fields, put_detector, DETECTOR, WORK, s16
from contact_source_probe import synthetic_stage
from tutorial_actor_identity_probe import SUPPORTED_EXE, OVERLAY_SHA, iter_resources

# The source interpreter itself is authored instrumentation. Record arguments
# at original call entry and return scalar at actual source continuation PCs.
run_source = math_probe.run_source.replace('        w = self.get(pc, 4)', '''        if pc == 0x80049c54:
            self.segment_calls.append({
                'start': [self.get(self.r[4] + i * 2, 2, True) for i in range(4)],
                'end': [self.get(self.r[5] + i * 2, 2, True) for i in range(4)],
                'class': self.r[7], 'result': None})
        if pc in (0x80036770, 0x800367d4):
            self.segment_calls[-1]['result'] = signed32(self.r[2])
        w = self.get(pc, 4)''')
namespace = {}; probe_globals = dict(math_probe.OverlayProbe.run.__globals__); probe_globals['signed32'] = signed32
exec(run_source, probe_globals, namespace)


class PreparationProbe(MathProbe):
    def __init__(self, image, base):
        super().__init__(image, base)
        self.segment_calls = []
    run = namespace['run']


def sha(blob): return hashlib.sha256(blob).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--dat', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists(): raise SystemExit('Refusing to overwrite preparation proof')
    args.out.mkdir(parents=True)
    image = args.exe.read_bytes(); assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    resources = list(iter_resources(args.dat / 'ST00.BIN'))
    overlay = next(r['payload'] for r in resources if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    rows, private, steps, commands = [], [], 0, 0
    counts = {'authored': 0, 'owner_stage': 0, 'phase0': 0, 'phase1': 0, 'phase2': 0,
              'original_query0': 0, 'original_query1': 0}
    bounds = [-25, 25, -35, 35, -70, 10]

    def execute(cpu, heading, trig, lookup_factor, before, distance, actor, player,
                profile, global96, scratch75, query_bounds):
        nonlocal steps, commands
        cpu.r[4], cpu.r[5], cpu.r[29], cpu.r[28] = DETECTOR, WORK, 0x700000, 0x80097864
        cpu.put(WORK + 128, 4, distance); cpu.put(WORK + 86, 2, heading)
        for i, v in enumerate(actor): cpu.put(WORK + 20 + i * 2, 2, v)
        for i, v in enumerate(player): cpu.put(0x800b51c4 + i * 2, 2, v)
        if profile is not None:
            for i, v in enumerate(profile): cpu.put(0x800899ac + 4 * before[3] + i, 1, v)
        if trig is not None:
            index = abs(2048 - s16(heading)) & 4095
            cpu.put(0x80092ec4 + index * 4, 4, (trig[0] & 65535) | ((trig[1] & 65535) << 16))
        if lookup_factor is not None:
            for i in range(192): cpu.put(0x80092d38 + i * 2, 2, lookup_factor)
        for i, v in enumerate(query_bounds): cpu.put(0x80097d0c + i * 2, 2, v)
        cpu.put(0x800bbd96, 2, global96); cpu.put(0x800bbd84, 1, 0)
        cpu.put(0x1f800075, 1, scratch75)
        put_detector(cpu, before)
        cpu.hook(0x8001a008)  # Playback/producer intent excluded from field reducer.
        cpu.run(0x8003648c)
        after, detected = detector_fields(cpu), cpu.r[2]
        notified = cpu.get(0x800bbd84, 1)
        sound = int(any(call['target'] == '0x8001a008' for call in cpu.calls))
        calls = cpu.segment_calls
        if calls:
            assert calls[0]['start'][:3] == calls[0]['end'][:3] and calls[0]['class'] == 65535
            counts['original_query0'] += 1
        if len(calls) == 2:
            assert calls[1]['start'][:3] == calls[1]['end'][:3] and calls[1]['class'] == 65535
            counts['original_query1'] += 1
        assert len(calls) <= 2
        segment_fields = [len(calls)]
        for i in range(2):
            segment_fields += calls[i]['start'][:3] + [calls[i]['result']] if i < len(calls) else [0, 0, 0, 0]
        steps += cpu.steps; commands += cpu.gte_commands
        counts['phase0' if before[4] not in (1, 2) else 'phase1' if before[4] == 1 else 'phase2'] += 1
        return after + [detected, notified, sound] + segment_fields

    player_points = [(0, -60, 0), (0, -60, -100), (0, -60, 100), (-125, -60, 0),
                     (0, -139, 0), (0, 199, 0), (99, 99, 99), (26751, 0, 0)]
    for packed, code, flags, phase, factor, player, countdown in itertools.product(
            (1, -32768), (1, 3, 8, 0x21), (0, 0x8000), (0, 1, 2), (4096, 16384),
            player_points, (1, 2)):
        cpu = PreparationProbe(image, base); synthetic_stage(cpu, packed, code, flags)
        heading, trig = 0, (0, -4096)
        before = [0, -60, 0, 4, phase, 0, 0, -4096, countdown, -6]
        profile = [10, 20, 15]; actor = [0, -60, 0]
        expected = execute(cpu, heading, trig, factor, before, 0, actor, player, profile, 0, 2, bounds)
        rows.append([packed, code, flags, 0] + bounds + [heading] + list(trig) + [factor] +
                    before + [0] + actor + list(player) + profile + [0, 2] + expected)
        counts['authored'] += 1
    # Source-only early rejects bypass every geometric/helper dependency.
    for phase, distance, player in itertools.product((0, 1, 2), (15 << 20, 0xffffffff),
                                                    ((-26751, 0, 0), (26751, 0, 0))):
        cpu = PreparationProbe(image, base); synthetic_stage(cpu, 1)
        before = [0, 0, 0, 4, phase, 0, 0, -4096, 2, -6]
        actor, profile = [0, 0, 0], [10, 20, 15]
        expected = execute(cpu, 0, (0, -4096), 4096, before, distance, actor, player, profile, 0, 2, bounds)
        rows.append([1, 1, 0, 0] + bounds + [0, 0, -4096, 4096] + before + [distance] + actor + list(player) + profile + [0, 2] + expected)
        counts['authored'] += 1

    selected = {}
    for name, load in [('idx', 0x8015c000), ('mdt', 0x80164000), ('stg', 0x80194000), ('hed', 0x8013a000)]:
        found = [r for r in resources if r['type'] == 0 and r['load'] == load]; assert len(found) == 1
        selected[name] = found[0]
    idx, mdt, stg, hed = [selected[name]['payload'] for name in ('idx', 'mdt', 'stg', 'hed')]
    descriptors = []
    for i in range(512):
        pointer = struct.unpack_from('<I', hed, i * 8 + 4)[0]
        if pointer == 0: break
        descriptors.append(pointer & 0xffffff)
    materialized = []
    for i in range(len(idx) // 12):
        descriptor = struct.unpack_from('<I', idx, i * 12 + 8)[0]
        if descriptor == 0 or descriptor & 0xffffff not in descriptors: continue
        header = struct.unpack_from('<I', stg, descriptor - 0x80194000 + 24)[0]
        materialized.append((i, struct.unpack_from('<H', stg, header - 0x80194000 + 2)[0]))
    sx, sz, width, height, cells = struct.unpack_from('<4BI', mdt, 12)
    template = PreparationProbe(image, base)
    template.mem.update({0x80100000 + i: v for i, v in enumerate(overlay)})
    for resource in selected.values():
        template.mem.update({resource['load'] + i: v for i, v in enumerate(resource['payload'])})
        template.mem.update({(resource['load'] & 0xffffff) + i: v for i, v in enumerate(resource['payload'])})
    for identity, seed in materialized:
        template.put(0x8015c006 + identity * 12, 2, seed); template.put(0x15c006 + identity * 12, 2, seed)
    for address, size, value in [(0x1f800040, 1, sx), (0x1f800041, 1, sz),
        (0x1f800044, 2, width), (0x1f800046, 2, height), (0x1f800048, 4, cells), (0x1f80004c, 4, 0x8015c000)]:
        template.put(address, size, value)
    owner_bounds = list(struct.unpack_from('<6h', image, 0x80097d0c - base + 2048))
    for record, phase, delta, countdown in itertools.product((0x8011a274, 0x8011a28c, 0x8011a2a4),
        (0, 1, 2), ((0, 0, 0), (0, 0, -100), (0, 0, 100), (256, 0, 0), (0, -60, -256)), (2, 255)):
        actor = list(struct.unpack_from('<3h', overlay, record - 0x80100000 + 12))
        player = [s16(actor[i] + delta[i]) for i in range(3)]
        origin = [actor[0], s16(actor[1] - 96), actor[2]]
        vector = [s16(player[i] - origin[i]) for i in range(3)]
        before = origin + [4, phase] + vector + [countdown, -6]
        c = PreparationProbe(image, base); c.mem = template.mem.copy()
        expected = execute(c, 0, None, None, before, 0, actor, player, None, 0, 2, owner_bounds)
        private.append(before + [0] + actor + player + [0, 2] + expected)
        counts['owner_stage'] += 1
    fixture = args.out / 'authored-preparation.tsv'
    fixture.write_text('packed code flags raw6 bounds6 heading trig2 lookupfactor before10 distance actor3 player3 profile3 global96 scratch75 after10 result3 segmentcalls+pointresultpairs\n' +
                       ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    owner_file = args.out / 'owner-stage-preparation.tsv'
    owner_file.write_text('before10 distance actor3 player3 global96 scratch75 after10 result3 segmentcalls+pointresultpairs\n' +
                          ''.join('\t'.join(map(str, row)) + '\n' for row in private))
    evidence = {'schema': 'actor-detection-preparation-source-proof/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'exe_sha256': SUPPORTED_EXE, 'overlay_sha256': OVERLAY_SHA, 'counts': counts,
        'instructions_executed': steps, 'restricted_gte_commands': commands,
        'materialized_idx_identities': len(materialized), 'authored_fixture_sha256': sha(fixture.read_bytes()),
        'owner_fixture_sha256': sha(owner_file.read_bytes()), 'no_original_payload_exported': True, 'retail_parity': False,
        'return_only_hooks': ['8001A008 sound request intent excluded; no normalization/interpolation/contact hooks'],
        'limits': ['Authored fixture uses synthetic profile/trig/lookup/geometry scalars.',
                   'Owner-stage fixture/reference inputs remain owner-local and are not source release payloads.',
                   'Native unknown contact families/arithmetic trap domains hold rather than claim a matching result.',
                   'Sound/notification publication and full actor callback/retail replay remain external.']}
    (args.out / 'source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))
if __name__ == '__main__': main()

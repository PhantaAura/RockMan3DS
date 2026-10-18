#!/usr/bin/env python3
"""Read-only source detector differential; authored fields only in fixtures.

Original CPU rotation and source state machine execute. The restricted GTE
adapter implements only the original pure heading MVMVA command with explicit
signed matrix/vector MAC results; general GTE flags/projection are excluded.
Phase1 helper outputs are labelled authored premises, never retail parity.
"""
import argparse
import hashlib
import inspect
import itertools
import json
import struct
import sys
import textwrap
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE
sys.path.insert(0, str(SOURCE / 'docs/research_01417'))
from class20_03_callback_source_probe import authored_store_hook
WORK, DETECTOR = 0x1000, 0x134c


def s16(x):
    x &= 65535
    return x - 65536 if x & 32768 else x


class DetectionProbe(OverlayProbe):
    def __init__(self, image, base):
        super().__init__(image, base, b'')
        self.data2, self.control2 = [0] * 32, [0] * 32
        self.gte_commands = 0

    def cop2(self, word):
        rs, rt, rd = word >> 21 & 31, word >> 16 & 31, word >> 11 & 31
        if rs == 6:
            self.control2[rd] = self.r[rt]
        elif rs == 2:
            self.r[rt] = self.control2[rd]
        elif rs == 4:
            self.data2[rd] = self.r[rt]
        elif rs == 0:
            self.r[rt] = self.data2[rd]
        elif rs >= 16:
            assert word & 63 == 0x12, 'Restricted GTE supports MVMVA only'
            sf, mx, v, cv = word >> 19 & 1, word >> 17 & 3, word >> 15 & 3, word >> 13 & 3
            assert (sf, mx, v, cv) == (1, 0, 0, 0), (sf, mx, v, cv)
            assert self.control2[5:8] == [0, 0, 0], 'Source heading translation must be zero'
            matrix = []
            for packed in self.control2[:5]:
                matrix.extend((s16(packed), s16(packed >> 16)))
            vector = [s16(self.data2[0]), s16(self.data2[0] >> 16), s16(self.data2[1])]
            for i in range(3):
                raw = sum(matrix[i * 3 + j] * vector[j] for j in range(3))
                assert -(1 << 43) <= raw < (1 << 43)
                mac = raw >> 12
                self.data2[25 + i] = mac & 0xffffffff
                self.data2[9 + i] = max(-32768, min(32767, mac)) & 0xffffffff
            # FLAG is not read for detection; it is stored in unused scratch.
            self.control2[31] = 0
            self.gte_commands += 1
        else:
            raise ValueError('Restricted unsupported GTE register transfer')


# Extend the established authored arithmetic interpreter only at its opcode
# dispatch. This source text is our own instrumentation, never owner code.
run_source = textwrap.dedent(inspect.getsource(OverlayProbe.run))
needle = 'elif op in (34, 38, 42, 46):'
replacement = '''elif op == 18:
    self.cop2(w)
elif op == 50:
    self.data2[rt] = self.get((a + si) & 0xffffffff, 4)
elif op == 58:
    self.put((a + si) & 0xffffffff, 4, self.data2[rt])
''' + needle
run_source = run_source.replace(needle, textwrap.indent(replacement, '        ').lstrip(), 1)
namespace = {}
exec(run_source, OverlayProbe.run.__globals__, namespace)
DetectionProbe.run = namespace['run']


def detector_fields(cpu):
    return [cpu.get(DETECTOR + offset, width, signed) for offset, width, signed in
            [(0, 2, True), (2, 2, True), (4, 2, True), (6, 1, False), (7, 1, False),
             (8, 2, True), (10, 2, True), (12, 2, True), (14, 1, False), (15, 1, True)]]


def put_detector(cpu, fields):
    for value, offset, width in zip(fields, (0, 2, 4, 6, 7, 8, 10, 12, 14, 15),
                                    (2, 2, 2, 1, 1, 2, 2, 2, 1, 1)):
        cpu.put(DETECTOR + offset, width, value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    image = args.exe.read_bytes()
    assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    if args.out.exists():
        raise SystemExit('Refusing to overwrite source evidence directory')
    args.out.mkdir(parents=True)
    base = struct.unpack_from('<I', image, 24)[0]
    rows, steps, commands = [], 0, 0
    counts = {'phase0': 0, 'phase1_authored_helpers': 0, 'phase2': 0, 'range_rejection': 0}

    def case(heading=0, trig=(0, -4096), phase=0, player=(11, -60, -100),
             actor=(0, 0, 0), origin=(0, -96, 0), vector=(0, 0, -4096),
             countdown=2, offset=-6, distance=0, profile=(10, 20, 15), global_value=0,
             mode=0, interpolated=(0, 0, -4096), wall0=0, wall1=0):
        nonlocal steps, commands
        cpu = DetectionProbe(image, base)
        cpu.r[4], cpu.r[5], cpu.r[29] = DETECTOR, WORK, 0x700000
        # Original profile lookup reads this authored record, not original data.
        for i, value in enumerate(profile):
            cpu.put(0x800899ac + 4 * 4 + i, 1, value)
        angle = 2048 - s16(heading)
        index = abs(angle) & 4095
        cpu.put(0x80092ec4 + index * 4, 4, (trig[0] & 65535) | ((trig[1] & 65535) << 16))
        cpu.put(WORK + 128, 4, distance)
        cpu.put(WORK + 86, 2, heading)
        for i, value in enumerate(actor): cpu.put(WORK + 20 + i * 2, 2, value)
        for i, value in enumerate(player): cpu.put(0x800b51c4 + i * 2, 2, value)
        cpu.put(0x800bbd96, 2, global_value)
        cpu.put(0x800bbd84, 1, 0)
        cpu.put(0x1f800075, 1, mode)
        before = list(origin) + [4, phase] + list(vector) + [countdown, offset]
        put_detector(cpu, before)
        normalized = (0, 0, -4096)
        authored_store_hook(cpu, 0x8007a694, [(5, i * 2, 2, v) for i, v in enumerate(normalized)])
        authored_store_hook(cpu, 0x80033f04, [(4, i * 2, 2, v) for i, v in enumerate(interpolated)])
        # Segment hook returns independent first/second authored results.
        # Two call sites can have separate output with an authored scratch counter.
        words = [(15 << 26) | (8 << 16) | 0x0006,
                 (35 << 26) | (8 << 21) | (9 << 16), 0,
                 (4 << 26) | (9 << 21) | 4, 0,
                 (15 << 26) | (2 << 16) | ((wall1 >> 16) & 65535),
                 (13 << 26) | (2 << 21) | (2 << 16) | (wall1 & 65535),
                 (31 << 21) | 8, 0,
                 (9 << 26) | (10 << 16) | 1,
                 (43 << 26) | (8 << 21) | (10 << 16),
                 (15 << 26) | (2 << 16) | ((wall0 >> 16) & 65535),
                 (13 << 26) | (2 << 21) | (2 << 16) | (wall0 & 65535),
                 (31 << 21) | 8, 0]
        # Fix branch target to authored first-return block (+9 words).
        words[3] = (4 << 26) | (9 << 21) | 5
        for i, word in enumerate(words): cpu.put(0x80049c54 + i * 4, 4, word)
        # Audio body excluded, intent recorded through original call trace.
        cpu.hook(0x8001a008)
        cpu.run(0x8003648c)
        after = detector_fields(cpu)
        detected = cpu.r[2]
        notified = cpu.get(0x800bbd84, 1)
        sounded = int(any(call['target'] == '0x8001a008' for call in cpu.calls))
        rows.append([heading] + list(trig) + before + [distance] + list(actor) + list(player) +
                    list(profile) + list(normalized) + list(interpolated) +
                    [wall0, wall1, global_value, mode] + after + [detected, notified, sounded])
        steps += cpu.steps; commands += cpu.gte_commands
        counts['range_rejection' if distance >= profile[2] << 20 else
               'phase0' if phase not in (1, 2) else 'phase1_authored_helpers' if phase == 1 else 'phase2'] += 1

    for phase, heading, trig, player in itertools.product(
            (0, 3, 255), (0, 1, 1024, 2048, 3072, 4095, 32767, 32768, 65535),
            ((0, -4096), (4096, 0), (-4096, 0), (1, 4095), (-1, -4095)),
            ((0, 0, 0), (100, 0, -100), (-100, 0, 100), (32767, -32768, -32768))):
        case(heading=heading, trig=trig, phase=phase, player=player, actor=(-32768, 32767, 32767))
    for phase, distance, range_byte in itertools.product((0, 1, 2, 255),
            (0, 1, 0x00efffff, 0x00f00000, 0xffffffff), (0, 1, 15, 255)):
        case(phase=phase, distance=distance, profile=(0, 255, range_byte))
    for heading, trig, countdown, global_value, player in itertools.product(
            (0, 2048, 32768), ((0, -4096), (1, 4095)),
            (0, 1, 2, 255), (-32768, -1, 0, 1, 32767), ((0, 0, -100), (0, 0, 100))):
        case(heading=heading, trig=trig, phase=2, player=player,
             countdown=countdown, global_value=global_value)
    for delta, interpolated, walls, countdown, mode, global_value in itertools.product(
            ((0, 0, 0), (-100, 99, 0), (99, 99, 99), (100, 0, 0),
             (-26750, 0, 0), (26750, 0, 0), (-26751, 0, 0), (26751, 0, 0)),
            ((0, 0, 0), (0, 0, -4096), (32767, -32768, -1)),
            ((0, 0), (1, 0), (0, 1), (-1, 0)), (0, 1, 2), (0, 2), (-1, 0, 1)):
        case(phase=1, origin=(0, 0, 0), player=delta, interpolated=interpolated,
             wall0=walls[0], wall1=walls[1], countdown=countdown, mode=mode, global_value=global_value)
    fixture = args.out / 'authored-detection.tsv'
    fixture.write_text(''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    globals_cases = []
    for value94, value96 in itertools.product((-32768, -1, 0, 1, 2, 32767), repeat=2):
        cpu = DetectionProbe(image, base)
        cpu.r[29] = 0x700000
        cpu.put(0x800bbd94, 2, value94); cpu.put(0x800bbd96, 2, value96)
        cpu.hook(0x8005c868)
        cpu.run(0x8005c2b8, 0x8005c318)
        globals_cases.append([value94, value96, cpu.get(0x800bbd94, 2, True), cpu.get(0x800bbd96, 2, True)])
        steps += cpu.steps
    cpu = DetectionProbe(image, base); cpu.r[29] = 0x700000
    for address in range(0x800bbd78, 0x800bbe50): cpu.put(address, 1, 255)
    cpu.run(0x8005c258)
    assert cpu.get(0x800bbd94, 2) == cpu.get(0x800bbd96, 2) == cpu.get(0x800bbd84, 1) == 0
    steps += cpu.steps
    (args.out / 'authored-globals.tsv').write_text(''.join('\t'.join(map(str, row)) + '\n' for row in globals_cases))
    metadata = {'schema': 'actor-detection-source-proof/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'exe_sha256': SUPPORTED_EXE, 'cases': len(rows), 'counts': counts,
        'globals_countdown_cases': len(globals_cases), 'original_initializer_clear_verified': True,
        'instructions_executed': steps, 'restricted_mvmva_commands': commands,
        'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
        'retail_parity': False, 'no_original_payload_exported': True,
        'source': {'detector': '0x800364B0', 'rotated_mask': '0x80030628/mask0x10',
                   'profile_table': '0x800899AC+profile*4', 'global_initialize': '0x8005C258',
                   'global_tick': '0x8005C2B8', 'global_setter': '0x8005EBB4'},
        'limits': ['Authored scalar work/profile/trig premises; fixture does not contain original tables.',
                   'Phase1 normalized/interpolated vectors and wall segment returns are authored helper outputs.',
                   'Restricted GTE implements pure heading matrix MAC used by source; general flags/projection excluded.',
                   'Sound156 is an intent; original sound owner is not exercised.',
                   'Subsystem initialization is proved; complete New Game/frame ordering remains integration evidence.']}
    (args.out / 'source-evidence.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps(metadata, indent=2))

if __name__ == '__main__': main()

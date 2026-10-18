#!/usr/bin/env python3
"""Original class20 scheduling and class03 prelude on authored work, read only.

The source dispatcher, full callback control flow and resident random trampoline
execute original owner instructions. A0 returns authored ordinal scalars; this
does NOT simulate the BIOS algorithm, prove seed/clock ownership or live parity.
Animation consumers are explicitly excluded. Output contains scalars and hashes,
never retail code, tables, instruction listings, assets or saves.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

POOL = 0x8009F108
STRIDE = 0x3C0
COUNT = 16
ORDINAL = 0x9100
WORK_LOG = 0x9200
RETURN_LOG = 0x9300


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def authored_gateway(cpu):
    """A0 instrumentation records original caller/work; supplies ordinal scalar.

    These encodings are authored instrumentation, not copied retail code. The
    gateway is deliberately the only random-service replacement. It never reads
    or writes a BIOS seed; a scalar starts at1234 plus the one-based call count.
    """
    words = [
        # Signed16 displacements9100/9200/9300 select these low addresses
        # from base10000; they do not mean positive unsigned additions.
        (15 << 26) | (8 << 16) | 1,
        (35 << 26) | (8 << 21) | (9 << 16) | 0x9100,     # count
        0,
        (9 << 16) | (10 << 11) | (2 << 6),              # t2=count*4
        (10 << 21) | (8 << 16) | (10 << 11) | 33,       # +scratch
        (43 << 26) | (10 << 21) | (16 << 16) | 0x9200,  # log s0/work
        (43 << 26) | (10 << 21) | (31 << 16) | 0x9300,  # log source RA
        (9 << 26) | (9 << 21) | (9 << 16) | 1,
        (43 << 26) | (8 << 21) | (9 << 16) | 0x9100,
        (9 << 26) | (9 << 21) | (2 << 16) | 0x1234,
        (31 << 21) | 8,
        0,
    ]
    for i, word in enumerate(words):
        cpu.put(0xA0 + i * 4, 4, word)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite chronology proof')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise SystemExit('Unsupported owner executable profile')
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(row['payload'] for row in iter_resources(args.dat / 'ST00.BIN')
                   if row['load'] == 0x80100000 and 'PROGBIN' in row['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise SystemExit('Unsupported owner tutorial overlay profile')
    flags = (0, 1, 2, 3, 128, 255)
    contexts = [([flags[(seed + slot) % 6] for slot in range(COUNT)],
                 [flags[(seed // 6 + slot * 3) % 6] for slot in range(COUNT)])
                for seed in range(36)]
    contexts += [([int(slot == selected) for slot in range(COUNT)], [1] * COUNT)
                 for selected in range(COUNT)]
    contexts += [([1] * COUNT, [int(slot != selected) for slot in range(COUNT)])
                 for selected in range(COUNT)]
    contexts += [([0] * COUNT, [1] * COUNT), ([1] * COUNT, [0] * COUNT),
                 ([1] * COUNT, [1] * COUNT), ([255] * COUNT, [255] * COUNT)]
    cases, total_steps = [], 0
    for flag0, flag6 in contexts:
        cpu = OverlayProbe(image, base, overlay)
        cpu.r[29] = 0x8000
        # Authored subtype/dispatch binding points to the real owner callback.
        cpu.put(0x1F800058, 4, 0x4000)
        cpu.put(0x400C, 4, 0x4100)
        cpu.put(0x4100, 4, 0x8010226C)
        authored_gateway(cpu)
        # The callback body does not need detection for parameter20; fixed
        # action0/phase1/timer300 stays before selection. Pose consumption is
        # excluded, rather than manufacturing a renderer/GTE implementation.
        for address in (0x800322A8, 0x80032488, 0x8002C8CC, 0x800341BC):
            cpu.hook(address)
        for slot in range(COUNT):
            work = POOL + slot * STRIDE
            for offset, width, value in ((0, 1, flag0[slot]), (1, 1, 0x20),
                    (2, 1, 3), (3, 1, 0x20), (6, 1, flag6[slot]), (8, 1, 1),
                    (9, 1, 0), (10, 1, 1), (0xAC, 1, 0), (0x32C, 2, 300)):
                cpu.put(work + offset, width, value)
            for lane in range(4):
                cpu.put(work + 0x14 + lane * 2, 2, slot * 10 + lane)
                cpu.put(work + 0x1C + lane * 4, 4, (slot * 10 + lane) * 65536)
                cpu.put(work + 0x54 + lane * 2, 2, slot * 256 + lane)
            cpu.put(work + 0x32A, 2, 0xAAAA)
        cpu.run(0x80016FD4)
        expected = [slot for slot in range(COUNT) if flag0[slot] & flag6[slot] & 1]
        count = cpu.get(ORDINAL, 4)
        measured_work = [cpu.get(WORK_LOG + i * 4, 4) for i in range(count)]
        measured_ra = [cpu.get(RETURN_LOG + i * 4, 4) for i in range(count)]
        if count != len(expected):
            raise ValueError('Authored chronology mismatch: ' + json.dumps({
                'flag0': flag0, 'flag6': flag6, 'count': count, 'expected': expected,
                'indirect': cpu.indirect, 'direct_calls': cpu.calls, 'steps': cpu.steps}))
        if measured_work != [POOL + slot * STRIDE for slot in expected]:
            raise ValueError('Original callback work order: ' + json.dumps({
                'actual_work': [hex(work) for work in measured_work], 'expected_slots': expected,
                'return_addresses': [hex(ra) for ra in measured_ra]}))
        # The JAL at22F0 returns to22F8. The sample store is at22FC;
        # conflating them causes genuine reference samples to be rejected.
        assert measured_ra == [0x801022F8] * len(expected)
        for slot in range(COUNT):
            sample = 0x1235 + expected.index(slot) if slot in expected else 0xAAAA
            assert cpu.get(POOL + slot * STRIDE + 0x32A, 2) == sample
            assert cpu.get(POOL + slot * STRIDE + 0x32C, 2) == (299 if slot in expected else 300)
        cases.append({'authored_flag0': flag0, 'authored_flag6': flag6,
                      'actual_callback_slots_in_order': expected,
                      'actual_original_random_trampoline_calls': count,
                      'actual_caller_return_addresses': [hex(ra) for ra in measured_ra],
                      'original_instructions_plus_instrumentation': cpu.steps})
        total_steps += cpu.steps
    result = {
        'schema': 'mml-class03-rng-callback-chronology-source/1',
        'status': 'SOURCE_BACKED_PARTIAL', 'exe_sha256': sha(image),
        'overlay_sha256': sha(overlay), 'authored_cases': len(cases),
        'original_and_instrumentation_instructions_executed': total_steps,
        'dispatcher': '0x80016FD4', 'pool_base': hex(POOL), 'pool_slots': COUNT,
        'pool_stride': STRIDE, 'admission': '(work0 & work6 & 1) != 0',
        'callback': '0x8010226C', 'resident_rand_trampoline': '0x8007FE50',
        'class03_caller_return_address': '0x801022F8',
        'class03_sample_store': '0x801022FC',
        'observed_order_on_authored_contexts': 'ascending admitted original pool slots',
        'random_call_ownership': 'one unconditional original trampoline call per admitted class03 callback',
        'cases': cases,
        'game_seed_proved': False, 'full_game_consumer_chronology_proved': False,
        'BIOS_algorithm_exercised': False, 'live_reference_replay': False, 'retail_parity': False,
        'hooks': ['A0 authored ordinal-return logger; no BIOS RNG implementation or seed writes',
                  '800322A8/80032488 pose consumers excluded',
                  '8002C8CC/800341BC effect/proxy consumers excluded',
                  'Authored dynamic-dispatch table binding to original callback'],
        'limits': ['The original source dispatcher/callback/trampoline execute; inputs are authored synthetic work.',
                   'Other actor pools, subtypes, event/cutscene consumers and initialization remain unrepresented.',
                   'Original frame/update cadence, pauses and input chronology are not established.',
                   'A private PRNG per imported actor would violate demonstrated shared call order.',
                   'Consuming a scalar only when low bits are used is not original full callback scheduling.',
                   'Excluded helpers may have additional scheduling/side effects; no complete consumer claim.',
                   'No owner instruction bytes, original tables, assets or save data exported.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ('status', 'authored_cases',
        'original_and_instrumentation_instructions_executed', 'retail_parity')}))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Read-only BIOS random-service ownership evidence; no retail payload output.

The gateway returns explicitly authored scalars. It does not implement BIOS
randomness, supply a game seed, or claim native/retail replay parity.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from mml_exe_research import Image
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

SCRATCH = 0x9100
WORK = 0x1000
GATEWAY = 0xA0
RAND = 0x8007FE50
CALLBACK = 0x8010226C
AFTER_SAMPLE_STORE = 0x80102300


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def direct_candidates(blob, base, target):
    """Aligned direct jump/call candidates, without claiming every word is code."""
    result = []
    for offset in range(0, len(blob) - 3, 4):
        word = struct.unpack_from('<I', blob, offset)[0]
        pc = base + offset
        if word >> 26 in (2, 3):
            destination = ((pc + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
            if destination == target:
                result.append(hex(pc))
    return result


def function_id_candidates(blob, base, identifier):
    result = []
    for offset in range(0, len(blob) - 3, 4):
        word = struct.unpack_from('<I', blob, offset)[0]
        if word >> 26 in (9, 13) and (word >> 21 & 31) == 0 and (word >> 16 & 31) == 9:
            if word & 0xFFFF == identifier:
                result.append(hex(base + offset))
    return result


def srand_trampoline_candidates(blob, base):
    """Match the observed three-instruction A0 trampoline shape, ID30 only.

    Negative evidence is limited to this shape and supplied image; indirect
    calls, runtime patches, other overlays and BIOS boot remain uninspected.
    """
    result = []
    for offset in range(0, len(blob) - 11, 4):
        first, jump, delay = struct.unpack_from('<3I', blob, offset)
        if (first >> 26 in (9, 13) and (first >> 21 & 31) == 0 and
                (first >> 16 & 31) == 10 and first & 0xFFFF == 0xA0 and
                jump >> 26 == 0 and (jump >> 21 & 31) == 10 and jump & 63 == 8 and
                delay >> 26 in (9, 13) and (delay >> 21 & 31) == 0 and
                (delay >> 16 & 31) == 9 and delay & 0xFFFF == 0x30):
            result.append(hex(base + offset))
    return result


def authored_gateway(cpu):
    """Record ID/call count and return a supplied scalar from scratch.

    All encodings below are authored instrumentation. The original EXE rand
    trampoline and overlay callback prelude remain unmodified.
    """
    words = [
        (15 << 26) | (8 << 16) | 1,
        (43 << 26) | (8 << 21) | (9 << 16) | 0x9100,
        (35 << 26) | (8 << 21) | (11 << 16) | 0x9104,
        0,
        (9 << 26) | (11 << 21) | (11 << 16) | 1,
        (43 << 26) | (8 << 21) | (11 << 16) | 0x9104,
        (35 << 26) | (8 << 21) | (2 << 16) | 0x9108,
        0,
        (31 << 21) | 8,
        0,
    ]
    for index, word in enumerate(words):
        cpu.put(GATEWAY + index * 4, 4, word)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite evidence')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise SystemExit('Unsupported owner executable profile')
    exe = Image(image)
    overlay = next(row['payload'] for row in iter_resources(args.dat / 'ST00.BIN')
                   if row['load'] == 0x80100000 and 'PROGBIN' in row['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise SystemExit('Unsupported owner tutorial overlay profile')
    total_steps = 0
    trampoline_rows = []
    for argument in (0, 1, 0x13579BDF, 0xFFFFFFFF):
        cpu = OverlayProbe(image, exe.base, overlay)
        cpu.r[4], cpu.r[31] = argument, 0x7770
        cpu.run(RAND, GATEWAY)
        assert cpu.r[9] == 0x2F and cpu.r[10] == GATEWAY
        assert cpu.r[4] == argument and cpu.r[31] == 0x7770 and cpu.steps == 3
        trampoline_rows.append({'a0_argument_before': argument,
                                'a0_argument_after': cpu.r[4], 'function_id_t1': cpu.r[9],
                                'gateway_t2': cpu.r[10], 'caller_ra_after': cpu.r[31],
                                'original_instructions_executed': cpu.steps})
        total_steps += cpu.steps
    prelude_rows = []
    for state, sample in itertools.product((0, 1, 2, 255), (0, 1, 0x7FFF, 0x8000, 0xFFFFFFFF)):
        cpu = OverlayProbe(image, exe.base, overlay)
        authored_gateway(cpu)
        cpu.r[4], cpu.r[29], cpu.r[31] = WORK, 0x8000, 0
        cpu.put(WORK + 8, 1, state)
        cpu.put(WORK + 172, 1, 7)
        cpu.put(SCRATCH + 8, 4, sample)
        cpu.run(CALLBACK, AFTER_SAMPLE_STORE)
        assert cpu.get(SCRATCH, 4) == 0x2F and cpu.get(SCRATCH + 4, 4) == 1
        assert cpu.get(WORK + 810, 2) == sample & 0xFFFF
        assert cpu.get(WORK + 173, 1) == 7
        assert cpu.get(WORK + 350, 2) == 16912
        assert cpu.get(WORK + 8, 1) == state
        prelude_rows.append({'state08': state, 'authored_gateway_return': sample,
                             'sample32A_after': cpu.get(WORK + 810, 2),
                             'gateway_calls': cpu.get(SCRATCH + 4, 4),
                             'function_id_t1': cpu.get(SCRATCH, 4),
                             'request_snapshot_AD': cpu.get(WORK + 173, 1),
                             'work15E_after': cpu.get(WORK + 350, 2),
                             'probe_instructions_executed': cpu.steps})
        total_steps += cpu.steps
    # One shared gateway, two authored work blocks, explicitly supplied samples.
    # This proves prelude call order and truncation, not sample generation.
    cpu = OverlayProbe(image, exe.base, overlay)
    authored_gateway(cpu)
    ordered_rows = []
    for ordinal, (work, state, sample) in enumerate(((WORK, 0, 0xFFFF1234),
                                                   (WORK + 0x1000, 1, 0x89AB),
                                                   (WORK, 2, 0x5678)), 1):
        cpu.r[4], cpu.r[29], cpu.r[31] = work, 0x8000, 0
        cpu.put(work + 8, 1, state)
        cpu.put(SCRATCH + 8, 4, sample)
        cpu.run(CALLBACK, AFTER_SAMPLE_STORE)
        assert cpu.get(SCRATCH + 4, 4) == ordinal
        assert cpu.get(work + 810, 2) == sample & 0xFFFF
        ordered_rows.append({'authored_callback_ordinal': ordinal, 'work': hex(work),
                             'state08': state, 'authored_gateway_return': sample,
                             'sample32A_after': cpu.get(work + 810, 2),
                             'shared_gateway_count': cpu.get(SCRATCH + 4, 4)})
    total_steps += cpu.steps
    ranges = [('exe', exe.base, image[2048:2048 + exe.size]),
              ('tutorial_overlay', 0x80100000, overlay)]
    scans = []
    for label, base, blob in ranges:
        scans.append({'image': label, 'bytes': len(blob),
                      'rand_direct_call_candidates': direct_candidates(blob, base, RAND),
                      'direct_A0_candidates': direct_candidates(blob, base, 0xA0) +
                                              direct_candidates(blob, base, 0x800000A0),
                      'function30_load_candidates': function_id_candidates(blob, base, 0x30),
                      'srand_trampoline_shape_candidates': srand_trampoline_candidates(blob, base)})
    result = {'schema': 'mml-bios-rng-ownership/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
              'trampoline_region_sha256': sha(image[exe.offset(RAND):exe.offset(RAND) + 12]),
              'callback_prelude_region_sha256': sha(overlay[CALLBACK - 0x80100000:
                                                           AFTER_SAMPLE_STORE - 0x80100000]),
              'rand_trampoline': trampoline_rows, 'callback_preludes': prelude_rows,
              'ordered_gateway_authored_cases': ordered_rows,
              'static_scans': scans, 'probe_instructions_executed': total_steps,
              'BIOS_algorithm_exercised': False, 'game_seed_proved': False,
              'shared_consumer_lifecycle_proved': False, 'retail_parity': False,
              'hooks': ['Only A0 BIOS gateway replaced with authored scalar-return instrumentation.',
                        'Original callback stops immediately after sample32A store; state dispatch excluded.'],
              'limits': ['Static aligned scan may include data or miss indirect calls/runtime patches.',
                         'Other overlays, BIOS boot, warm restart and save restoration are not covered.',
                         'No seed, original BIOS state, retail consumer chronology or pause cadence is supplied.',
                         'No original code, tables, instruction listings, BIOS payload or assets are serialized.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'status': result['status'], 'trampoline_cases': len(trampoline_rows),
                      'prelude_cases': len(prelude_rows), 'ordered_cases': len(ordered_rows),
                      'instructions': total_steps, 'game_seed_proved': False,
                      'rand_call_candidates': {row['image']: len(row['rand_direct_call_candidates'])
                                               for row in scans}}))


if __name__ == '__main__':
    main()

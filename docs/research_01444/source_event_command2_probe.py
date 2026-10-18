#!/usr/bin/env python3
"""Bounded original type2 owner oracle; authored states, no helper hooks.

Writes scalar/count/hash evidence only. The immutable owner is read locally.
Complete cases execute all reached helpers. Phase0 pause prefixes stop before
the existing transport owner and are explicitly incomplete.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'docs/research_01424')]
from source_event_command_probe import Probe, SUPPORTED_EXE, put, get, pattern

QUEUE = 0x800a3a40
FIELDS = ((0x80098a84, 4), (0x800989a4, 4), (0x800989c4, 4),
          (0x800989c8, 4), (0x80098964, 1), (0x80098b42, 1),
          (0x800988d0, 1), (0x800ad142, 2), (0x800988dc, 1),
          (0x80098994, 2), (0x800ad146, 2))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def make(image, base, seed, subtype, phase, ready, count, current,
         flags=0x1234, toggle=0, saved=0x9235, volume=0x4567):
    cpu = Probe(image, base, b'')
    cpu.r[29] = 0x700000
    raw = bytearray(pattern(256, seed))
    for index in range(16):
        struct.pack_into('<I', raw, index * 16, 2 if index < count else 0)
    struct.pack_into('<I', raw, current * 16 + 8, subtype)
    put(cpu, QUEUE, raw)
    values = (QUEUE + current * 16, QUEUE + count * 16, phase,
              0x31415926, ready, 73, 19, flags, toggle, saved, volume)
    for (address, width), value in zip(FIELDS, values):
        cpu.put(address, width, value)
    return cpu, raw, list(values)


def snapshot(cpu):
    return [cpu.get(address, width) for address, width in FIELDS]


def popped(raw, count):
    expected = bytearray(raw)
    expected[:count * 16] = raw[16:(count + 1) * 16]
    expected[count * 16:count * 16 + 4] = b'\0' * 4
    return expected


def check(cpu, raw, scalars):
    assert snapshot(cpu) == scalars, (snapshot(cpu), scalars)
    assert get(cpu, QUEUE, 256) == raw
    assert not cpu.return_only_hooks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--proof', required=True, type=Path)
    args = parser.parse_args()
    if args.proof.exists():
        raise ValueError('Refusing to overwrite retained evidence')
    image = args.exe.read_bytes()
    assert digest(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    readword = lambda address: struct.unpack_from('<I', image, 2048 + address - base)[0]
    assert readword(0x80087688) == 0x8001b858
    expected_targets = (0x8001b63c, 0x8001b644, 0x8001b6fc, 0x8001b7b4)
    assert tuple(readword(0x80087670 + index * 4) for index in range(4)) == expected_targets
    counts = {'subtype0_noop': 0, 'subtype12_complete': 0,
              'subtype3_complete': 0, 'phase0_incomplete_prefix': 0}
    total_steps = total_mixes = queue_pops = 0
    max_steps = 0
    outcomes = hashlib.sha256()
    # Only subtype0 is a true no-op. It must not require phase/readiness fields.
    for phase, ready, count in itertools.product((0, 1, 2, 0xffffffff), (0, 1, 255), (1, 7, 15)):
        cpu, raw, scalars = make(image, base, counts['subtype0_noop'], 0,
                                 phase, ready, count, 0)
        cpu.run(0x8001b858)
        check(cpu, raw, scalars)
        assert not cpu.mix
        counts['subtype0_noop'] += 1
        total_steps += cpu.steps
        max_steps = max(max_steps, cpu.steps)
    # Phase1 and unrecognized phases never enter the unmodeled CD driver.
    for subtype, phase, ready, count, current in itertools.product(
            (1, 2), (1, 2, 3, 0xffffffff), (0, 1, 255), (1, 7, 15), (0, 2)):
        if current >= count:
            continue
        cpu, raw, scalars = make(image, base, counts['subtype12_complete'], subtype,
                                 phase, ready, count, current)
        if phase == 1 and ready:
            scalars[1] -= 16
            scalars[2] = scalars[3] = 0
            scalars[5] = subtype
            raw = popped(raw, count)
            queue_pops += 1
        cpu.run(0x8001b858)
        check(cpu, raw, scalars)
        assert not cpu.mix
        counts['subtype12_complete'] += 1
        total_steps += cpu.steps
        max_steps = max(max_steps, cpu.steps)
        outcomes.update(struct.pack('<' + 'I' * len(scalars), *scalars) + raw)
    # Subtype3 includes every real mix helper and queue compaction.
    for flags, toggle, saved, volume, count, current in itertools.product(
            (0, 1, 0x7ffe, 0xffff), (0, 1, 128, 255),
            (0, 0x9235, 0xffff), (0, 0x4567, 0xffff), (1, 7, 15), (0, 2)):
        if current >= count:
            continue
        cpu, raw, scalars = make(image, base, counts['subtype3_complete'], 3,
                                 0x12345678, 0, count, current,
                                 flags, toggle, saved, volume)
        scalars[1] -= 16
        scalars[2] = scalars[3] = 0
        scalars[7] |= 0x8000
        if toggle:
            scalars[8] = 0
            scalars[10] = saved
        else:
            scalars[8] = 1
            scalars[9] = volume
            scalars[10] = 0
        raw = popped(raw, count)
        cpu.run(0x8001b858)
        check(cpu, raw, scalars)
        assert len(cpu.mix) == 127
        assert cpu.mix == [bytes((v, v if flags & 1 else 0, v, v if flags & 1 else 0)).hex()
                           for v in range(127, 0, -1)]
        counts['subtype3_complete'] += 1
        total_steps += cpu.steps
        total_mixes += len(cpu.mix)
        queue_pops += 1
        max_steps = max(max_steps, cpu.steps)
        outcomes.update(struct.pack('<' + 'I' * len(scalars), *scalars) + raw)
    boundaries = []
    for subtype, flags in itertools.product((1, 2), (0, 1, 0xffff)):
        cpu, raw, scalars = make(image, base, subtype, subtype, 0, 0, 1, 0, flags)
        cpu.run(0x8001b858, 0x8001cb7c)
        scalars[7] |= 0x8000
        check(cpu, raw, scalars)
        assert len(cpu.mix) == 127
        counts['phase0_incomplete_prefix'] += 1
        total_steps += cpu.steps
        total_mixes += len(cpu.mix)
        max_steps = max(max_steps, cpu.steps)
        boundaries.append({'subtype': subtype, 'flags': flags, 'stop_before': '8001CB7C',
                           'mix_calls': len(cpu.mix), 'complete': False})
    regions = [(0x8001b63c, 0x8001b89c), (0x8001caac, 0x8001cb30),
               (0x8001d394, 0x8001d414), (0x80078efc, 0x80078f84)]
    proof = {'schema': 'mml-source-event-command2/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'exe_sha256': digest(image), 'entry': '8001B858',
             'outer_type2_slot': '80087688', 'callback_table': '80087670',
             'bounded_callback_indices': [0, 1, 2, 3], 'cases': counts,
             'original_instructions_executed': total_steps,
             'original_cd_mix_calls': total_mixes, 'complete_case_queue_pops': queue_pops,
             'max_case_instructions': max_steps, 'helper_hooks': 0,
             'instruction_patches': 0, 'output_scalars_digest': outcomes.hexdigest(),
             'phase0_boundaries': boundaries,
             'regions': [{'start': hex(lo), 'end_exclusive': hex(hi),
                          'sha256': digest(image[2048 + lo - base:2048 + hi - base])}
                         for lo, hi in regions],
             'limits': ['Authored scalar/queue states, not recorded retail chronology.',
                        'Indices outside0..3 are not admitted; source owner has no bounds check.',
                        'Subtype1/2 phase0 stops before pause helper; no transport receipt or real media chronology is claimed.',
                        'MMIO mix stores are logical packets only, not audible or hardware verification.',
                        'No owner instructions, lookup tables, audio or assets are exported.'],
             'retail_parity': False, 'hardware_tested': False}
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps({key: proof[key] for key in ('cases', 'original_instructions_executed',
          'original_cd_mix_calls', 'complete_case_queue_pops', 'max_case_instructions')}))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Original index1 ordered double ramp and explicit authored pause boundary.

No transport body is executed or replaced. Original caller slices resume after
the declared transport boundary. Callback cases execute the actual callback.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
from source_event_command2_probe import make, check, popped, digest, get, put, QUEUE, SUPPORTED_EXE


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
    totals = {'cases': 0, 'original_instructions': 0, 'actual_callback_cases': 0,
              'original_mix_calls': 0, 'queue_pops': 0}
    all_calls = set()
    records = []
    for ready, callback, flags, (count, current) in itertools.product(
            (0, 1, 255), (None, 2, 3), (0, 1), ((1, 0), (7, 2))):
        cpu, raw, scalars = make(image, base, totals['cases'], 1, 0,
                                 ready, count, current, flags)
        cpu.put(0x8009896c, 1, 41)
        cpu.put(0x800988c0, 1, 17)
        cpu.put(0x80098ab8, 1, 97)
        cpu.put(0x80091ffc, 4, 0x81234567)
        cpu.put(0x80092000, 4, 0x82345678)
        # Preserve the inherited per-slice10,000-instruction bound.
        cpu.run(0x8001b858, 0x8001cb7c)
        first_steps = cpu.steps
        assert len(cpu.mix) == 127
        scalars[7] |= 0x8000
        check(cpu, raw, scalars)
        cpu.steps = 0
        cpu.run(0x8001cb7c, 0x8001d254)
        second_steps = cpu.steps
        assert len(cpu.mix) == 254
        assert cpu.r[4:7] == [9, 0, 0x80098a98]
        assert cpu.get(0x8009896c, 1) == 0
        assert cpu.get(0x80091ffc, 4) == 0x8001cc08
        assert cpu.get(0x80092000, 4) == 0
        check(cpu, raw, scalars)
        mix_sequence = [bytes((v, v if flags & 1 else 0,
                               v, v if flags & 1 else 0)).hex()
                        for v in range(127, 0, -1)]
        assert cpu.mix == mix_sequence + mix_sequence
        calls_before_transport = set((call['site'], call['target']) for call in cpu.calls)
        assert {target for _, target in calls_before_transport} <= {
            '0x8001d394', '0x80077de4', '0x80078efc', '0x8001cb7c',
            '0x80077a24', '0x80077a0c', '0x8001d254'}
        all_calls.update(calls_before_transport)
        # Explicit authored receipt boundary, not original transport execution.
        response = bytes((33, 7, 14, 21, 28, 35, 42, 49))
        put(cpu, 0x80098a98, response)
        callback_steps = 0
        if callback is not None:
            saved_registers = cpu.r[:]
            cpu.r[4], cpu.r[31], cpu.steps = callback, 0, 0
            cpu.run(0x8001cc08)
            callback_steps = cpu.steps
            cpu.r = saved_registers
            totals['actual_callback_cases'] += 1
        effective_ready = 1 if callback == 2 else ready
        expected_error = 8 if callback not in (None, 2) else 0
        expected_c0 = 17 if callback is None else (0 if callback == 2 else 128)
        expected_callback = 0x8001cc08 if callback is None else 0
        scalars[4] = effective_ready
        scalars[6] = 1
        scalars[2] = 1
        should_pop = bool(effective_ready)
        if should_pop:
            scalars[1] -= 16
            scalars[2] = scalars[3] = 0
            scalars[5] = 1
            raw = popped(raw, count)
            totals['queue_pops'] += 1
        # Resume immediately after the original pause-wrapper call.
        cpu.steps = 0
        cpu.run(0x8001cbe4)
        tail_steps = cpu.steps
        check(cpu, raw, scalars)
        assert get(cpu, 0x80098a98, 8) == response
        assert cpu.get(0x8009896c, 1) == expected_error
        assert cpu.get(0x800988c0, 1) == expected_c0
        assert cpu.get(0x80098ab8, 1) == 0
        assert cpu.get(0x80091ffc, 4) == expected_callback
        assert cpu.get(0x80092000, 4) == 0
        assert len(cpu.mix) == 254
        totals['cases'] += 1
        totals['original_instructions'] += first_steps + second_steps + callback_steps + tail_steps
        totals['original_mix_calls'] += len(cpu.mix)
        records.append({'ready_before': ready, 'authored_callback_code': callback,
                        'stereo': flags, 'queue_count_before': count,
                        'current_index_before': current, 'phase_after': scalars[2],
                        'ready_after': effective_ready, 'queue_pop': should_pop,
                        'special_after': scalars[5], 'mode_after': scalars[6],
                        'status_after': 0, 'error_after': expected_error,
                        'byte800988c0_after': expected_c0,
                        'callback_after': hex(expected_callback),
                        'slice_instruction_counts': [first_steps, second_steps,
                                                     callback_steps, tail_steps]})
    regions = [(0x8001b644, 0x8001b6fc), (0x8001b858, 0x8001b89c),
               (0x8001cb7c, 0x8001cc7c), (0x80077a0c, 0x80077a3c),
               (0x80077de4, 0x80077dfc)]
    proof = {'schema': 'mml-source-event-command2-pause-boundary/1',
             'status': 'SOURCE_BACKED_PARTIAL', 'exe_sha256': digest(image),
             'totals': totals, 'source_entry': '8001B858', 'callback_index': 1,
             'ordered_prefix': ['127mix127..1', 'half800AD142|8000',
                                '8001CB7C:127mix127..1', 'clear8009896C',
                                'half800AD142|8000', 'callback80092000=0',
                                'callback80091FFC=8001CC08',
                                '8001D254(command9,null,response80098A98)'],
             'prefix_direct_calls': [{'site': site, 'target': target}
                                     for site, target in sorted(all_calls)],
             'ramp_wait_or_vsync_calls': 0,
             'authored_boundary': {'stop_before': '8001D254', 'resume_at': '8001CBE4',
                                   'transport_body_executed': False,
                                   'response_bytes': 8,
                                   'callback_codes': [None, 2, 3]},
             'cases': records, 'helper_hooks': 0, 'instruction_patches': 0,
             'regions': [{'start': hex(lo), 'end_exclusive': hex(hi),
                          'sha256': digest(image[2048 + lo - base:2048 + hi - base])}
                         for lo, hi in regions],
             'limits': ['Author-selected response and callback chronology; no original CD driver execution.',
                        'Split source execution resets only the interpreter instruction counter between bounded slices.',
                        'Actual callback executes separately with parent registers restored at the explicit transport boundary.',
                        'No retail timeline, audible fade, frame duration or hardware parity is established.'],
             'retail_parity': False, 'hardware_tested': False}
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps(totals))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Original event poll/init/phase0 and pure countdown differential, read only.

Every work/flag/HP/busy input is authored synthetic data. The original dispatcher,
poll, flag reader, initializer and supported handler branches execute unchanged.
Only the source poll TABLE is replaced by explicitly authored three-entry data;
there are no helper hooks in accepted differential cases. Outputs contain only
authored work bytes, measured scalars, addresses and hashes, never owner payloads.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

WORK = 0x800C4C48
FLAGS = (23, 1540, 2047)


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--fixture', required=True, type=Path)
    parser.add_argument('--proof', required=True, type=Path)
    args = parser.parse_args()
    if args.fixture.exists() or args.proof.exists():
        raise SystemExit('Refusing to overwrite event differential evidence')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise SystemExit('Unsupported owner executable identity')
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(record['payload'] for record in iter_resources(args.dat / 'ST00.BIN')
                   if record['load'] == 0x80100000 and 'PROGBIN' in record['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise SystemExit('Unsupported owner tutorial overlay identity')
    contexts = [(0, 1, 79, 0x11223344, transition, hp, busy, mask)
                for transition, hp, busy, mask in itertools.product(
                    (0, 1, -1), (-32768, -1, 0, 1, 32767),
                    (-32768, -1, 0, 1, 32767), (0, 1, 3, 7))]
    contexts += [(active, 1, 0, 0x11223344, -1, -1, busy, 0)
                 for active, busy in itertools.product((1, 2, 255), (-32768, -1, 0, 1, 32767))]
    contexts += [(active, 1, 4, timer, -1, -1, -1, 0)
                 for active, timer in itertools.product((1, 2, 255),
                    (-2147483648, -2, -1, 0, 2, 32767, 2147483647))]
    rows, instructions = [], 0
    for index, (active, event, phase, timer, transition, hp, busy, mask) in enumerate(contexts):
        cpu = OverlayProbe(image, base, overlay)
        cpu.r[29] = 0x8000
        before = bytearray((offset * 37 + index * 11) & 255 for offset in range(0xA8))
        before[0], before[1], before[4] = active, event, phase
        struct.pack_into('<I', before, 0x0C, timer & 0xFFFFFFFF)
        for offset, value in enumerate(before):
            cpu.put(WORK + offset, 1, value)
        cpu.put(0x800C1B60, 1, 0)
        cpu.put(0x800C356C, 1, transition)
        cpu.put(0x800B521E, 2, hp)
        cpu.put(0x80098A72, 2, busy)
        for flag_index, flag in enumerate(FLAGS):
            address, bit = 0x800BE378 + (flag >> 3), 0x80 >> (flag & 7)
            cpu.put(address, 1, (cpu.get(address, 1) & ~bit) |
                    (bit if mask & (1 << flag_index) else 0))
            payload = struct.pack('<HBB', flag, 1, flag_index + 0xA0)
            for lane, value in enumerate(payload):
                cpu.put(0x8011ADF8 + flag_index * 4 + lane, 1, value)
        cpu.put(0x8011ADF8 + len(FLAGS) * 4, 2, 65535)
        flag_reads, global_halfword_reads = [], []
        original_get = cpu.get
        def get(address, width, signed=False):
            value = original_get(address, width, signed)
            if address == 0x8001DA58 and width == 4:
                flag_reads.append(cpu.r[4])
            if address >= 0x80000000 and width == 2:
                global_halfword_reads.append(address)
            return value
        cpu.get = get
        cpu.run(0x8001D990)
        assert not cpu.return_only_hooks
        after = bytes(cpu.get(WORK + offset, 1) for offset in range(0xA8))
        selected = flag_reads[-1] if any(target['target'] == '0x8001dde4' for target in cpu.calls) else -1
        if active == 0 and transition == 0 and mask and hp >= 0:
            assert 0x80098A72 in global_halfword_reads
            assert after[0] == 1 and after[1] == 1 and after[4] == (0 if busy else 1)
        if active != 0 and phase == 0:
            assert 0x80098A72 in global_halfword_reads
        assert 0x800A8A72 not in global_halfword_reads
        rows.append([transition, hp, busy, mask, selected, before.hex(), after.hex()])
        instructions += cpu.steps
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text('transition\thp\tbusy980\tflagmask\tselectedflag\tauthored_before\tactual_after\n' +
                           ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    regions = [(0x8001D990, 0x8001DA58), (0x8001DDE4, 0x8001DE78)]
    proof = {'schema': 'mml-source-event-pipeline-differential/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
             'original_instruction_cases': len(rows), 'original_instructions_executed': instructions,
             'fixture_sha256': sha(args.fixture.read_bytes()),
             'regions': [{'start': hex(lo), 'end_exclusive': hex(hi),
                          'sha256': sha(image[2048 + lo - base:2048 + hi - base])}
                         for lo, hi in regions],
             'corrected_original_busy_halfword': '0x80098A72',
             'incorrect_inherited_busy_halfword': '0x800A8A72',
             'initializer_hp_gate': 'signed player800B521E >=0; negative HP leaves event work unchanged',
             'poll_order': 'first enabled authored flag; no further poll after refused initializer',
             'same_tick_dispatch': True, 'accepted_cases_helper_hooks': [],
             'authored_tables': 'Three flag/event records replace source table data only; original code unchanged',
             'retail_parity': False, 'live_gameplay': False, 'hardware_tested': False,
             'limits': ['Source table/work/global premises are authored, not an original gameplay trace.',
                        'Event1phase1 requires real player consumer80042238; no readiness acknowledgement is invented.',
                        'Phase4expiry requires80017C30; only pure nonexpiry source branches are accepted here.',
                        'Actor command/message/audio/camera/transition consumers and normal tutorial completion are not established.',
                        'No original bytecode/table/code/assets/save data serialized.']}
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps({key: proof[key] for key in ('status', 'original_instruction_cases',
        'original_instructions_executed', 'corrected_original_busy_halfword', 'retail_parity')}))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Hook-free owner story predicate and parameter20 action0 caller proof.

Runs original instructions with explicitly authored work/global flag bytes.
Owner inputs are read-only; output contains scalars/hashes, never code/tables.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

WORK, BANK = 0x1000, 0x800be378


def sha(data):
    return hashlib.sha256(data).hexdigest()


class ReadProbe(OverlayProbe):
    def __init__(self, image, base, overlay):
        super().__init__(image, base, overlay)
        self.flag_reads = []
        self.tracking = False

    def get(self, address, width, signed=False):
        if self.tracking and BANK <= address < BANK + 8192:
            self.flag_reads.append((address, width))
        return super().get(address, width, signed)

    def reset(self):
        self.r = [0] * 32
        self.r[29] = 0x8000
        self.steps = 0
        self.calls = []
        self.indirect = []
        self.flag_reads = []
        self.tracking = False


def reverse_byte(value):
    return sum(((value >> i) & 1) << (7 - i) for i in range(8))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--dat', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    image = args.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    if args.out.exists():
        raise SystemExit('Refusing to overwrite predicate evidence')
    cpu = ReadProbe(image, base, overlay)
    instructions = helper_cases = caller_cases = 0
    examples = []
    selected_ids = {0, 1, 7, 8, 1547, 1599, 1600, 1601, 2047, 2048, 65535}
    for bit_id in list(range(2048)) + [2048, 65535]:
        address, mask = BANK + (bit_id >> 3), 0x80 >> (bit_id & 7)
        for pattern in (0, mask, mask ^ 255, 255):
            cpu.reset()
            cpu.r[4] = bit_id
            cpu.put(address, 1, pattern)
            cpu.tracking = True
            cpu.run(0x8001da58)
            cpu.tracking = False
            actual = cpu.r[2]
            assert actual == int(bool(pattern & mask))
            assert cpu.flag_reads == [(address, 1)]
            assert not cpu.calls and not cpu.indirect and not cpu.return_only_hooks
            assert cpu.r[29] == 0x8000
            # Neutral native save/session packing uses LSB-first physical bits
            # but retains exactly the same semantic IDs within0..2047.
            native = int(bool(reverse_byte(pattern) & (1 << (bit_id & 7))))
            assert native == actual
            helper_cases += 1
            instructions += cpu.steps
            if bit_id in selected_ids:
                examples.append({'id': bit_id, 'flag_byte_address': hex(address),
                                 'authored_flag_byte': pattern, 'source_mask': mask,
                                 'result': actual, 'native_domain': bit_id < 2048})

    for bit_id in range(2048):
        address, mask = BANK + (bit_id >> 3), 0x80 >> (bit_id & 7)
        for enabled in (False, True):
            cpu.reset()
            cpu.r[4] = WORK
            for offset, width, value in [(3, 1, 0x20), (9, 1, 0), (10, 1, 2),
                                         (11, 1, 93), (808, 2, 0x12), (812, 2, 321),
                                         (816, 2, 123), (866, 2, bit_id),
                                         (128, 4, 0xffffffff), (828, 4, 0xffffff)]:
                cpu.put(WORK + offset, width, value)
            cpu.put(address, 1, mask if enabled else mask ^ 255)
            cpu.tracking = True
            cpu.run(0x80102884)
            cpu.tracking = False
            assert cpu.flag_reads == [(address, 1)]
            assert [call['target'] for call in cpu.calls] == ['0x8001da58']
            assert not cpu.indirect and not cpu.return_only_hooks
            assert cpu.get(WORK + 9, 1) == (3 if enabled else 0)
            assert cpu.get(WORK + 10, 1) == (0 if enabled else 2)
            assert cpu.get(WORK + 11, 1) == (0 if enabled else 93)
            assert cpu.get(WORK + 808, 2) == 0x32
            assert cpu.get(WORK + 812, 2) == 321 and cpu.get(WORK + 816, 2) == 123
            caller_cases += 1
            instructions += cpu.steps

    def region(start, end, source):
        data = image[2048 + start - base:2048 + end - base] if source == 'EXE' else overlay[start - 0x80100000:end - 0x80100000]
        return {'image': source, 'start': hex(start), 'end_exclusive': hex(end), 'sha256': sha(data)}

    result = {'schema': 'class20-03-story-predicate-source/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
              'helper_cases': helper_cases, 'caller_cases': caller_cases,
              'instructions_executed': instructions, 'hooks': [],
              'helper': {'argument': 'unsigned flag ID in a0', 'bank': hex(BANK),
                         'read': 'u8[bank + (id >> 3)]', 'mask': '0x80 >> (id & 7)',
                         'result': '0 or1; true exactly when selected bit set'},
              'caller': {'entry': '0x80102884', 'phase': 2, 'parameter_mask': '0x20',
                         'argument_source': 'unsigned halfword work+0x362',
                         'true': 'action3; complete halfword0A clear',
                         'false': 'action0 phase2 retained; unrelated packet/distance ignored',
                         'always': 'flags328 mask0x20 set; timer32C/work330 unchanged'},
              'native_mapping': {'semantic_ids': '0..2047', 'getter': 'RuntimeSession::story_flag(id)',
                                 'physical_packing': 'native LSB-first is neutral; retain semantic ID',
                                 'outside_domain': 'leave predicate unknown; helper has no source bound'},
              'selected_authored_examples': examples,
              'regions': [region(0x8001da58, 0x8001da8c, 'EXE'),
                          region(0x80102884, 0x801029b4, 'ST00')],
              'limits': ['All work/global flag bytes are explicit synthetic memory premises.',
                         'No flag creation/event progression, allocation clock or live encounter is proved.',
                         'Native getter mapping is semantic layout equivalence, not a retail save-byte copy.',
                         'Source helper reads beyond native bank for IDs>=2048; native false is not admissible there.',
                         'No original bytes, code listing, tables, assets or retail flag state exported.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('helper_cases', 'caller_cases', 'instructions_executed', 'hooks')}, indent=2))


if __name__ == '__main__':
    main()

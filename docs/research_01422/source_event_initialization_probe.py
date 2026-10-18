#!/usr/bin/env python3
"""Hash-guarded, read-only original source event initialization differentials.

Full original fresh flag initialization and scene reset execute unchanged.
ST00 area event prefixes execute unchanged from the table branch to their
common tail; the excluded render/object/camera consumers are not acknowledged.
Only authored memory, measured scalar mutations and hashes are serialized.
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

FLAGS, WORK, PLAYER = 0x800BE378, 0x800C4C48, 0x800B51B0


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'dat', 'fixture', 'proof'):
        ap.add_argument('--' + name, required=True, type=Path)
    a = ap.parse_args()
    if a.fixture.exists() or a.proof.exists():
        raise SystemExit('Refusing to overwrite source event initialization evidence')
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    rows, steps = [], 0
    def cpu():
        c = OverlayProbe(image, base, overlay)
        c.r[29] = 0x8000
        return c
    def put(c, address, blob):
        for i, value in enumerate(blob):
            c.put(address + i, 1, value)
    def get(c, address, size):
        return bytes(c.get(address + i, 1) for i in range(size))
    for seed in range(16):
        c = cpu()
        before = bytes((i * 37 + seed * 11) & 255 for i in range(256))
        put(c, FLAGS, before)
        c.r[4] = 0
        c.run(0x8001D8C0)
        assert not c.return_only_hooks
        after = get(c, FLAGS, 256)
        expected = bytearray(256)
        for offset, value in ((0xB0, 16128), (0xB4, 24), (0xB8, 128), (0xBC, 0)):
            struct.pack_into('<I', expected, offset, value)
        expected[1216 >> 3] |= 0x80 >> (1216 & 7)
        assert after == expected
        rows.append(['flags', seed, before.hex(), after.hex(), '-', '-', '-'])
        steps += c.steps
    for seed, fresh_mode in itertools.product(range(8), (0, 1, 255)):
        c = cpu()
        before = bytes((i * 53 + seed * 17) & 255 for i in range(168))
        put(c, WORK, before)
        c.r[4] = fresh_mode
        c.run(0x8001D928)
        assert not c.return_only_hooks
        after = get(c, WORK, 168)
        assert after == bytes(4) + before[4:]
        rows.append(['reset', fresh_mode, before.hex(), after.hex(), '-', '-', '-'])
        steps += c.steps
    for area, flags00 in itertools.product(range(3), (0, 1, 2, 3, 127, 255)):
        c = cpu()
        before = bytearray((i * 41 + flags00 * 13) & 255 for i in range(0x5F4))
        before[0] = flags00
        flags_before = bytes((i * 17 + area * 23) & 255 for i in range(256))
        put(c, PLAYER, before)
        put(c, FLAGS, flags_before)
        c.put(0x8009888C, 4, 0x11223344)
        c.r[8], c.r[5] = area, PLAYER
        c.run(0x80100460, 0x801004F0)
        assert not c.return_only_hooks
        after = get(c, PLAYER, 0x5F4)
        flags_after = get(c, FLAGS, 256)
        expected = before[:]
        expected_flags = bytearray(flags_before)
        if area == 0:
            expected[0] &= 0xFC
            expected[8:12] = bytes(4)
            expected_flags[1540 >> 3] |= 0x80 >> (1540 & 7)
        else:
            expected[0x44E] = 1
            if area == 1:
                expected[0] |= 3
        assert after == expected and flags_after == expected_flags
        global_after = c.get(0x8009888C, 4)
        assert global_after == (0x80000000 if area == 0 else 0x11223344)
        rows.append(['area', area, before.hex(), after.hex(),
                     flags_before.hex(), flags_after.hex(), global_after])
        steps += c.steps
    a.fixture.parent.mkdir(parents=True, exist_ok=True)
    a.fixture.write_text('kind\tselector\tauthored_before\tactual_after\tflags_before\tflags_after\tglobal8009888c\n' +
                         ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    def region(lo, hi, source):
        blob = image[2048 + lo - base:2048 + hi - base] if source == 'resident' else overlay[lo - 0x80100000:hi - 0x80100000]
        return {'start': hex(lo), 'end_exclusive': hex(hi), 'sha256': sha(blob)}
    proof = {'schema': 'mml-source-event-initialization/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
             'original_instruction_cases': len(rows), 'original_instructions_executed': steps,
             'checks': {'fresh_flag_bank': 16, 'scene_reset_retained_tail': 24, 'area_event_prefix': 18},
             'accepted_cases_helper_hooks': [], 'fixture_sha256': sha(a.fixture.read_bytes()),
             'regions': [region(0x8001D8C0, 0x8001D974, 'resident'),
                         region(0x80067E18, 0x80067E6C, 'resident'),
                         region(0x80100460, 0x801004F0, 'overlay')],
             'initialization_scope': ['Fresh8001D8C0 resets256bytes then bank-resident word defaults and flag1216.',
                                     'Original8001D928 resets only eventwork00..03, not its168byte whole buffer.',
                                     'ST00 area0 sets1540 through the original flag setter; native follows this explicit lifecycle branch.',
                                     'ST00 area1/2 player presentation flags are supported in this prefix only.'],
             'limits': ['All before-work/flag/player bytes are authored synthetic inputs.',
                        'Boot BSS state, whole NewGame callback and source-relative tick ordering remain unproved.',
                        'The area initializer render/object/camera/audio consumers outside the accepted prefix are excluded.',
                        'Busy80098A72 initial value is not inferred; no complete event or natural transition claimed.',
                        'No retail code/table/assets/save payload exported.'],
             'retail_parity': False, 'live_gameplay': False, 'hardware_tested': False}
    a.proof.parent.mkdir(parents=True, exist_ok=True)
    a.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps({key: proof[key] for key in ('status', 'checks', 'original_instructions_executed')}))


if __name__ == '__main__':
    main()

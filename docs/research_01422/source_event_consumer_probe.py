#!/usr/bin/env python3
"""Unchanged original event1/player/voice logic with authored safe work.

Only authored seeds, scalar outcomes, measured byte differences and hashes are
written. Native driver receipts describe logical intent, not SPU playback.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
WORK, PLAYER, VOICES = 0x800C4C48, 0x800B51B0, 0x800A33E0
LOCK, CURRENT, PENDING, ACTIVE = 0x800BC050, 0x800C0BFA, 0x800C5680, 0x8009AE40


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def pattern(size, seed):
    return bytes((i * 37 + seed * 11) & 255 for i in range(size))


def diff(before, after):
    return ','.join(f'{i}:{v}' for i, v in enumerate(after) if v != before[i]) or '-'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'dat', 'fixture', 'proof'):
        ap.add_argument('--' + name, type=Path, required=True)
    a = ap.parse_args()
    if a.fixture.exists() or a.proof.exists():
        raise SystemExit('Refusing to overwrite consumer differential evidence')
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base, loadsize = struct.unpack_from('<II', image, 24)
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    assert base <= 0x80098A72 and 0x80098A74 <= base + loadsize
    cold_boot_busy = struct.unpack_from('<h', image, 2048 + 0x80098A72 - base)[0]
    assert cold_boot_busy == 0
    def cpu():
        c = OverlayProbe(image, base, overlay)
        c.r[29] = 0x700000
        return c
    def put(c, address, blob):
        for i, value in enumerate(blob):
            c.put(address + i, 1, value)
    def get(c, address, size):
        return bytes(c.get(address + i, 1) for i in range(size))
    contexts = list(itertools.product((0, 1, 3, 255), (0, 1, 3), (0, 13, 14),
                    (0, 0xC0), (0, 255), (0, 1), (0, 1)))
    rng = random.Random(0x42238)
    contexts = contexts[::3]  # 192 cross-product cases plus directed/random edges.
    contexts += [(1, 1, 13, 0, 0, 0, 0), (1, 1, 14, 0, 0, 0, 0),
                 (1, 1, 14, 0, 0, 1, 0), (1, 1, 0, 0x40, 0, 0, 0)]
    contexts += [tuple(rng.choice(domain) for domain in
                 ((0, 1, 2, 3, 255), (0, 1, 2, 3, 255), (0, 13, 14, 255),
                  (0, 0x40, 0x80, 0xC0, 255), (0, 1, 127, 128, 255), (0, 1, 255),
                  (0, 1, 2, 0xFFFFFFFF))) for _ in range(128)]
    rows, steps = [], 0
    for seed, (flags, state, action, busy174, busy177, byte93, lock) in enumerate(contexts):
        c = cpu()
        player = bytearray(pattern(0x5F4, seed))
        player[0], player[8], player[9], player[10] = flags, state, action, 2
        player[0x174], player[0x177] = busy174, busy177
        work = bytearray(pattern(168, seed + 1))
        work[0], work[1], work[4] = 1, 1, 1
        voices = pattern(24 * 54, seed + 2)
        put(c, PLAYER, player); put(c, WORK, work); put(c, VOICES, voices)
        masks = tuple(((seed * (17 + i * 13)) ^ (0xA55A >> i)) & 65535 for i in range(4))
        for address, values in ((PENDING, masks[:2]), (ACTIVE, masks[2:])):
            for i, value in enumerate(values): c.put(address + 2 * i, 2, value)
        c.put(LOCK, 4, lock); c.put(CURRENT, 2, 0x1234); c.put(0x800C1B93, 1, byte93)
        c.r[4] = WORK
        c.run(0x801010C4)
        assert not c.return_only_hooks
        calls = [x['target'] for x in c.calls]
        ready = c.get(WORK + 4, 1) == 2
        assert calls.count('0x8001b2f0') == (2 if ready else 0)
        assert calls.count('0x800742c8') == (2 if ready and lock != 1 else 0)
        after_masks = tuple(c.get(address + 2 * i, 2)
                            for address in (PENDING, ACTIVE) for i in range(2))
        rows.append([seed, flags, state, action, busy174, busy177, byte93, lock,
                     *masks, int(ready), c.get(LOCK, 4), c.get(CURRENT, 2), *after_masks,
                     diff(player, get(c, PLAYER, 0x5F4)), diff(work, get(c, WORK, 168)),
                     diff(voices, get(c, VOICES, 24 * 54))])
        steps += c.steps
    reset_cases = []
    for seed in range(16):
        c = cpu()
        for address, width in ((LOCK, 4), (PENDING, 2), (PENDING + 2, 2), (ACTIVE, 2), (ACTIVE + 2, 2)):
            c.put(address, width, seed * 17 + 19)
        c.run(0x8006B57C, 0x8006B584)
        c.run(0x800729D0, 0x800729F0)
        assert not c.return_only_hooks
        assert c.get(LOCK, 4) == 0
        assert all(c.get(address, 2) == 0 for address in (PENDING, PENDING + 2, ACTIVE, ACTIVE + 2))
        reset_cases.append(seed); steps += c.steps
    busy_cases = []
    for entry, selector in ((0x80017FE8, 2), (0x800182D4, 1), (0x8001840C, 1)):
        for busy in (-32768, -1, 1, 32767):
            c = cpu(); c.r[16] = 0x80098A68; c.r[18] = 1
            c.put(0x80098A69 if entry == 0x80017FE8 else 0x80098A6B, 1, selector)
            c.put(0x80098A72, 2, busy); c.put(0x80098A74, 2, 0)
            c.run(entry, 0x80012ECC)
            assert not c.return_only_hooks and c.get(0x80098A72, 2) == 0
            busy_cases.append({'entry': hex(entry), 'before_busy': busy, 'after_busy': 0,
                               'stop_before_untranslated_consumer': '0x80012ECC'})
            steps += c.steps
    a.fixture.parent.mkdir(parents=True, exist_ok=True)
    a.fixture.write_text('seed flags state action busy174 busy177 byte93 lock pending0 pending1 active0 active1 ready afterlock aftervoice afterpending0 afterpending1 afteractive0 afteractive1 playerdiff workdiff voicediff\n' +
                         ''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    def region(lo, hi):
        blob = overlay[lo-0x80100000:hi-0x80100000] if lo >= 0x80100000 else image[2048+lo-base:2048+hi-base]
        return {'start': hex(lo), 'end_exclusive': hex(hi), 'sha256': sha(blob)}
    proof = {'schema': 'mml-source-event-consumer/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
             'consumer_cases': len(rows), 'logical_driver_reset_cases': len(reset_cases),
             'busy_completion_prefix_cases': busy_cases, 'original_instructions_executed': steps,
             'accepted_cases_helper_hooks': [], 'fixture_sha256': sha(a.fixture.read_bytes()),
             'boot_busy': {'address': '0x80098A72', 'value': cold_boot_busy,
                           'source': 'hash-guarded executable load image', 'scene_reset_equivalence': False},
             'regions': [region(lo, hi) for lo, hi in ((0x80042238, 0x80042314),
                 (0x8004324C, 0x80043278), (0x8001B2F0, 0x8001B314), (0x80071220, 0x80071298),
                 (0x800742C8, 0x80074398), (0x801010C4, 0x8010114C),
                 (0x8006B57C, 0x8006B584), (0x800729D0, 0x800729F0))],
             'source_stop_writes': '54-byte voice slots+00/+04 halfwords,+1D byte; pending masks OR; active AND complement of entire pending masks',
             'source_lock_rejection': 'lock exactly1 returns-1; original player ignores both driver returns',
             'atomic_native_receipt_policy': 'both22/23 calls must fit and be consumed before player/event/driver publication',
             'limits': ['All work/global inputs are authored, not a live retail scene.',
                        'Driver reset slices prove only logical fields; excluded hardware consumers are not acknowledged.',
                        'Cold boot busy0 is measured image data; fresh scene/NewGame does not imply busy reset.',
                        'Busy completion prefixes stop before callback80012ECC; whole fade scheduling remains unported.',
                        'Phases2+ camera/command/message consumers and natural tutorial completion remain unfinished.',
                        'No original code/table/assets/save/sample payload is exported.'],
             'audible_playback_proved': False, 'retail_parity': False, 'hardware_tested': False}
    a.proof.parent.mkdir(parents=True, exist_ok=True)
    a.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps({k: proof[k] for k in ('status', 'consumer_cases', 'logical_driver_reset_cases', 'original_instructions_executed')}))


if __name__ == '__main__':
    main()

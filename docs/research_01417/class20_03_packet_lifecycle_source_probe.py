#!/usr/bin/env python3
"""Original hit delivery and outer packet clear on two consecutive callbacks."""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--dat', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    if a.out.exists():
        raise SystemExit('Refusing to overwrite packet evidence')
    cases = []
    steps = 0
    for flags, damage_flag, damage in itertools.product((4, 6), (0, 0x08000000, 0x20000000), (1, 16)):
        c = OverlayProbe(image, base, overlay)
        for offset, size, value in ((0, 1, 1), (1, 1, 0x20), (2, 1, 3), (8, 1, 1),
                                    (9, 1, 0), (10, 1, 1), (172, 1, 0), (808, 2, flags),
                                    (110, 2, 128), (812, 2, 20), (828, 4, damage_flag | damage)):
            c.put(0x1000 + offset, size, value)
        # Explicit USA stats-selector0 context, as in the existing initializer
        # proof. Only native output scalars are emitted, never this owner table.
        c.put(0x80098844, 4, 0x8008f060)
        for address in (0x8003648c, 0x8007fe50, 0x800322a8, 0x80032488,
                        0x8002c8cc, 0x800341bc, 0x8001a050):
            c.hook(address)
        frames = []
        for index in range(2):
            c.r[4], c.r[29], c.r[31] = 0x1000, 0x8000, 0
            before_calls = len(c.calls)
            c.run(0x8010226c)
            hp = c.get(0x106e, 2, True)
            expected = 128 - damage if flags & 2 and damage_flag else 128
            assert hp == expected and c.get(0x133c, 4) == 0
            assert c.get(0x1009, 1) == 0
            hit_calls = [call for call in c.calls[before_calls:] if call['target'] == '0x801041b0']
            assert len(hit_calls) == bool(flags & 2)
            frames.append({'hp': hp, 'packet33C': 0, 'hit_consumer_calls': len(hit_calls),
                           'direct_calls': c.calls[before_calls:]})
        steps += c.steps
        cases.append({'flags328': flags, 'packet_input': damage_flag | damage,
                      'hp_before': 128, 'frames': frames})
    evidence = {'schema': 'class20-03-packet-lifecycle-source-cases/1',
                'status': 'SOURCE_BACKED_PARTIAL', 'case_count': len(cases),
                'callback_count': len(cases) * 2, 'instructions_executed': steps,
                'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
                'cases': cases,
                'hooks': ['8007FE50 explicit RNG0', '8003648C explicit detection false',
                          '800322A8/80032488 animation consumers excluded',
                          '8002C8CC/800341BC proxy/effect consumers excluded',
                          '8001A050 sound consumer excluded'],
                'limits': ['Synthetic explicit HP128 and stats-selector0; not live retail context.',
                           'Hit consumer801041B0 and outer packet clear execute original instructions.',
                           'Nonlethal stationary cases; defeat/allocation/projectile delivery excluded.',
                           'No original bytes, table payloads, function tables or assets serialized.']}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps({'case_count': len(cases), 'callback_count': len(cases) * 2,
                      'instructions_executed': steps, 'evidence_sha256': sha(a.out.read_bytes())}, indent=2))


if __name__ == '__main__':
    main()

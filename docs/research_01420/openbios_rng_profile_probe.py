#!/usr/bin/env python3
"""Read-only installed OpenBIOS RNG profile proof, emitting scalar/hash evidence.

This deliberately proves a reference BIOS service, not the retail game's seed,
consumer chronology, save ownership or native parity. No BIOS/retail bytes are
written. Inputs and synthetic probe memory are separate.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415')]
from mml_exe_research import Image, index_function
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE

BIOS_SHA = '713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
ROM = 0xBFC00000
RAND, SRAND = ROM + 0x60E4, ROM + 0x6110
SEED = 0x8448
MASK = 0xFFFFFFFF


def sha(data):
    return hashlib.sha256(data).hexdigest()


def immediate_address(first, second):
    assert first >> 26 == 15 and second >> 26 == 9
    assert first >> 16 & 31 == second >> 21 & 31
    low = second & 65535
    if low & 32768:
        low -= 65536
    return ((first & 65535) * 65536 + low) & MASK


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bios', required=True, type=Path)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    a = p.parse_args()
    if a.out.exists():
        raise SystemExit('Refusing to replace evidence')
    bios, image = a.bios.read_bytes(), a.exe.read_bytes()
    if sha(bios) != BIOS_SHA or sha(image) != SUPPORTED_EXE:
        raise SystemExit('Unsupported installed BIOS or owner EXE profile')
    exe = Image(image)
    word = lambda offset: struct.unpack_from('<I', bios, offset)[0]
    bss_start = immediate_address(word(0x330), word(0x334))
    bss_end = immediate_address(word(0x338), word(0x33C))
    assert bss_start == 0x4AA0 and bss_end == 0xB05C
    assert bss_start <= SEED < bss_end
    # Load the exact existing ROM into the inherited integer-instruction probe.
    # It is not patched; no A0 hook or native generator is substituted here.
    cpu = OverlayProbe(image, exe.base, b'')
    cpu.mem.update({ROM + i: v for i, v in enumerate(bios)})
    instruction_total, rows = 0, []
    seeds = [0, 1, MASK, 0x80000000, 0x13579BDF, 0xCAFEBABE]
    seeds += [((index * 0x9E3779B1) ^ 0xA5A5A5A5) & MASK for index in range(250)]
    for seed in seeds:
        cpu.steps = 0
        cpu.r[4], cpu.r[31] = seed, 0
        cpu.run(SRAND)
        assert cpu.get(SEED, 4) == seed
        instruction_total += cpu.steps
        prior = seed
        # Consecutive calls exercise shared state and wraparound, not just outputs.
        for ordinal in range(1, 17):
            cpu.steps = 0
            cpu.r[31] = 0
            cpu.run(RAND)
            after = (prior * 1103515245 + 12345) & MASK
            value = (after >> 16) & 32767
            assert cpu.get(SEED, 4) == after and cpu.r[2] == value
            instruction_total += cpu.steps
            if ordinal in (1, 16):
                rows.append({'authored_seed': seed, 'ordinal': ordinal,
                             'seed_before': prior, 'seed_after': after,
                             'bios_return': cpu.r[2]})
            prior = after
    boot = index_function(exe, exe.entry, 2048)
    result = {
        'schema': 'mml-openbios-rng-reference-profile/1',
        'status': 'MATCHES_MEASURED_FIELDS',
        'bios_sha256': sha(bios), 'owner_exe_sha256': sha(image),
        'reference_rand_entry': hex(RAND), 'reference_srand_entry': hex(SRAND),
        'reference_seed_physical': hex(SEED),
        'reference_seed_cached_alias': hex(SEED | 0x80000000),
        'installed_bios_bss_start': hex(bss_start),
        'installed_bios_bss_end_exclusive': hex(bss_end),
        'seed_inside_installed_bss': True,
        'bios_rand_region_sha256': sha(bios[0x60E4:0x6110]),
        'bios_srand_region_sha256': sha(bios[0x6110:0x611C]),
        'bios_bss_initialization_region_sha256': sha(bios[0x330:0x358]),
        'seed_cases': len(seeds), 'consecutive_calls_per_case': 16,
        'rand_calls_exercised': len(seeds) * 16,
        'source_instructions_executed': instruction_total,
        'scalar_checkpoint_rows': rows,
        'game_entry': hex(exe.entry),
        'game_entry_direct_call_candidates': boot['callees'],
        'game_entry_cfg_truncated': boot['bounded_scan_truncated'],
        'hooks': [], 'live_reference_capture': False,
        'retail_bios_profile_proved': False, 'game_seed_proved': False,
        'game_shared_consumer_order_proved': False, 'retail_parity': False,
        'limits': [
            'This exact installed OpenBIOS profile is a reference implementation.',
            'Cold BIOS BSS clearing does not establish the seed at game entry.',
            'BIOS startup, warm reset, save states and earlier consumers are excluded.',
            'Game-entry CFG lists direct calls; it does not execute the whole boot.',
            'Seed inputs are authored; no live game randomness was captured.',
            'No instruction bytes, BIOS image or retail payload are serialized.',
        ],
    }
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in (
        'status', 'seed_cases', 'rand_calls_exercised',
        'source_instructions_executed', 'game_seed_proved')}))


if __name__ == '__main__':
    main()

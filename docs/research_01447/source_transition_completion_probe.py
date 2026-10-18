#!/usr/bin/env python3
"""Bounded original 80015840 state-1 completion tail on authored owners.

The callback instructions are unchanged. Three separately owned callees return
through synthetic hooks so this probe establishes the caller's completion-store
order only; it does not claim retail CD timing or full transition chronology.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01422')]
from cook_actor_combat import OwnerExe, OWNER_SHA
from class60_06_source_probe import FastProbe

WORK = 0x1000
TRANSITION = 0x800C3558
HELPERS = (0x80038648, 0x8001F3C8)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    if output.exists() or output == SOURCE or SOURCE in output.parents:
        raise ValueError('Choose a new private output outside source')
    image = args.exe.read_bytes()
    owner = OwnerExe(image)
    cases = []
    total_steps = 0
    for seed in range(16):
        cpu = FastProbe(image, owner.base, b'')
        for i in range(96):
            cpu.put(WORK + i, 1, (seed * 29 + i * 17 + 5) & 255)
        cpu.put(WORK + 1, 1, 1)
        cpu.put(TRANSITION + 20, 1, 2)
        for helper in HELPERS:
            cpu.hook(helper)
        cpu.r[4], cpu.r[29], cpu.r[31] = WORK, 0x700000, 0
        cpu.run(0x80015840)
        calls = tuple(int(item['target'], 16) for item in cpu.calls)
        if calls != HELPERS:
            raise ValueError('Unexpected state-1 helper order')
        if cpu.get(TRANSITION + 20, 1) != 0 or cpu.get(WORK, 1) != 3 or cpu.get(WORK + 1, 1) != 0:
            raise ValueError('Original completion tail did not publish expected stores')
        total_steps += cpu.steps
        cases.append({'seed': seed, 'steps': cpu.steps})
    block = owner.read(0x80015840, 0xAC)
    proof = {
        'schema': 'mml-source-transition-completion/1',
        'status': 'SOURCE_BACKED_PARTIAL',
        'source_exe_sha256': OWNER_SHA,
        'source_callback': '0x80015840',
        'callback_range_sha256': sha(block),
        'authored_state1_cases': len(cases),
        'original_callback_instructions': total_steps,
        'ordered_return_only_helpers': [hex(value) for value in HELPERS],
        'transition_status_store': {'instruction': '0x800158cc', 'address': '0x800c356c', 'value': 0},
        'camera_initializer': '0x80038648',
        'work_completion_stores': {'work_plus_00': 3, 'work_plus_01': 0},
        'callback_instruction_patches': 0,
        'return_only_helper_hooks': len(HELPERS),
        'retail_parity': False,
        'physical_hardware_tested': False,
        'limits': [
            'Proves the unchanged state-1 caller tail after two explicitly returned helper owners.',
            'State-0 transport/spawn through 80016D64 is an earlier separately tested native receipt.',
            'Does not execute camera initialization or event continuation helper bodies.',
            'Native acknowledgement requires separate area replacement, spawn, and gameplay-camera receipts.',
            'Authored memory is not a live retail transition trace.'
        ]
    }
    output.mkdir(parents=True)
    (output / 'source-transition-completion-proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps(proof, indent=2))


if __name__ == '__main__':
    main()

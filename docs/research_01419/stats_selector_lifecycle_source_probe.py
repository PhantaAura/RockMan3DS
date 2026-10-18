#!/usr/bin/env python3
"""Original fresh-boot selector writer/control-flow evidence on authored inputs.

Only scalar/hash/selected call-edge metadata is emitted. No original bytes,
instruction listings, tables, save payloads, combat records or assets escape.
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
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources

SELECTOR = 0x800C1B6B
SNAPSHOT_BYTE = 0x800B5F4F
FRESH_HELPERS = (0x8007CF90, 0x8007C478, 0x8007DF38, 0x80063EF0,
                 0x80053788, 0x800665E8, 0x8001D440)


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--dat', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    if args.out.exists():
        raise SystemExit('Refusing to overwrite selector evidence')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE:
        raise SystemExit('Unsupported owner executable')
    exe = Image(image)
    overlay = next(row['payload'] for row in iter_resources(args.dat / 'ST00.BIN')
                   if row['load'] == 0x80100000 and 'PROGBIN' in row['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise SystemExit('Unsupported tutorial overlay')
    total_steps = 0

    def fresh():
        cpu = OverlayProbe(image, exe.base, overlay)
        cpu.r[29], cpu.r[31] = 0x7000, 0
        return cpu

    boot = index_function(exe, exe.entry)
    assert any(int(call['site'], 16) == 0x800680A0 and int(call['target'], 16) == 0x80011C90
               for call in boot['callees'])
    bootstrap = []
    for initial_helper_result in (0, 1):
        cpu = fresh()
        cpu.r[28] = 0x80097864
        cpu.hook(0x80067FF8, initial_helper_result)
        cpu.hook(0x80012938)
        cpu.hook(0x80011F04)
        cpu.run(0x80011C90, 0x80012E10)
        assert cpu.r[4] == 0 and cpu.r[5] == 0x800131FC
        bootstrap.append({'authored_initial_helper_result': initial_helper_result,
                          'registration_helper': '0x80012e10', 'callback_argument_a1': hex(cpu.r[5]),
                          'slot_argument_a0': cpu.r[4], 'instructions': cpu.steps,
                          'direct_calls': cpu.calls})
        total_steps += cpu.steps
    cpu = fresh()
    cpu.r[4], cpu.r[5] = 0, 0x800131FC
    cpu.hook(0x8007FF70)
    cpu.run(0x80012E10, 0x8007FF10)
    assert cpu.r[4] == 0x800131FC
    registration = {'source_helper': '0x80012e10', 'BIOS_wrapper': '0x8007ff10',
                    'entrypoint_argument_a0': hex(cpu.r[4]), 'instructions': cpu.steps,
                    'hooks': cpu.return_only_hooks,
                    'limits': ['Critical section and actual thread stack/GP ownership excluded.']}
    total_steps += cpu.steps
    cpu = fresh()
    cpu.r[31] = 0x7770
    cpu.run(0x8007FF10, 0xB0)
    assert cpu.r[9] == 0x0E and cpu.r[10] == 0xB0 and cpu.r[31] == 0x7770
    registration.update({'BIOS_function_id': cpu.r[9], 'BIOS_gateway': hex(cpu.r[10]),
                         'original_trampoline_instructions': cpu.steps})
    total_steps += cpu.steps
    narrow = []
    restored = []
    consumed = []
    for value in range(256):
        cpu = fresh()
        cpu.r[2] = (value * 0x1020304) & 0xFFFFFFFF
        cpu.put(SELECTOR, 1, value)
        cpu.run(0x8001332C, 0x8001333C)
        assert cpu.get(SELECTOR, 1) == 1
        narrow.append({'previous_selector': value, 'selector_after': cpu.get(SELECTOR, 1),
                       'instructions': cpu.steps})
        total_steps += cpu.steps
        cpu = fresh()
        cpu.put(SELECTOR, 1, value ^ 0xA5)
        cpu.put(SNAPSHOT_BYTE, 1, value)
        cpu.run(0x80065B18)
        assert cpu.get(SELECTOR, 1) == value
        restored.append({'authored_byte800B5F4F': value, 'previous_selector': value ^ 0xA5,
                         'selector_after': cpu.get(SELECTOR, 1), 'instructions': cpu.steps})
        total_steps += cpu.steps
        cpu = fresh()
        cpu.r[28] = 0x80097864
        cpu.put(SELECTOR, 1, value)
        cpu.run(0x80064718)
        pointer = cpu.get(0x80098844, 4)
        assert pointer == (0x8008F060 if value == 2 else 0x8008EDC4)
        consumed.append({'selector': value, 'table_pointer80098844': hex(pointer),
                         'selector_equals_two': value == 2, 'instructions': cpu.steps})
        total_steps += cpu.steps
    dispatch = []
    for prior_selector, prior_state in ((0, 0), (1, 1), (2, 2), (127, 17), (255, 255)):
        cpu = fresh()
        cpu.put(SELECTOR, 1, prior_selector)
        cpu.put(0x80098B1C, 1, prior_state)
        cpu.put(0x80098B1D, 1, prior_state)
        for address in FRESH_HELPERS:
            cpu.hook(address)
        cpu.run(0x800131FC, 0x80013248)
        assert cpu.get(SELECTOR, 1) == 1
        assert cpu.get(0x80098B1C, 1) == 1 and cpu.get(0x80098B1D, 1) == 0
        assert cpu.indirect == [{'site': '0x80013240', 'target': '0x8001326c'}]
        assert exe.word(0x80080894) == 0x8001326C
        dispatch.append({'previous_selector': prior_selector, 'previous_dispatch_state': prior_state,
                         'selector_after': cpu.get(SELECTOR, 1),
                         'dispatch_state80098B1C_after': cpu.get(0x80098B1C, 1),
                         'dispatch_state80098B1D_after': cpu.get(0x80098B1D, 1),
                         'source_table_index': 0, 'source_table_entry': '0x80080894',
                         'original_indirect_dispatch': cpu.indirect,
                         'instructions': cpu.steps, 'hooks': cpu.return_only_hooks})
        total_steps += cpu.steps
    regions = []
    for lo, hi in ((0x80068000, 0x800680A8), (0x80011C90, 0x80011CD8),
                   (0x80012E10, 0x80012E60), (0x800131FC, 0x800133CC),
                   (0x80065B18, 0x80065E04), (0x80064718, 0x80064794)):
        regions.append({'start': hex(lo), 'end_exclusive': hex(hi),
                        'sha256': sha(image[exe.offset(lo):exe.offset(hi)])})
    result = {'schema': 'mml-stats-selector-lifecycle/1', 'status': 'SOURCE_BACKED_PARTIAL',
              'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
              'boot_direct_edge': {'site': '0x800680a0', 'target': '0x80011c90'},
              'bootstrap_registration': bootstrap, 'BIOS_registration': registration,
              'fresh_dispatch_cases': dispatch, 'fresh_writer_cases': narrow,
              'snapshot_copy_cases': restored, 'stats_table_consumer_cases': consumed,
              'code_regions': regions, 'probe_instructions_executed': total_steps,
              'fresh_boot_selector': 1, 'restore_source_semantic_name': None,
              'native_universal_default_justified': False, 'BIOS_seed_proved': False,
              'retail_parity': False,
              'limits': ['Setup/graphics/game-subsystem calls are authored return-only hooks in dispatch cases.',
                         'Thread callback argument is proven; BIOS scheduling/stack/GP are not exercised.',
                         'Snapshot copy block caller and persistence meaning remain unknown.',
                         'Aliases, bulk copies and other overlays may overwrite selector later.',
                         'No combat-profile bytes, original tables, game payloads or save data are serialized.']}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'status': result['status'], 'fresh_writer_cases': len(narrow),
                      'snapshot_copy_cases': len(restored), 'consumer_cases': len(consumed),
                      'dispatch_cases': len(dispatch), 'bootstrap_cases': len(bootstrap),
                      'instructions': total_steps, 'fresh_boot_selector': 1,
                      'universal_default': False}))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Bounded source scalar checks for ordinary PL00 action2 and idle turns.

Uses the owner's hash-identified local executable. Outputs scalar constants,
addresses, hashes and check counts only. Motion integration, sound, combat and
jump prerequisites are explicitly excluded from these synthetic checks.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from player_contact_source_probe import CPU, SUPPORTED_EXE

WORK = 0x1000

def prepared(image, base, action=2, substate=0, direction=0, previous=0, held=0):
    c = CPU(image, base)
    c.r[29] = 0x4000
    c.r[4] = WORK
    c.r[5] = 0
    c.run(0x800425fc)  # exact control-profile copy, including derived masks
    c.steps = 0
    # Synthetic return-only hooks. No owner instruction payload is exported.
    for addr in (0x80041e90, 0x8001a008, 0x80040b68, 0x800406dc, 0x80041044):
        c.put(addr, 4, (31 << 21) | 8)
        c.put(addr + 4, 4, (2 << 11) | 33)  # synthetic zero return value
    c.put(WORK + 8, 1, 1)
    c.put(WORK + 9, 1, action)
    c.put(WORK + 10, 1, substate)
    c.put(WORK + 0x112, 1, direction)
    c.put(WORK + 0x113, 1, previous)
    c.put(WORK + 0x11c, 2, held)
    c.put(WORK + 0x175, 1, 1)
    c.put(WORK + 0xac, 1, 1)
    c.put(WORK + 0xad, 1, 1)
    return c

def call(c, entry):
    c.r[4] = WORK
    c.r[31] = 0
    c.r[2] = 0
    c.steps = 0
    c.run(entry)

def check(c, offset, size, expected, signed=False):
    actual = c.get(WORK + offset, size, signed)
    if actual != expected:
        raise ValueError('scalar field mismatch at ' + hex(offset) + ': ' + str((actual, expected)))

def scalar_checks(image, base):
    initialization = 0
    requests = ((4,5,9,7),(7,0,5,9),(9,7,0,5),(5,9,7,0))
    for direction in range(4):
        for previous in range(4):
            c = prepared(image, base, direction=direction, previous=previous)
            call(c, 0x8003cd4c)
            check(c, 0x44, 2, 512, True)
            check(c, 0x116, 2, (0,1024,2048,-1024)[direction], True)
            check(c, 0xa, 1, 2)
            check(c, 0xac, 1, requests[direction][previous])
            check(c, 0x113, 1, 0)
            check(c, 0x174, 1, 4)
            initialization += 1
    animation_gates = 0
    for direction in (0,2):
        for control in (-1,0,1):
            c = prepared(image, base, substate=2, direction=direction)
            c.put(WORK + 0xac, 1, 4 if direction == 0 else 9)
            c.put(WORK + 0xad, 1, 4 if direction == 0 else 9)
            c.put(WORK + 0xa7, 1, control)
            c.put(WORK + 0x44, 2, 512)
            call(c, 0x8003cd4c)
            check(c, 0xa, 1, 4 if control == -1 else 2)
            check(c, 0xac, 1, 1 if control == -1 else (4 if direction == 0 else 9))
            check(c, 0x44, 2, 512, True)
            animation_gates += 1
    release = 0
    for direction in (0,2):
        c = prepared(image, base, substate=5, direction=direction)
        c.put(WORK + 0x44, 2, 512)
        call(c, 0x8003cd4c)
        check(c, 0xa, 1, 1 if direction == 0 else 6)
        check(c, 0x9, 1, 0 if direction == 0 else 2)
        check(c, 0x4c, 2, 48, True)
        check(c, 0xac, 1, 1 if direction == 0 else 10)
        release += 1
    for byte2 in (-1,0,1,2):
        c = prepared(image, base, substate=6, direction=2)
        c.put(WORK + 0xa6, 1, byte2)
        c.put(WORK + 0x44, 2, 512)
        c.put(WORK + 0x116, 2, 2048)
        call(c, 0x8003cd4c)
        check(c, 0x9, 1, 0 if byte2 == 1 else 2)
        check(c, 0x44, 2, 0 if byte2 == 1 else 512, True)
        check(c, 0x116, 2, 0 if byte2 == 1 else 2048, True)
        release += 1
    predicates = 0
    for held, action, substate, direction in ((0,0,0,0),(16,2,4,0),(64,2,0,2),
                                               (80,2,0,2),(0x2010,1,0,0),(0x2040,1,0,0)):
        c = prepared(image, base, substate=4, held=held)
        call(c, 0x8003f97c)
        check(c, 0x9, 1, action)
        check(c, 0xa, 1, substate)
        check(c, 0x112, 1, direction)
        predicates += 1
    turns = 0
    for action in (2,4):
        for modifier in (0,0x2000):
            if action == 2 and modifier: continue
            for held in (0x400,0x800,0xc00):
                for before in (-80,-40,-1,0,1,40,80):
                    c = prepared(image, base, action=action, substate=4 if action == 2 else 0,
                                 held=held | modifier)
                    c.put(WORK + 0xb4, 2, before)
                    call(c, 0x8003cd4c if action == 2 else 0x8003d6dc)
                    step, cap = (40,40) if action == 2 else ((2,32) if modifier else (4,72))
                    if held == 0xc00: expected = 0
                    elif held == 0x800: expected = 0 if before < 0 else min(before + step,cap)
                    else: expected = 0 if before > 0 else max(before - step,-cap)
                    check(c, 0xb4, 2, expected, True)
                    check(c, 0x56, 2, expected & 4095)
                    turns += 1
    return {'initialization_cases_passed': initialization, 'animation_gate_cases_passed': animation_gates,
            'release_cases_passed': release, 'ordinary_forward_predicate_cases_passed': predicates,
            'run_and_idle_turn_cases_passed': turns,
            'excluded_calls': ['0x80041E90 position integration','0x8001A008 sound',
                               '0x80040B68 firing prerequisites','0x800406DC running jump owner',
                               '0x80041044 interaction prerequisite'],
            'limits': ['scalar blocks with explicit return-only hooks, not a complete retail tick',
                       'reverse release was source-probed only; wider deceleration owner remains unproved',
                       'no collision, camera, scripted owner, combat or progression parity claim']}

def retail_window(trace):
    data = json.loads(trace.read_text())
    frames = data['frames'][:25]
    start = data['initial']
    if len(frames) != 25: raise ValueError('retail window requires 25 actual samples')
    for index, row in enumerate(frames):
        if row['input']['held_psx_mask'] != 16 or row['velocity_work'] != [512,0,0]:
            raise ValueError('retail ordinary-forward window changed')
        expected = start['position'][:]
        expected[2] -= 32 * (index + 1)
        if row['position'] != expected or row['state'] != [1,2,2 if index < 5 else 4]:
            raise ValueError('retail forward scalar sequence changed')
        if row['animation_request'] != (4 if index < 5 else 1):
            raise ValueError('retail animation request sequence changed')
        if row['guest_vsync'] - start['guest_vsync'] != 2 * (index + 1):
            raise ValueError('retail player update cadence changed')
    return {'trace_sha256': hashlib.sha256(trace.read_bytes()).hexdigest(),
            'sample_phase': data['sample_phase'], 'checked_initial_state': start['state'],
            'checked_player_updates': 25, 'observed_forward_velocity_work': 512,
            'observed_delta_z_world_units_per_update': -32, 'observed_vsyncs_per_player_update': 2,
            'startup_request': 4, 'sustained_request': 1, 'sustained_request_first_update_index': 5,
            'remaining_samples_excluded': len(data['frames']) - 25,
            'exclusion_reason': 'tutorial event removes input; wider scripted owner remains unported',
            'status': 'MEASURED_RETAIL_WINDOW_ONLY; native comparison is a separate artifact'}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--retail-trace', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    image = a.exe.read_bytes()
    digest = hashlib.sha256(image).hexdigest()
    if digest != SUPPORTED_EXE: raise ValueError('unsupported executable identity')
    base = struct.unpack_from('<I',image,24)[0]
    regions = [('idle_action',0x8003cb20,0x8003cbf8),('slow_action1',0x8003cbf8,0x8003cd4c),
               ('ordinary_action2',0x8003cd4c,0x8003d124),('directional_setup',0x8003d124,0x8003d1e0),
               ('idle_turn_action4',0x8003d6dc,0x8003d7d4),('action2_predicate',0x8003f97c,0x8003fda8),
               ('ordinary_selectors',0x80040380,0x80040574),('idle_turn_selector',0x800405f4,0x80040630),
               ('direction_yaw_integration_wrapper',0x80041e90,0x80041ef4)]
    result = {'format':'player-locomotion-source-evidence/1', 'status':'SOURCE_BACKED_PARTIAL',
              'PHYSICS_1TO1':False, 'exe_sha256':digest, 'source_scalar_checks':scalar_checks(image,base),
              'actual_retail_forward_window':retail_window(a.retail_trace),
              'source_blocks':[{'id':name,'start':hex(begin),'end_exclusive':hex(end),
                                'sha256':hashlib.sha256(image[begin-base+2048:end-base+2048]).hexdigest()}
                               for name,begin,end in regions],
              'payload_policy':'scalar facts, addresses, hashes and counts only; no executable bytes or instruction listings'}
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result['source_scalar_checks']))

if __name__ == '__main__': main()

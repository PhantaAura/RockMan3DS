#!/usr/bin/env python3
"""Generate reproducible control inputs, not fictional retail golden outputs."""
import argparse
import hashlib
import json
from pathlib import Path

# Masks are the existing source-backed USA profile 0, not guessed PS1 bindings.
FORWARD, BACKWARD, TURN_LEFT, TURN_RIGHT, JUMP = 0x10, 0x40, 0x400, 0x800, 0x4000


def scenarios():
    flat = 'Verified unobstructed level floor; same position, heading, floor/contact, and player work in both captures.'
    specs = [
        ('IDLE', [(60, 0, 0, 0)], flat, 'Measure baseline and idle camera.'),
        ('FLAT_ACCEL', [(90, FORWARD, 0, 0)], flat, 'Measure acceleration and maximum speed.'),
        ('RELEASE', [(30, FORWARD, 0, 0), (60, 0, 0, 0)], flat, 'Measure release/braking behavior.'),
        ('BRAKE', [(30, FORWARD, 0, 0), (30, BACKWARD, 0, 0), (30, 0, 0, 0)], flat, 'Reverse-input braking; BRAKE is an experiment label, not a proven retail state.'),
        ('REVERSE', [(30, FORWARD, 0, 0), (60, BACKWARD, 0, 0)], flat, 'Measure reversal and backward movement.'),
        ('TURN_45', [(8, TURN_RIGHT, 0, 0), (30, 0, 0, 0)], flat, '45 is a requested comparison goal; 8 input ticks do not establish a measured 45-degree turn.'),
        ('TURN_90', [(16, TURN_RIGHT, 0, 0), (30, 0, 0, 0)], flat, '90 is a requested comparison goal; no expected heading is synthesized.'),
        ('TURN_180', [(32, TURN_RIGHT, 0, 0), (30, 0, 0, 0)], flat, '180 is a requested comparison goal; no expected heading is synthesized.'),
        ('TURN_REVERSE', [(16, TURN_RIGHT, 0, 0), (16, TURN_LEFT, 0, 0), (30, 0, 0, 0)], flat, 'Measure turn acceleration, direction changes, and release.'),
        ('JUMP', [(1, JUMP, 0, 0), (89, 0, 0, 0)], flat, 'Measure tap launch, apex, gravity, and landing.'),
        ('JUMP_HOLD', [(30, JUMP, 0, 0), (60, 0, 0, 0)], flat, 'Measure hold-dependent behavior; do not infer from tap.'),
        ('LAND', [(1, JUMP, 0, 0), (119, 0, 0, 0)], flat, 'Observe landing/contact and state/animation handoff.'),
        ('AIR_CONTROL', [(1, JUMP, 0, 0), (30, FORWARD | TURN_RIGHT, 0, 0), (60, 0, 0, 0)], flat, 'Measure air movement and turning.'),
        ('SLOPE_UP', [(90, FORWARD, 0, 0)], 'Evidence-identified ascending floor with matching source primitive and entry pose.', 'Measure floor adhesion and speed on ascent.'),
        ('SLOPE_DOWN', [(90, FORWARD, 0, 0)], 'Evidence-identified descending floor with matching source primitive and entry pose.', 'Measure floor adhesion and descent.'),
        ('LEDGE', [(60, FORWARD, 0, 0), (60, 0, 0, 0)], 'Evidence-identified ledge with clear landing surface; matching start distance and heading.', 'Observe floor loss, fall, and landing.'),
        ('STEP', [(90, FORWARD, 0, 0)], 'Evidence-identified step; known source height and matching entry pose.', 'Measure step acceptance/contact response.'),
        ('WALL', [(90, FORWARD, 0, 0)], 'Evidence-identified wall with matching contact/approach angle.', 'Measure rejection/sliding, including speed/state.'),
        ('KNOCKBACK', [(120, 0, 0, 0)], 'Same real enemy/effect and frame-controlled damage stimulus, HP, position, and recovery state.', 'Input alone cannot generate the required hit; keep UNKNOWN until stimulus is reproduced.'),
        ('FIRE_MOVE', [(60, FORWARD, 1, 0)], flat, 'Native fire_pressed is a per-tick bridge flag; actual retail pad schedule must be measured and documented before parity.'),
        ('LOCK_MOVE', [(60, FORWARD, 0, 1)], 'Same real eligible target, range, facing, and lock state in both runs.', 'Native lock_held is a bridge flag; empty-target replay is not retail lock-on parity.'),
    ]
    return [{'id': name, 'status': 'UNKNOWN', 'retail_reference_available': False,
             'start_requirements': environment, 'purpose': purpose,
             'segments': [{'ticks': count, 'held_psx_mask': held, 'fire_pressed': bool(fire),
                           'lock_held': bool(lock), 'advance_dialogue': False}
                          for count, held, fire, lock in segments]}
            for name, segments, environment, purpose in specs]


def rows_for(spec):
    rows = []
    for segment in spec['segments']:
        for _ in range(segment['ticks']):
            rows.append(f"{len(rows)} {segment['held_psx_mask']} {int(segment['fire_pressed'])} "
                        f"{int(segment['lock_held'])} {int(segment['advance_dialogue'])}")
    return '\n'.join(rows) + '\n'


def write_corpus(output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    records = scenarios()
    for spec in records:
        payload = rows_for(spec)
        path = output / (spec['id'] + '.tsv')
        with path.open('x') as stream:
            stream.write(payload)
        spec['input_file'] = path.name
        spec['input_file_sha256'] = hashlib.sha256(payload.encode()).hexdigest()
        spec['ticks'] = sum(segment['ticks'] for segment in spec['segments'])
    result = {'format': 'mml-control-scenario-corpus/1', 'control_profile': 'USA-profile-0',
              'PHYSICS_1TO1': False,
              'limitations': ['Input experiments contain no expected retail measurements.',
                              'Do not map input ticks to GPU vsyncs without a measured update cadence.',
                              'Retail/native comparison requires matching initial work, environment, and sample phase.',
                              'Turn-angle names are goals, not predicted outcomes.',
                              'Fire/lock bridge flags require evidence-backed retail pad translation.'],
              'source_evidence': ['src/player/player_runtime.c:l3_pl00_install_control_profile_800425fc',
                                  'platform/3ds/source/main.cpp:make_psx_held'],
              'scenarios': records}
    with (output / 'CONTROL_SCENARIOS.json').open('x') as stream:
        stream.write(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try:
        result = write_corpus(args.output)
    except OSError as error:
        parser.exit(2, str(error) + '\n')
    print(str(args.output / 'CONTROL_SCENARIOS.json') + ': ' + str(len(result['scenarios'])) + ' scenarios; no retail golden traces')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

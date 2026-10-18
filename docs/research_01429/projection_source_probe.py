#!/usr/bin/env python3
"""Recover the original area-config projection owner without helper hooks.

The exact owner EXE/ST00 inputs remain local. Original instructions execute
unchanged through the 40-byte writer and fog/shift consumers. Emitted authored
fixtures contain no retail records; the separate owner proof retains only
scalar observations. Neither proves live camera timing or hardware GTE parity.
"""

import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01425'),
               str(SOURCE / 'docs/research_01415'), str(SOURCE / 'docs/research_01414')]
from source_camera_source_probe import Probe
from cook_actor_combat import OwnerExe, OWNER_SHA
from tutorial_actor_identity_probe import OVERLAY_SHA, iter_resources


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fill(cpu, address, data):
    for offset, value in enumerate(data):
        cpu.put(address + offset, 1, value)


def read(cpu, address, size):
    return bytes(cpu.get(address + offset, 1) for offset in range(size))


def signed16(value):
    value &= 65535
    return value - 65536 if value & 32768 else value


def signed32(value):
    value &= 0xffffffff
    return value - 0x100000000 if value & 0x80000000 else value


def quotient(value):
    # Original division truncates toward zero. The native fog setter retains
    # the low halfword through its original CTC2 control27 operation.
    numerator = -320 * value
    return -(abs(numerator) // 384) if numerator < 0 else numerator // 384


def verify_writer(cpu, record, destination):
    expected = read(cpu, record, 40)
    assert read(cpu, 0x800bc7f8, 40) == expected
    assert read(cpu, 0x800c0dc0, 40) == expected
    assert read(cpu, 0x8009a350, 40) == expected
    assert read(cpu, destination, 8) == read(cpu, 0x800badb0, 8)
    distance = struct.unpack_from('<H', expected, 36)[0]
    assert cpu.get(0x800bc81c, 2) == distance
    assert signed32(cpu.control2[27]) == signed16(quotient(distance))
    assert cpu.control2[28] == 0x01400000
    assert cpu.get(0x1f800078, 1) == expected[35]
    assert not cpu.return_only_hooks
    return distance


def shift_result(cpu, shift, distance):
    before = cpu.steps
    cpu.r[31] = 0
    cpu.run((0x8003697c, 0x800369c0, 0x80036a1c)[shift])
    scaled = distance << shift
    selected = scaled if scaled < 13107 else distance
    assert cpu.get(0x1f800074, 1) == shift
    assert signed32(cpu.control2[27]) == signed16(quotient(selected))
    assert cpu.control2[28] == 0x01400000
    assert not cpu.return_only_hooks
    return {
        'shift': shift, 'fog_input': selected,
        'dqa': signed32(cpu.control2[27]), 'dqb': cpu.control2[28],
        'scratch_1f800006': cpu.get(0x1f800006, 2),
        'scratch_1f800076': cpu.get(0x1f800076, 2),
        'instructions': cpu.steps - before,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'st00', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose a NEW owner-local output directory outside source')
    image = args.exe.read_bytes()
    owner = OwnerExe(image)
    overlay = next(record['payload'] for record in iter_resources(args.st00)
                   if record['load'] == 0x80100000 and 'PROGBIN' in record['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise ValueError('Unsupported ST00 program overlay identity')
    generator = random.Random(0x01429)
    authored_rows = []
    authored_steps = 0
    shift_steps = 0
    distances = [0, 1, 3276, 3277, 6553, 6554, 13106, 13107,
                 16383, 32767, 32768, 65535]
    distances += [generator.randrange(65536) for _ in range(68)]
    for seed, distance in enumerate(distances):
        cpu = Probe(image, owner.base)
        cpu.r[29] = 0x700000
        record = 0x2000 + seed % 4
        destination = 0x1000 + seed % 4
        config = bytearray(generator.randrange(256) for _ in range(40))
        struct.pack_into('<H', config, 36, distance)
        fill(cpu, record, config)
        fill(cpu, 0x800badb0, bytes(generator.randrange(256) for _ in range(8)))
        cpu.r[4:6] = [destination, record]
        cpu.run(0x800371a0)
        assert verify_writer(cpu, record, destination) == distance
        writer_steps = cpu.steps
        authored_steps += writer_steps
        for shift in range(3):
            result = shift_result(cpu, shift, distance)
            shift_steps += result['instructions']
            authored_rows.append([seed, record & 3, destination & 3, bytes(config).hex(),
                                  distance, shift, result['fog_input'], result['dqa'],
                                  result['dqb'], result['scratch_1f800006'],
                                  result['scratch_1f800076']])

    owner_rows = []
    owner_steps = 0
    for area in range(5):
        cpu = Probe(image, owner.base)
        cpu.mem.update({0x80100000 + index: value for index, value in enumerate(overlay)})
        cpu.r[16], cpu.r[29] = 0x800c1b61, 0x700000
        cpu.put(0x800c1b61, 1, area)
        cpu.run(0x80100528, 0x800371a0)
        destination, record = cpu.r[4:6]
        assert record == 0x8011ac54 + area * 40
        cpu.run(0x800371a0, 0x80100564)
        distance = verify_writer(cpu, record, destination)
        writer_steps = cpu.steps
        owner_steps += writer_steps
        shifts = []
        for shift in range(3):
            result = shift_result(cpu, shift, distance)
            owner_steps += result['instructions']
            shifts.append(result)
        owner_rows.append({'area': area, 'config_record': hex(record),
                           'field_offset': 36, 'raw_800bc81c': distance,
                           'writer_instructions': writer_steps, 'shifts': shifts})

    cpu = Probe(image, owner.base)
    cpu.r[29] = 0x700000
    cpu.run(0x80013648, 0x800371a0)
    destination, record = cpu.r[4:6]
    assert destination == 0x800808d0 and record == 0x800808a8
    cpu.run(0x800371a0, 0x80013660)
    cold_distance = verify_writer(cpu, record, destination)
    cold_steps = cpu.steps

    fixture = ('seed\tsource_alignment\tdestination_alignment\tauthored_config\tdistance\tshift\tselected_fog_input\tdqa\tdqb\tscratch06\tscratch76\n' +
               ''.join('\t'.join(map(str, row)) + '\n' for row in authored_rows)).encode()
    blocks = [('config_writer', 0x800371a0, 0x80037440),
              ('shift0', 0x8003697c, 0x800369c0),
              ('shift1', 0x800369c0, 0x80036a1c),
              ('shift2', 0x80036a1c, 0x80036a88),
              ('fog', 0x8007a1cc, 0x8007a230)]
    proof = {
        'schema': 'mml-projection-initialization/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'owner_exe_sha256': OWNER_SHA, 'overlay_sha256': OVERLAY_SHA,
        'writer': '800371A0', 'destination': '800BC7F8', 'field_offset': 36,
        'field_address': '800BC81C', 'field_type': 'unsigned_halfword',
        'stage_caller': '80100528..80100564',
        'stage_config_base': '8011AC54', 'stage_config_stride': 40,
        'fresh_boot_caller': '80013648..80013660',
        'authored_writer_cases': len(distances), 'authored_shift_cases': len(authored_rows),
        'owner_stage_writer_cases': len(owner_rows), 'owner_stage_shift_cases': len(owner_rows) * 3,
        'fresh_boot_writer_cases': 1,
        'unchanged_original_instructions': authored_steps + shift_steps + owner_steps + cold_steps,
        'instruction_patches': 0, 'helper_hooks': 0,
        'authored_fixture_sha256': sha(fixture),
        'source_blocks': [{'id': name, 'start': hex(start), 'end_exclusive': hex(end),
                          'sha256': sha(owner.read(start, end - start))} for name, start, end in blocks],
        'retail_payload_exported': False, 'retail_parity': False, 'hardware_tested': False,
        'limits': ['Original caller selection and scalar ownership only; no complete stage lifecycle.',
                   'Authored records exercise copy alignment, fog narrowing and shift thresholds.',
                   'Control transfers are interpreted; no independent hardware GTE comparison.',
                   'This does not bind current scratch camera, follow behavior or script chronology.'],
    }
    private = {**proof, 'owner_stage_scalars': owner_rows,
               'fresh_boot_record': hex(record), 'fresh_boot_800bc81c': cold_distance}
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out / 'projection-config-authored.tsv').write_bytes(fixture)
    (args.out / 'projection-public-proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    (args.out / 'projection-owner-proof.json').write_text(json.dumps(private, indent=2) + '\n')
    print(json.dumps(proof, indent=2))


if __name__ == '__main__':
    main()

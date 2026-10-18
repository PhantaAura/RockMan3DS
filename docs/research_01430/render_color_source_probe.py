#!/usr/bin/env python3
"""Bounded original color ownership with authored fixtures, no retail exports.

Executes unchanged original FarColor/BackColor setters, area-config writer,
flat packet constructor and DPCS call-site slice. DPCS arithmetic is a restricted
documented model; it is not independently measured hardware GTE behavior.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01429')]
from projection_source_probe import Probe as BaseProbe, fill, read
from cook_actor_combat import OwnerExe, OWNER_SHA
from tutorial_actor_identity_probe import OVERLAY_SHA, iter_resources


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Probe(BaseProbe):
    def cop2(self, word):
        if word >> 21 & 31 >= 16 and word & 0x1ffffff == 0x780010:
            # Published DPCS(sf=1,lm=0), including the intermediate signed IR
            # saturation. Only byte-owned FC and IR0 0..4096 are admitted here.
            rgb = self.data2[6]
            assert 0 <= self.data2[8] <= 4096
            assert all(0 <= value <= 4080 and value % 16 == 0
                       for value in self.control2[21:24])
            fifo = rgb & 0xff000000
            for channel in range(3):
                mac = (rgb >> (channel * 8) & 255) << 16
                difference = ((self.control2[21 + channel] << 12) - mac) >> 12
                difference = max(-32768, min(32767, difference))
                mac = (difference * self.data2[8] + mac) >> 12
                self.data2[25 + channel] = mac & 0xffffffff
                self.data2[9 + channel] = max(-32768, min(32767, mac)) & 0xffffffff
                fifo |= max(0, min(255, mac >> 4)) << (channel * 8)
            self.data2[20:23] = self.data2[21:23] + [fifo]
            self.gte_commands += 1
        else:
            super().cop2(word)


def controls():
    return [(0xa5a50000 + i * 0x101) & 0xffffffff for i in range(32)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'st00', 'out', 'render'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose a NEW private output directory outside source')
    image = args.exe.read_bytes()
    owner = OwnerExe(image)
    overlay = next(record['payload'] for record in iter_resources(args.st00)
                   if record['load'] == 0x80100000 and 'PROGBIN' in record['internal'].upper())
    if sha(overlay) != OVERLAY_SHA:
        raise ValueError('Unsupported ST00 program overlay identity')
    rng = random.Random(0x01430)
    rows, steps, commands = [], 0, 0
    for kind, address, registers in [('F', 0x8007b0f8, (21,22,23)),
                                     ('B', 0x8007b0d8, (13,14,15))]:
        for seed in range(128):
            rgb = [rng.getrandbits(32) for _ in range(3)]
            if seed < 8:
                rgb = [0,1,255,256,0x80000000,0xffffffff,0x10000000,0x0fffffff][seed:seed+1] * 3
            cpu = Probe(image, owner.base)
            cpu.control2 = controls()
            before = list(cpu.control2)
            cpu.r[4:7] = rgb
            cpu.run(address)
            expected = [(value << 4) & 0xffffffff for value in rgb]
            for index in range(32):
                assert cpu.control2[index] == (expected[registers.index(index)]
                                              if index in registers else before[index])
            assert not cpu.return_only_hooks
            rows.append([kind, *rgb, *expected])
            steps += cpu.steps

    for seed in range(80):
        cpu = Probe(image, owner.base)
        cpu.control2 = controls()
        back = cpu.control2[13:16]
        cpu.r[29] = 0x700000
        record = 0x2000 + seed % 4
        destination = 0x1000 + seed % 4
        authored = bytes(rng.randrange(256) for _ in range(40))
        fill(cpu, record, authored)
        fill(cpu, 0x800badb0, bytes(rng.randrange(256) for _ in range(8)))
        cpu.r[4:6] = [destination, record]
        cpu.run(0x800371a0)
        assert read(cpu, 0x800bc7f8, 40) == authored
        assert cpu.control2[21:24] == [value << 4 for value in authored[32:35]]
        assert cpu.control2[13:16] == back
        assert not cpu.return_only_hooks
        steps += cpu.steps

    for seed in range(128):
        prefix = rng.getrandbits(32)
        cpu = Probe(image, owner.base)
        cpu.r[16], cpu.r[17], cpu.r[9] = 0x3000, prefix, 0x4000
        cpu.run(0x800251d4, 0x800251f0)
        constructed = cpu.get(0x3054, 4)
        assert constructed == (prefix & 0x02ffffff) | 0x2c000000
        rows.append(['P', prefix, 1, constructed])
        steps += cpu.steps

    for seed in range(256):
        rgbc = rng.getrandbits(32)
        far = [rng.randrange(256) << 4 for _ in range(3)]
        ir0 = (0,1,17,2047,2048,4095,4096)[seed % 7]
        cpu = Probe(image, owner.base)
        cpu.r[2], cpu.r[16], cpu.r[29] = 0x3000, 0x4000, 0x7000
        cpu.put(0x3000, 4, rgbc)
        cpu.put(0x701c, 4, 1024)
        cpu.put(0x7030, 4, 9)
        cpu.put(0x1f800102, 1, 3)
        cpu.control2[21:24] = far
        cpu.data2[8] = ir0
        cpu.run(0x800252f4, 0x80025334)
        output = cpu.data2[22]
        assert cpu.get(0x3000, 4) == output
        assert output >> 24 == rgbc >> 24
        assert cpu.gte_commands == 1 and not cpu.return_only_hooks
        rows.append(['D', rgbc, *far, ir0, output, *cpu.data2[25:28], *cpu.data2[9:12]])
        steps += cpu.steps
        commands += cpu.gte_commands

    owner_rows = []
    owner_steps = 0
    for area in range(5):
        cpu = Probe(image, owner.base)
        cpu.control2 = controls()
        before_back = list(cpu.control2[13:16])
        cpu.mem.update({0x80100000 + i: value for i, value in enumerate(overlay)})
        cpu.r[16], cpu.r[29] = 0x800c1b61, 0x700000
        cpu.put(0x800c1b61, 1, area)
        cpu.run(0x80100528, 0x800371a0)
        record = cpu.r[5]
        assert record == 0x8011ac54 + area * 40
        cpu.run(0x800371a0, 0x80100564)
        far_rgb = [cpu.get(record + 32 + i, 1) for i in range(3)]
        assert cpu.control2[21:24] == [value << 4 for value in far_rgb]
        assert cpu.control2[13:16] == before_back
        assert not cpu.return_only_hooks
        owner_rows.append({'area': area, 'config_record': hex(record),
                           'far_rgb': far_rgb, 'control21_23': cpu.control2[21:24]})
        owner_steps += cpu.steps

    # Aggregate only from the already cooked retained-byte geometry. No source
    # geometry, colors, texture pages or materials are copied into the release.
    render = args.render.read_bytes()
    if len(render) < 80 or render[:8] != b'L3RG\x03\0P\0':
        raise ValueError('Expected retained-byte L3RG v3')
    count, offset = struct.unpack_from('<I', render, 32)[0], struct.unpack_from('<I', render, 56)[0]
    if offset < 80 or offset + count * 24 > len(render):
        raise ValueError('Primitive color section outside cooked data')
    prefixes = [struct.unpack_from('<I', render, offset + i * 24)[0] for i in range(count)]
    rgb_counts = Counter(prefix & 0xffffff for prefix in prefixes)
    aggregate = {'input_sha256': sha(render), 'primitive_count': count,
                 'distinct_rgb_count': len(rgb_counts), 'neutral_808080_count': rgb_counts[0x808080],
                 'nonneutral_rgb_count': count - rgb_counts[0x808080]}

    fixture = ('# Authored original CPU color slices; DPCS uses a documented restricted GTE model.\n' +
               ''.join(' '.join(map(str, row)) + '\n' for row in rows)).encode()
    blocks = [('far_setter', 0x8007b0f8, 0x8007b118),
              ('back_setter', 0x8007b0d8, 0x8007b0f8),
              ('area_config_writer', 0x800371a0, 0x80037440),
              ('positive_flat_quad_color', 0x800251d4, 0x800251f0),
              ('positive_dpcs_output', 0x800252f4, 0x80025334)]
    proof = {'schema': 'mml-source-render-color/1', 'status': 'SOURCE_BACKED_PARTIAL',
             'owner_exe_sha256': OWNER_SHA, 'overlay_sha256': OVERLAY_SHA,
             'far_setter_cases': 128, 'back_setter_cases': 128,
             'authored_area_config_cases': 80, 'owner_area_config_cases': 5,
             'flat_packet_cases': 128, 'dpcs_cases': 256,
             'unchanged_original_instructions': steps + owner_steps,
             'modeled_dpcs_commands': commands, 'helper_hooks': 0, 'instruction_patches': 0,
             'fixture_sha256': sha(fixture), 'terrain_color_aggregate': aggregate,
             'source_blocks': [{'id': name, 'start': hex(start), 'end_exclusive': hex(end),
                                'sha256': sha(owner.read(start, end - start))}
                               for name, start, end in blocks],
             'retail_payload_exported': False, 'retail_parity': False, 'hardware_tested': False,
             'native_depth_cue_bound': False,
             'references': ['https://psx-spx.consoledev.net/geometrytransformationenginegte/',
                            'https://psx-spx.consoledev.net/graphicsprocessingunitgpu/',
                            'https://github.com/devkitPro/citro3d/blob/master/include/c3d/texenv.h'],
             'limits': ['FarColor is control21..23; BackColor13..15 has separate source ownership.',
                        'Area config +32..34 reaches FarColor; no live/current render cadence is inferred.',
                        'DPCS is a restricted published arithmetic model, not independent hardware GTE measurement.',
                        'IR0 inputs are authored; current H, matrix, depth and per-primitive command ownership are unbound.',
                        'No generic GPU fog, guessed far RGB, source shading completion or visual acceptance is claimed.']}
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out / 'source-render-color.tsv').write_bytes(fixture)
    (args.out / 'render-color-public-proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    (args.out / 'render-color-owner-proof.json').write_text(json.dumps({**proof, 'owner_far_colors': owner_rows}, indent=2) + '\n')
    print(json.dumps({key: proof[key] for key in ('status', 'unchanged_original_instructions',
          'modeled_dpcs_commands', 'fixture_sha256', 'terrain_color_aggregate')}))


if __name__ == '__main__':
    main()

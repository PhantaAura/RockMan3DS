#!/usr/bin/env python3
"""Reproduce selected enemy evidence from existing owner files; hashes/scalars only."""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from tutorial_actor_identity_probe import OverlayProbe, SOURCE, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


def sha(data):
    return hashlib.sha256(data).hexdigest()


def floor_scalar_checks(image, base, overlay):
    bounds_bytes = overlay[0x1BED4:0x1BEE0]
    bounds = list(struct.unpack('<6h', bounds_bytes))
    assert bounds == [-60, 60, -60, 60, -128, 0]
    passed = hits = steps = 0
    for enemy in (False, True):
        for x in (-572, -571, -512, -1, 0, 511, 512, 571, 572):
            for z in (-572, 0, 572):
                for y in (-200, -128, -1, 0, 127, 128):
                    for code in (1, 3, 15):
                        for flags in (0, 0x8000):
                            c = OverlayProbe(image, base, b'')
                            c.r[4:8] = [0x1000, 0x8011BED4 if enemy else 0, 0, 0x20]
                            c.r[29] = 0x8000
                            c.hook(0x8004B1E0, 1)  # one authored candidate cell index
                            c.put(0x1F800118, 2, 0)
                            c.put(0x1F800048, 4, 0x5000); c.put(0x5000, 2, 1)
                            c.put(0x1F80004C, 4, 0x2000)
                            if enemy:
                                c.mem.update({0x8011BED4 + i: v for i, v in enumerate(bounds_bytes)})
                            for i, v in enumerate((x, y, z)): c.put(0x1000 + 2 * i, 2, v)
                            for i, v in enumerate((-512, 512, -512, 512)): c.put(0x3000 + 2 * i, 2, v)
                            c.put(0x2014, 4, 0x2500); c.put(0x2508, 4, 0x3000)
                            c.put(0x3008, 4, 0x4000); c.put(0x300C, 4, 1)
                            c.put(0x4000, 2, -128); c.put(0x4002, 2, 0)
                            c.put(0x4004, 2, code); c.put(0x4006, 2, flags)
                            c.run(0x80048EA4)
                            result = c.r[2] if c.r[2] < 0x80000000 else c.r[2] - 0x100000000
                            edge, top = (572, 128) if enemy else (512, 0)
                            admitted = -edge < x < edge and -edge < z < edge and y < top and flags == 0
                            assert result == (-128 if admitted else 32767)
                            assert [c.get(0x1000 + 2 * i, 2, True) for i in range(3)] == [x, y, z]
                            passed += 1; hits += admitted; steps += c.steps
    return {'bounds_address': '0x8011bed4', 'bounds_signed_x0_x1_z0_z1_y0_y1': bounds,
            'bounds_sha256': sha(bounds_bytes), 'floor_call_site': '0x801025a8',
            'floor_wrapper': '0x80048e6c', 'floor_core': '0x80048ea4',
            'fresh_delta_xyz': [0, 0, 0], 'passed_cases': passed,
            'floor_hits': hits, 'instructions_executed': steps, 'codes': [1, 3, 15],
            'profiles': ['class20/35 null bounds', 'class20/03 installed bounds'],
            'hooks': ['0x8004b1e0 returns one authored candidate cell index'],
            'source_position_preserved': True,
            'shared_slope_interval_evidence': 'floor08-source-evidence.json (240 scalar cases)',
            'limits': ['Authored synthetic rectangles; no full retail tick parity.',
                       'Sloped floor intervals use separately verified shared helper; movement remains unported.']}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--dat', type=Path, required=True)
    ap.add_argument('--native-probe', type=Path, required=True)
    ap.add_argument('--cooked-ebd', type=Path, required=True)
    ap.add_argument('--cooked-map', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    image = a.exe.read_bytes()
    assert sha(image) == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert sha(overlay) == OVERLAY_SHA
    ebd = next(r['payload'] for r in iter_resources(a.dat / 'ST00_01.BIN') if r['load'] == 0x8016C000)
    word = lambda address: struct.unpack_from('<I', overlay, address - 0x80100000)[0]
    selected = {0x8011A4DC: 0x8011A480, 0x8011A480: 0x8010226C,
                0x8011BE9C: 0x80102410, 0x8011BEA0: 0x80102600,
                0x8011BEA4: 0x80102864, 0x8011C894: 0x80111CD0}
    for address, target in selected.items():
        assert word(address) == target
    row = struct.unpack_from('<4B3I', ebd, 4)
    assert row[:4] == (0x20, 3, 0, 0)
    assert [p - 0x8016C000 for p in row[4:]] == [0x314, 0x14A4, 0x364C]
    initializers = []
    for selector in (0, 2):
        c = OverlayProbe(image, base, overlay)
        c.mem.update({0x8016C000 + i: v for i, v in enumerate(ebd)})
        c.r[28], c.r[29] = 0x80097864, 0x8000
        c.put(0x800C1B6B, 1, selector)
        c.run(0x80064718)
        stats_table = c.get(0x80098844, 4)
        c.r[4], c.r[31], c.steps = 0x1000, 0, 0
        c.put(0x1000, 1, 1); c.put(0x1001, 1, 0x20); c.put(0x1002, 1, 3); c.put(0x1006, 1, 3)
        c.hook(0x80048E6C); c.hook(0x8003346C)
        c.run(0x80102410)
        hp = c.get(0x106E, 2, True)
        assert hp == (128 if selector == 2 else 64)
        assert c.get(0x10B0, 2) == hp
        assert c.get(0x10AC, 1) == 0 and c.get(0x1078, 4) == 0x8011BED4
        initializers.append({'raw_stats_selector': selector, 'context_high_byte': 0,
                             'stats_table': hex(stats_table), 'initial_hp': hp,
                             'initial_hp_copy_B0': hp, 'pose_request': 0,
                             'body_bounds_pointer': '0x8011bed4',
                             'hooks': c.return_only_hooks,
                             'floor_and_matrix_effects_excluded': True})
    damage_cases = []
    for hp, damage, flag in ((128, 16, 0x08000000), (3, 4, 0x08000000),
                              (3, 3, 0x08000000), (128, 16, 0), (128, 16, 0x20000000)):
        c = OverlayProbe(image, base, overlay)
        c.r[4], c.r[28], c.r[29] = 0x1000, 0x80097864, 0x8000
        c.put(0x1002, 1, 3); c.put(0x1008, 1, 1); c.put(0x1328, 2, 14)
        c.put(0x106E, 2, hp); c.put(0x133C, 4, flag | damage)
        c.put(0x80098844, 4, 0x8008F060); c.hook(0x8001A050)
        c.run(0x801041B0)
        after = c.get(0x106E, 2, True)
        assert after == (hp - damage if flag & 0x38000000 else hp)
        assert bool(c.get(0x1328, 2) & 0x40) == (after <= 0)
        damage_cases.append({'hp_before': hp, 'packet': hex(flag | damage), 'hp_after': after,
                             'defeat_latch': bool(c.get(0x1328, 2) & 0x40),
                             'action_after': c.get(0x1009, 1),
                             'packet_33C_retained': c.get(0x133C, 4) == (flag | damage),
                             'hooks': c.return_only_hooks})
    records = []
    for address in (0x8011A274, 0x8011A28C, 0x8011A2A4):
        off = address - 0x80100000
        pos = struct.unpack_from('<3h', overlay, off + 12)
        heading, parameter, context = overlay[off + 7] * 16, overlay[off + 3], overlay[off + 11]
        command = [str(a.native_probe.resolve()), str(a.cooked_ebd.resolve()),
                   *(str(v) for v in pos), str(heading), str(parameter), '0', str(context)]
        if a.cooked_map:
            command.extend([str(a.cooked_map.resolve()), '1'])
        native = json.loads(subprocess.check_output(command, text=True))
        records.append({'address': hex(address), 'class': '0x20', 'subtype': 3,
                        'parameter3': parameter, 'dispatch4': overlay[off + 4],
                        'origin': list(pos), 'heading': heading, 'context_high_byte': context,
                        'predicate': hex(struct.unpack_from('<I', overlay, off + 20)[0]),
                        'native_probe': native, 'native_stats_selector_is_explicit_synthetic_input': 0})
    def region(start, end, exe=False):
        data = image[2048 + start - base:2048 + end - base] if exe else overlay[start - 0x80100000:end - 0x80100000]
        assert len(data) == end - start
        return {'image': 'EXE' if exe else 'ST00', 'start': hex(start), 'end_exclusive': hex(end), 'sha256': sha(data)}
    out = {'schema': 'class20-03-source-evidence/1', 'status': 'SOURCE_BACKED_PARTIAL',
           'enemy_role': 'source-directed movement, damage/health and attack-child path; retail name unassigned',
           'sources': {'exe_sha256': sha(image), 'overlay_sha256': sha(overlay),
                       'ebd_sha256': sha(ebd), 'cooked_ebd_sha256': sha(a.cooked_ebd.read_bytes())},
           'identity': {'class': '0x20', 'subtype': 3, 'admitted_parameters3': [0, 128, 160], 'dispatch4': 0,
                        'callback': '0x8010226c', 'initializer': '0x80102410', 'state1': '0x80102600',
                        'release_state': '0x80102864', 'resource_row': 0, 'null_variant': True,
                        'resource_offsets': ['0x314', '0x14a4', '0x364c'],
                        'scale_Q12': [512, 512, 512], 'shade': 'UNKNOWN'},
           'spawn_path': {'C0_subtype': 33, 'C0_callback': '0x80119be8', 'fixed_E0_slot': 31,
                          'allocator': '0x800317ac', 'E0_callback': '0x801116dc',
                          'state0': '0x80111718', 'state1': '0x80111810', 'state2': '0x80111cd0',
                          'area1_descriptor': '0x8011a40c', 'qualifier': 1, 'id': 2,
                          'x_grid_inclusive': [-3, 1], 'z_grid_inclusive': [-16, -15],
                          'conditional_record_stride': 24, 'first_list': '0x8011a274',
                          'predicate_helper': '0x8001da58', 'predicate_admission': 'helper result ==0',
                          'predicate_flags_base': '0x800be378', 'predicate_bit_order': 'MSB first',
                          'qualifier_commit_work': '0x1c byte', 'id_commit_work': '0x1d byte'},
           'combat_fields': {'health': 'work+0x6e signed16', 'initial_health_copy': 'work+0xb0',
                             'packet': 'work+0x33c u32', 'damage_mask': '0x00ffffff',
                             'damage_flag_mask': '0x38000000', 'defeat_latch': 'work+0x328 bit0x40',
                             'hit_consumer': '0x801041b0', 'player_position': '0x800b51c4',
                             'turn_toward_player_call': '0x80102a18 ->0x80031bec',
                             'attack_call': '0x801030ec ->0x80104078',
                             'attack_child': {'class': '0x60', 'subtype': 6, 'allocator': '0x80031518'},
                             'packet_lifecycle': 'hit consumer retains packet; repeated delivery owner not ported'},
           'initial_hp_checks': initializers, 'damage_checks': damage_cases, 'records': records,
           'floor_query_evidence': floor_scalar_checks(image, base, overlay),
           'cooked_map_sha256': sha(a.cooked_map.read_bytes()) if a.cooked_map else None,
           'code_regions': [region(0x80064718, 0x80064794, True), region(0x8001DA58, 0x8001DA9C, True),
                            region(0x8010226C, 0x80102410), region(0x80102410, 0x80102600),
                            region(0x801029B4, 0x80102B44), region(0x80104078, 0x801041B0),
                            region(0x801041B0, 0x80104470), region(0x80111CD0, 0x8011216C),
                            region(0x80048EA4, 0x8004920C, True), region(0x8004B3C0, 0x8004B414, True)],
           'limits': ['Native model probes use explicit synthetic stats selector0; no live selector is asserted.',
                      'Floor bounds and supported intervals are verified; actual map queries retain explicit unsupported-data results.',
                      'Shade/texture residency unknown; presentation is neutral diagnostic geometry.',
                      'Full AI, animation ticking, player/projectile delivery and defeat completion remain unported.',
                      'Initial C0/03 is a RGB555 palette timeline, not command/flag storage.',
                      'No original bytes, disassembly, function tables or assets serialized.']}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(out, indent=2) + '\n')
    print('verified two explicit HP profiles, five hit-consumer cases and three owner-local source model instances')


if __name__ == '__main__':
    main()

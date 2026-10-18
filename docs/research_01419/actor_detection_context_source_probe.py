#!/usr/bin/env python3
"""Source-owned per-area detector scratch mode, without payload exports.

Executes original ST00 configuration address calculation and40-byte copy,
then original render setup byte transfer. Lighting/GTE display consumers are
excluded; no original configuration table bytes are written to the report.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / 'docs/research_01415'))
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--dat', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    image = a.exe.read_bytes(); assert hashlib.sha256(image).hexdigest() == SUPPORTED_EXE
    base = struct.unpack_from('<I', image, 24)[0]
    overlay = next(r['payload'] for r in iter_resources(a.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(overlay).hexdigest() == OVERLAY_SHA
    if a.out.exists(): raise SystemExit('Refusing to overwrite context source evidence')
    rows, steps = [], 0
    for area in range(5):
        c = OverlayProbe(image, base, overlay)
        c.r[16], c.r[29] = 0x800c1b61, 0x700000
        c.put(0x800c1b61, 1, area)
        c.run(0x80100528, 0x800371a0)
        config = c.r[5]
        assert config == 0x8011ac54 + area * 40
        for target in (0x800176dc, 0x8007b0f8, 0x8007a1cc): c.hook(target)
        c.run(0x800371a0, 0x80100564)
        assert [c.get(0x800c0dc0 + i, 1) for i in range(40)] == [c.get(config + i, 1) for i in range(40)]
        mode_word = c.get(config + 38, 2)
        assert c.get(0x800c0de6, 2) == mode_word
        steps += c.steps
        render_transfers = []
        for function, end in ((0x80023728, 0x8002375c), (0x800238b4, 0x800238f4)):
            for gate in (0, 1):
                d = OverlayProbe(image, base, overlay); d.r[29] = 0x700000
                d.put(0x800c0de6, 2, mode_word); d.put(0x80098824, 4, gate)
                d.put(0x1f800075, 1, 0xaa)
                d.run(function, end)
                mode = d.get(0x1f800075, 1)
                assert mode == ((mode_word & 255) if gate == 0 else 0xaa)
                render_transfers.append({'entry': hex(function), 'gate80098824': gate,
                                         'scratch_mode': mode, 'expected_copy': gate == 0})
                steps += d.steps
        rows.append({'area_id': area, 'configuration_pointer': hex(config),
                     'configuration_scalar_offset': 38, 'scalar_mode_word': mode_word,
                     'scalar_mode_low_byte': mode_word & 255,
                     'original_40_byte_copy_verified': True, 'render_transfers': render_transfers})
    result = {'schema': 'actor-detection-context-source-proof/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'exe_sha256': SUPPORTED_EXE, 'overlay_sha256': OVERLAY_SHA,
        'source_owner': {'configuration_argument': '0x80100528..0x80100560',
                         'configuration_copy': '0x800371A0',
                         'global_scalar': '0x800C0DE6', 'scratch_scalar': '0x1F800075'},
        'area_contexts': rows, 'render_copy_cases': 20, 'instructions_executed': steps,
        'retail_parity': False, 'no_original_payload_exported': True,
        'limits': ['Original stage/configuration identity for supported ST00areas0..4 only.',
                   'Scalar mode has no invented gameplay name or sound meaning.',
                   'Lighting/GTE setup consumer calls excluded; original40-byte CPU copy executes.',
                   'Renderer gate0/1 are explicitly authored contexts; complete runtime render cadence remains unverified.',
                   'Unsupported overlay/area contexts must hold rather than borrow a mode.']}
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))

if __name__ == '__main__': main()

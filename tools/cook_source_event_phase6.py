#!/usr/bin/env python3
"""Cook bounded owner-local phase 6 camera words and callback identities.

Only the exact supplied EXE/ST00 identities are supported. Original addresses
remain identity tokens; native code does not dereference them. This tool reads
owner files and creates a new private output directory. Its cooked payload is
excluded from source releases and does not establish complete scene playback.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

SOURCE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE / 'tools'))
from cook_actor_combat import OwnerExe, OWNER_SHA
from build_stage_catalog import iter_resources

OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
OVERLAY_BASE = 0x80100000
CAMERA_TOKEN = 0x8011af4c
CALLBACK_TOKEN = 0x8011b2e8
CONFIG_TOKEN = 0x8011ac54
CAMERA_WORDS = 231
TIMELINE_RECORDS = 18
HEADER_BYTES = 96
PACKAGE_BYTES = 1180


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def validate_package(package):
    """Reject malformed neutral data using the native loader's fixed contract."""
    require(len(package) == PACKAGE_BYTES, 'Phase6 package size rejected')
    header = struct.unpack_from('<4sHH6I', package)
    require(header[:3] == (b'L3P6', 1, HEADER_BYTES), 'Phase6 magic/version/header rejected')
    require(header[3:8] == (1, 0, CAMERA_WORDS, TIMELINE_RECORDS, PACKAGE_BYTES),
            'Phase6 decoder/area/count coverage rejected')
    require(package[32:64] == bytes.fromhex(OWNER_SHA), 'Phase6 executable identity rejected')
    require(package[64:96] == bytes.fromhex(OVERLAY_SHA), 'Phase6 overlay identity rejected')
    require(header[8] == zlib.crc32(package[HEADER_BYTES:]) & 0xffffffff,
            'Phase6 payload CRC rejected')
    camera_token, callback_token, field_800bc81c, reserved = struct.unpack_from('<4I', package, 96)
    require(camera_token == CAMERA_TOKEN and callback_token == CALLBACK_TOKEN,
            'Phase6 source token identity rejected')
    require(field_800bc81c <= 65535 and reserved == 0, 'Phase6 field/reserved domain rejected')
    camera = struct.unpack_from('<231I', package, 112)
    timeline = [struct.unpack_from('<II', package, 1036 + index * 8)
                for index in range(TIMELINE_RECORDS)]
    require(camera[-1] == 0xffffffff and timeline[0][1] == 0x80100738 and
            timeline[-1] == (255, 0), 'Phase6 source table boundary rejected')


def cook(image, st00):
    """Return (neutral_package_bytes, evidence_dict) from exact owner inputs."""
    OwnerExe(image)
    matches = [record for record in iter_resources(Path(st00))
               if record['load'] == OVERLAY_BASE and 'PROGBIN' in record['internal'].upper()]
    require(len(matches) == 1, 'Phase6 requires one supported ST00 program resource')
    selected = matches[0]
    require(selected.get('type', 0) == 0, 'Phase6 program resource type rejected')
    overlay = selected['payload']
    require(sha(overlay) == OVERLAY_SHA, 'Unsupported phase6 overlay identity')
    camera_offset = CAMERA_TOKEN - OVERLAY_BASE
    callback_offset = CALLBACK_TOKEN - OVERLAY_BASE
    config_offset = CONFIG_TOKEN - OVERLAY_BASE
    require(config_offset + 40 <= len(overlay) and
            camera_offset + CAMERA_WORDS * 4 <= len(overlay) and
            callback_offset + TIMELINE_RECORDS * 8 <= len(overlay),
            'Phase6 source table coverage rejected')
    camera = overlay[camera_offset:camera_offset + CAMERA_WORDS * 4]
    timeline = overlay[callback_offset:callback_offset + TIMELINE_RECORDS * 8]
    field_800bc81c = struct.unpack_from('<H', overlay, config_offset + 36)[0]
    payload = struct.pack('<4I', CAMERA_TOKEN, CALLBACK_TOKEN, field_800bc81c, 0)
    payload += camera + timeline
    header = struct.pack('<4sHH6I', b'L3P6', 1, HEADER_BYTES, 1, 0,
                         CAMERA_WORDS, TIMELINE_RECORDS, PACKAGE_BYTES,
                         zlib.crc32(payload) & 0xffffffff)
    package = header + bytes.fromhex(OWNER_SHA) + bytes.fromhex(OVERLAY_SHA) + payload
    validate_package(package)
    evidence = {
        'schema': 'mml-source-event-phase6-cook/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'owner_exe_sha256': OWNER_SHA, 'overlay_sha256': OVERLAY_SHA,
        'supported_stage': 0, 'supported_area': 0,
        'camera_program_token': hex(CAMERA_TOKEN), 'camera_words': CAMERA_WORDS,
        'callback_table_token': hex(CALLBACK_TOKEN), 'timeline_records': TIMELINE_RECORDS,
        'field_owner': {'address': '800BC81C', 'source_record': hex(CONFIG_TOKEN),
                        'record_offset': 36, 'source_encoding': 'unsigned_halfword',
                        'initializer': '80100528..80100564 -> 800371A0'},
        'camera_words_sha256': sha(camera), 'timeline_sha256': sha(timeline),
        'package_bytes': len(package), 'package_sha256': sha(package),
        'payload_crc32': zlib.crc32(payload) & 0xffffffff,
        'original_code_exported': False, 'original_payload_embedded_in_source': False,
        'retail_parity': False, 'hardware_tested': False,
        'limits': [
            'Only exact ST00 area0 data is admitted; other areas require separate source evidence.',
            'The field800BC81C owner is not a FOV or proof of a complete current camera.',
            'Original addresses are identity tokens, never native executable pointers.',
            'Importing camera words and callback identities does not prove every operation or scene completion.',
            'The cooked payload remains owner-local and is excluded from source releases.',
        ],
    }
    return package, evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'st00', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.out.resolve()
    require(not output.exists() and not output.is_relative_to(SOURCE),
            'Choose a NEW directory outside source')
    package, evidence = cook(args.exe.read_bytes(), args.st00)
    output.mkdir(parents=True, exist_ok=False)
    (output / 'source_event_phase6.l3p6').write_bytes(package)
    (output / 'cook-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

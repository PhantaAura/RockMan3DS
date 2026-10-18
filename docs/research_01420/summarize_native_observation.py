#!/usr/bin/env python3
"""Summarize a real native-build CSV snapshot; never infer hardware FPS."""
import argparse
from collections import Counter
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import statistics

SOURCE = Path(__file__).resolve().parents[2]


def fields(row):
    result = {}
    for item in row:
        if '=' not in item:
            continue
        name, value = item.split('=', 1)
        name = name.strip().removeprefix('# ').strip()
        if re.fullmatch(r'-?\d+', value):
            value = int(value)
        elif value in ('true', 'false'):
            value = value == 'true'
        result[name] = value
    return result


def enum_names(path, name):
    text = path.read_text()
    match = re.search(r'enum class\s+' + name + r'\b[^\{]*\{([^}]+)\}', text, re.S)
    if not match:
        raise ValueError('Missing source enum ' + name)
    body = re.sub(r'//[^\n]*', '', match[1])
    result, index = {}, 0
    for item in body.split(','):
        item = item.strip()
        if not item:
            continue
        key = item.split('=', 1)[0].strip()
        if '=' in item:
            index = int(item.split('=', 1)[1].strip(), 0)
        result[index] = key
        index += 1
    return result


def count_named(rows, field, names):
    return dict(Counter(names.get(row[field], 'UnknownCode_' + str(row[field]))
                        for row in rows))


def stats(rows, key):
    values = [row[key] for row in rows]
    return {'samples': len(values), 'minimum': min(values),
            'median': statistics.median(values), 'maximum': max(values)}


def summarize(blob, path):
    rows = list(csv.reader(io.StringIO(blob.decode('utf-8'))))
    headers = [row for row in rows if row and row[0] == 'frame']
    if len(headers) != 1:
        raise ValueError('Expected one native CSV header')
    header, events, samples = headers[0], [], []
    for row in rows:
        if not row:
            continue
        if row[0].startswith('#'):
            event = fields(row)
            tag = row[0].removeprefix('# ').split('=', 1)[0]
            event['record_kind'] = tag
            events.append(event)
        elif row[0].isdigit():
            if len(row) != len(header):
                raise ValueError('Incomplete/malformed native frame sample')
            samples.append({key: int(value) for key, value in zip(header, row)})
    build = next(event for event in events if event['record_kind'] == 'build')
    if build['build'] != '0.0.14.20' or not samples:
        raise ValueError('Unsupported native build/no frame samples')
    contact = enum_names(SOURCE / 'include/legends3ds/player_contact_runtime.h',
                         'RuntimeContactStatus')
    callback = enum_names(SOURCE / 'include/legends3ds/class20_03_behavior.h',
                          'Class20_03CallbackStatus')
    detector = enum_names(SOURCE / 'include/legends3ds/actor_detection.h',
                          'ActorDetectionStatus')
    preparation = enum_names(SOURCE / 'include/legends3ds/actor_detection_preparation.h',
                             'ActorDetectionPreparationStatus')
    actor = [e for e in events if e['record_kind'] == 'actor_state1']
    context = [e for e in events if e['record_kind'] == 'actor_source_context']
    queries = [e for e in events if e['record_kind'] == 'actor_action2_query']
    return {
        'schema': 'mml-native-csv-observation/1', 'status': 'SOURCE_BACKED_PARTIAL',
        'source_csv_path': str(path.resolve()), 'source_csv_sha256': hashlib.sha256(blob).hexdigest(),
        'source_csv_bytes': len(blob), 'csv_rows': len(rows),
        'environment': {'emulator': 'Azahar 2126', 'native_build': build['build'],
                        'reported_old_model': build['new3ds'] == 0,
                        'reported_model_query_rc': build['model_query_rc'],
                        'physical_hardware_tested': False},
        'sdk_reservations': {key: build[key] for key in (
            'memory_layout', 'app_region_bytes', 'heap_reserved', 'linear_reserved', 'stack_reserved')},
        'sampling': {'frame_rows': len(samples), 'unique_gameplay_frames': len(set(s['frame'] for s in samples)),
                     'minimum_gameplay_frame': min(s['frame'] for s in samples),
                     'maximum_gameplay_frame': max(s['frame'] for s in samples),
                     'samples_by_area': dict(Counter(str(s['area']) for s in samples)),
                     'sparse_samples': True, 'fps_computed': False},
        'sampled_time_us': {key: stats(samples, key) for key in (
            'frame_before_log_us', 'interval_us', 'update_us', 'render_us', 'bottom_ui_us')},
        'sampled_memory_bytes': {key: stats(samples, key) for key in (
            'owned_linear', 'owned_texture', 'malloc_live', 'malloc_arena_free', 'linear_free', 'vram_free')},
        'sampled_render_submission': {key: stats(samples, key) for key in ('draw_calls', 'triangles')},
        'sampled_player_contact': {
            'counts': count_named(samples, 'contact_status', contact),
            'grounded_raw_counts': dict(Counter(str(s['grounded']) for s in samples)),
            'by_area': {str(area): count_named([s for s in samples if s['area'] == area],
                                             'contact_status', contact)
                        for area in sorted(set(s['area'] for s in samples))}},
        'actor_sample_evidence': {
            'state_rows': len(actor), 'context_rows': len(context),
            'callback_status_counts': count_named(actor, 'status', callback),
            'detector_status_counts': count_named(context, 'detector_status', detector),
            'preparation_status_counts': count_named(context, 'preparation_status', preparation),
            'source_state_rows': actor, 'source_context_rows': context,
            'full_callback_verified': False, 'retail_parity': False},
        'authored_action2_queries': {
            'rows': len(queries),
            'input_ready': sum(e.get('input_ready') == 1 for e in queries),
            'query_status_counts': count_named(queries, 'status', contact),
            'wall_status_counts': count_named(queries, 'wall_status', contact),
            'floor_status_counts': count_named(queries, 'floor_status', contact),
            'active_actor_modified': any(e.get('active_actor_modified') is True for e in queries),
            'actual_enemy_AI_admission': False},
        'area_load_transition_and_spawn_events': [e for e in events if e['record_kind'] == 'area_event'],
        'imported_actor_events': [e for e in events if e['record_kind'] == 'actor_event'],
        'gameplay_clock_events': [e for e in events if e['record_kind'] == 'gameplay_clock'],
        'event_record_counts': dict(Counter(e['record_kind'] for e in events)),
        'PHYSICS_1TO1': False, 'retail_parity': False,
        'limits': [
            'Emulator measurements do not establish physical Old 3DS performance.',
            'Sparse rows and repeated gameplay frame counters do not establish FPS or worst-case pacing.',
            'frame_before_log_us excludes its log write; interval includes previous logging.',
            'Render timing includes synchronization; loading and host scheduling affect samples.',
            'SDK reservations are not live usage; reported owned memory pools must not be summed blindly.',
            'Action2 area-load queries use authored admission on copies; they are not active AI encounters.',
            'Detector Ready only admits that source subset; missing callback dependencies remain explicit.',
            'An observed transition does not prove retail events or normal tutorial progression.',
        ],
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--csv', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--snapshot', type=Path)
    a = p.parse_args()
    if a.out.exists() or (a.snapshot and a.snapshot.exists()):
        raise SystemExit('Refusing to overwrite observation outputs')
    blob = a.csv.read_bytes()
    if not blob.endswith(b'\n') or len(blob) > 16 * 1024 * 1024:
        raise SystemExit('Incomplete CSV or fixed byte budget exceeded')
    result = summarize(blob, a.csv)
    if a.snapshot:
        a.snapshot.parent.mkdir(parents=True, exist_ok=True)
        a.snapshot.write_bytes(blob)
        result['frozen_csv_snapshot_path'] = str(a.snapshot.resolve())
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'output': str(a.out), 'sampled_rows': result['sampling']['frame_rows'],
                      'actor_statuses': result['actor_sample_evidence']['callback_status_counts'],
                      'detector_preparation': result['actor_sample_evidence']['preparation_status_counts'],
                      'physical_hardware_tested': False}))


if __name__ == '__main__':
    main()

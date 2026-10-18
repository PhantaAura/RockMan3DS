#!/usr/bin/env python3
"""Exact field comparison and provenance-checked gameplay measurement comparison.

``compare`` keeps the inherited unlabeled native determinism API. The CLI requires
the labeled format by default; bare native replay arrays never establish retail
parity. Unknown retail fields are reported as unavailable, not as matching zeros.
"""
import argparse
import hashlib
import json
import math
import re
from pathlib import Path

FORMAT = 'mml-gameplay-trace/1'
SOURCES = {'retail_reference', 'native_replay', 'synthetic_test'}
INPUT_FLAGS = ('fire_pressed', 'lock_held', 'advance_dialogue', 'lock_pressed',
               'pause_pressed', 'skip_held')
PROFILE_KEYS = ('game_profile', 'scenario', 'initial_state_id', 'timing',
                'coordinate_profile', 'control_profile', 'input_sha256')
MISSING = object()


class TraceProfileError(ValueError):
    """A malformed or incomparable measurement; not a gameplay divergence."""


def load_json(path):
    def bad_constant(value):
        raise TraceProfileError('non-finite JSON value: ' + value)
    try:
        return json.loads(Path(path).read_text(), parse_constant=bad_constant)
    except (json.JSONDecodeError, OSError) as error:
        raise TraceProfileError(str(error)) from error


def normalize_input(value):
    """Convert the inherited l3replay four-item input without inventing pad bits."""
    if isinstance(value, list) and len(value) == 4:
        if any(type(v) is not int for v in value):
            raise TraceProfileError('legacy input must contain four integers')
        value = dict(zip(('held_psx_mask',) + INPUT_FLAGS[:3], value))
    if not isinstance(value, dict) or set(value) - {'held_psx_mask', *INPUT_FLAGS}:
        raise TraceProfileError('unknown or malformed input fields')
    held = value.get('held_psx_mask')
    if type(held) is not int or not 0 <= held <= 65535:
        raise TraceProfileError('held_psx_mask must be a 16-bit positive held mask')
    result = {'held_psx_mask': held}
    for flag in INPUT_FLAGS:
        item = value.get(flag, False)
        if type(item) is bool:
            result[flag] = item
        elif type(item) is int and item in (0, 1):
            result[flag] = bool(item)
        else:
            raise TraceProfileError(flag + ' must be a boolean or 0/1')
    return result


def input_sha256(frames):
    rows = [[row['frame'], *normalize_input(row['input']).values()] for row in frames]
    return hashlib.sha256(json.dumps(rows, separators=(',', ':'), allow_nan=False).encode()).hexdigest()


def _sha(value, label):
    if not isinstance(value, str) or re.fullmatch('[0-9a-f]{64}', value) is None:
        raise TraceProfileError(label + ' must be a lower-case SHA-256')


def _text(value, label):
    if not isinstance(value, str) or not value.strip():
        raise TraceProfileError(label + ' must be a nonempty string')


def _finite(value, label):
    if isinstance(value, dict):
        for key, item in value.items():
            if not isinstance(key, str):
                raise TraceProfileError(label + ' has a non-string key')
            _finite(item, label + '.' + key)
    elif isinstance(value, list):
        for i, item in enumerate(value):
            _finite(item, label + '[' + str(i) + ']')
    elif type(value) is float and not math.isfinite(value):
        raise TraceProfileError(label + ' contains a non-finite number')
    elif value is not None and type(value) not in (int, float, bool, str):
        raise TraceProfileError(label + ' contains an unsupported value')


def validate_trace(trace):
    if not isinstance(trace, dict) or trace.get('format') != FORMAT:
        raise TraceProfileError('expected ' + FORMAT + '; unlabeled arrays are not retail references')
    meta = trace.get('metadata')
    if not isinstance(meta, dict) or meta.get('source_kind') not in SOURCES:
        raise TraceProfileError('source_kind must identify retail_reference, native_replay, or synthetic_test')
    game = meta.get('game_profile')
    if not isinstance(game, dict):
        raise TraceProfileError('missing exact game_profile')
    _text(game.get('id'), 'game_profile.id')
    _sha(game.get('sha256'), 'game_profile.sha256')
    producer = meta.get('producer')
    if not isinstance(producer, dict):
        raise TraceProfileError('missing producer provenance')
    for field in ('name', 'version'):
        _text(producer.get(field), 'producer.' + field)
    for field in ('artifact_sha256', 'settings_sha256'):
        _sha(producer.get(field), 'producer.' + field)
    for field in ('scenario', 'initial_state_id', 'coordinate_profile', 'control_profile'):
        _text(meta.get(field), field)
    timing = meta.get('timing')
    if not isinstance(timing, dict):
        raise TraceProfileError('missing timing profile')
    for field in ('clock', 'sample_phase'):
        _text(timing.get(field), 'timing.' + field)
    for field in ('rate_num', 'rate_den'):
        if type(timing.get(field)) is not int or not 0 < timing[field] <= 1000000:
            raise TraceProfileError('timing.' + field + ' must be a positive bounded integer')
    fields = meta.get('capture_fields')
    if not isinstance(fields, list) or not fields or any(not isinstance(field, str) for field in fields) or len(fields) != len(set(fields)):
        raise TraceProfileError('capture_fields must be a nonempty unique list')
    units = meta.get('field_units')
    if not isinstance(units, dict):
        raise TraceProfileError('missing field_units')
    for field in fields:
        _text(field, 'capture_fields entry')
        if field in ('frame', 'input'):
            raise TraceProfileError('frame/input are compulsory, not capture_fields')
        _text(units.get(field), 'field_units.' + field)
    frames = trace.get('frames')
    if not isinstance(frames, list) or not frames:
        raise TraceProfileError('frames must be a nonempty list')
    for i, row in enumerate(frames):
        if not isinstance(row, dict) or type(row.get('frame')) is not int or row['frame'] != i:
            raise TraceProfileError('frames must be contiguous integers beginning at zero')
        normalized = normalize_input(row.get('input'))
        if (row.get('input') != normalized or set(row['input']) != set(normalized) or
                any(type(row['input'][flag]) is not bool for flag in INPUT_FLAGS)):
            raise TraceProfileError('trace input must contain the full normalized boolean input object')
        for field in fields:
            if field not in row:
                raise TraceProfileError('frame ' + str(i) + ' lacks declared field ' + field)
        _finite(row, 'frame ' + str(i))
    _sha(meta.get('input_sha256'), 'input_sha256')
    if meta['input_sha256'] != input_sha256(frames):
        raise TraceProfileError('input_sha256 does not match recorded inputs')
    return trace


def compare(a,b):
    first=None;errors={};mismatches={};missing=object()
    def walk(av,bv,path,frame):
        nonlocal first
        if isinstance(av,dict) and isinstance(bv,dict):
            for k in sorted(set(av)|set(bv)):
                walk(av.get(k,missing),bv.get(k,missing),path+'.'+k if path else k,frame)
            return
        if isinstance(av,list) and isinstance(bv,list):
            for i in range(max(len(av),len(bv))):
                walk(av[i] if i<len(av) else missing,bv[i] if i<len(bv) else missing,f'{path}[{i}]',frame)
            return
        numeric=type(av) in (int,float) and type(bv) in (int,float)
        finite=numeric and math.isfinite(av) and math.isfinite(bv)
        different=av!=bv or (numeric and not finite) or (type(av)!=type(bv) and not numeric)
        if finite:
            delta=bv-av
            if path=='heading':delta=(delta+2048)%4096-2048
            e=abs(delta)
            bucket=errors.setdefault(path,{'max_abs_error':0,'sum_abs_error':0,'samples':0})
            bucket['max_abs_error']=max(bucket['max_abs_error'],e);bucket['sum_abs_error']+=e;bucket['samples']+=1
        if different:
            mismatches[path]=mismatches.get(path,0)+1
            if first is None:
                clean=lambda x:'<missing>' if x is missing else str(x) if isinstance(x,float) and not math.isfinite(x) else x
                first={'frame':frame,'field':path,'reference':clean(av),'candidate':clean(bv)}
                if finite:first['delta']=delta
    for i in range(max(len(a),len(b))):
        if i>=len(a) or i>=len(b):walk(len(a),len(b),'trace_length',i);break
        walk(a[i],b[i],'',a[i].get('frame',i))
    count=sum(e['samples'] for e in errors.values());total=sum(e['sum_abs_error'] for e in errors.values())
    for e in errors.values():e['mean_abs_error']=e.pop('sum_abs_error')/e['samples']
    return {'exact':first is None,'first_divergence':first,'field_errors':errors,'mismatch_counts':mismatches,
            'comparison_kind':'unlabeled_native_determinism','retail_comparison':False,'PHYSICS_1TO1':False,
            'max_abs_error':max((e['max_abs_error'] for e in errors.values()),default=0),
            'mean_abs_error':total/count if count else 0}


def compare_traces(reference, candidate):
    """Compare only observed reference fields after exact profile validation."""
    validate_trace(reference)
    validate_trace(candidate)
    rm, cm = reference['metadata'], candidate['metadata']
    for key in PROFILE_KEYS:
        if rm.get(key) != cm.get(key):
            raise TraceProfileError('incomparable profile: ' + key)
    for field in rm['capture_fields']:
        if cm['field_units'].get(field) != rm['field_units'][field]:
            raise TraceProfileError('incomparable field units: ' + field)
    retail = rm['source_kind'] == 'retail_reference' and cm['source_kind'] == 'native_replay'
    if rm['source_kind'] == 'native_replay' and cm['source_kind'] == 'retail_reference':
        raise TraceProfileError('retail_reference must be the reference, not the candidate')
    if rm['source_kind'] == 'retail_reference' and not retail:
        raise TraceProfileError('retail measurement comparison requires a native_replay candidate')
    first = None
    errors, coverage, mismatch = {}, {}, {}

    def walk(av, bv, path, frame):
        nonlocal first
        if isinstance(av, dict):
            if not av:
                walk('<empty object>', '<empty object>' if bv == {} else bv, path, frame)
            for key in sorted(av):
                item = bv.get(key, MISSING) if isinstance(bv, dict) else MISSING
                walk(av[key], item, path + '.' + key, frame)
            return
        if isinstance(av, list):
            if not av:
                walk('<empty list>', '<empty list>' if bv == [] else bv, path, frame)
            for i in range(max(len(av), len(bv) if isinstance(bv, list) else 0)):
                walk(av[i] if i < len(av) else MISSING,
                     bv[i] if isinstance(bv, list) and i < len(bv) else MISSING,
                     path + '[' + str(i) + ']', frame)
            return
        bucket = coverage.setdefault(path, {'observed_samples': 0, 'unavailable_reference_samples': 0,
                                            'unavailable_candidate_samples': 0, 'mismatch_samples': 0})
        if av is None:
            bucket['unavailable_reference_samples'] += 1
            return
        bucket['observed_samples'] += 1
        if bv is MISSING or bv is None:
            bucket['unavailable_candidate_samples'] += 1
        numeric = type(av) in (int, float) and type(bv) in (int, float)
        delta = None
        different = type(av) != type(bv) and not numeric or av != bv
        if numeric:
            delta = bv - av
            if path == 'heading':
                # A turn has 4096 units only when both field profiles say so.
                if rm['field_units'].get('heading') == 'turn_4096':
                    delta = (delta + 2048) % 4096 - 2048
                    different = delta != 0
            error = abs(delta)
            stats = errors.setdefault(path, {'max_abs_error': 0, 'sum_abs_error': 0, 'samples': 0})
            stats['max_abs_error'] = max(stats['max_abs_error'], error)
            stats['sum_abs_error'] += error
            stats['samples'] += 1
        if different:
            mismatch[path] = mismatch.get(path, 0) + 1
            bucket['mismatch_samples'] += 1
            if first is None:
                first = {'frame': frame, 'field': path,
                         'reference': '<missing>' if av is MISSING else av,
                         'candidate': '<missing>' if bv is MISSING else bv}
                if delta is not None:
                    first['delta'] = delta

    af, bf = reference['frames'], candidate['frames']
    for i in range(max(len(af), len(bf))):
        if i >= len(af) or i >= len(bf):
            walk(len(af), len(bf), 'trace_length', i)
            break
        for field in rm['capture_fields']:
            walk(af[i][field], bf[i].get(field, MISSING), field, i)
    for stats in errors.values():
        stats['mean_abs_error'] = stats.pop('sum_abs_error') / stats['samples']
    for stats in coverage.values():
        stats['status'] = ('UNKNOWN' if not stats['observed_samples'] else
                           'CANDIDATE' if stats['mismatch_samples'] or not retail else 'MATCHES_MEASURED_FIELDS')
    observed = sum(stats['observed_samples'] for stats in coverage.values())
    return {'format': 'mml-trace-comparison/1',
            'comparison_kind': 'retail_measurement' if retail else 'native_determinism' if
             rm['source_kind'] == cm['source_kind'] == 'native_replay' else 'synthetic_test',
            'retail_comparison': retail, 'PHYSICS_1TO1': False,
            'status': 'UNKNOWN' if not observed else 'MATCHES_MEASURED_FIELDS' if retail and first is None else 'CANDIDATE',
            'exact': first is None, 'observed_scalar_samples': observed,
            'first_divergence': first, 'field_errors': errors, 'field_coverage': coverage,
            'mismatch_counts': mismatch, 'reference_frames': len(af), 'candidate_frames': len(bf),
            'limitations': ['Matching captured fields does not establish whole-game or whole-physics 1:1 parity.',
                            'Unknown/null reference fields are excluded from matching claims.',
                            'Metadata records provenance; the importer does not authenticate an emulator run.']}


def main():
    p=argparse.ArgumentParser();p.add_argument('reference');p.add_argument('candidate');p.add_argument('--output')
    p.add_argument('--legacy-native', action='store_true', help='compare unlabeled native arrays as determinism only')
    a=p.parse_args()
    try:
        reference, candidate = load_json(a.reference), load_json(a.candidate)
        if a.legacy_native:
            if not isinstance(reference, list) or not isinstance(candidate, list):
                raise TraceProfileError('--legacy-native accepts only legacy arrays')
            r = compare(reference, candidate)
        else:
            r = compare_traces(reference, candidate)
    except TraceProfileError as error:
        p.exit(2, 'trace profile rejected: ' + str(error) + '\n')
    s=json.dumps(r,indent=2,allow_nan=False)+'\n'
    if a.output:Path(a.output).write_text(s)
    else:print(s,end='')
    return 0 if r['exact'] else 1
if __name__=='__main__':raise SystemExit(main())

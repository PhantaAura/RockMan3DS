#!/usr/bin/env python3
"""Cook a small PRIVATE selected-OpenBIOS candidate profile from observed scalars.

The full supplied snapshot is audited. An explicit completed-prefix option may
exclude only a final partial JSONL line, final observer failure and/or the exact
final unfinished call. It never invents a seed, return or chronology binding.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import zlib

SOURCE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('rng_reference_validator', SOURCE / 'docs/research_01420/validate_rng_reference_trace.py')
reference = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reference)
BIOS_SHA, EXE_SHA, OVERLAY_SHA = reference.BIOS_SHA, reference.EXE_SHA, reference.OVERLAY_SHA
MAX_BYTES, MAX_ROWS = 16 * 1024 * 1024, 65536
UNMODELED, NOT_RETAIL, INCOMPLETE, FAILED, PENDING, PARTIAL = 1, 2, 4, 8, 16, 32
RNG_FIELDS = {
    'observer_start': 'kind bios_sha256 source_exe_sha256 overlay_sha256 scope seed32 max_rows game_boot_parity retail_parity',
    'game_entry': 'kind vsync game_entry_count rand_ordinal seed32 source_identity_matches',
    'bios_rng_enter': 'kind vsync rand_ordinal game_entry_count service caller_ra seed_before32 a0_argument',
    'bios_rng_return': 'kind vsync rand_ordinal service caller_ra seed_before32 seed_after32 v0_actual',
    'class03_overlay_admitted': 'kind vsync rand_ordinal overlay_identity_matches source_overlay_sha256',
    'class03_overlay_not_admitted': 'kind vsync rand_ordinal skipped_visits overlay_identity_matches actor_fields_read',
    'class03_sample_store': 'kind vsync rand_ordinal slot work state08 action09 phase0A stats_index0C context_high0F position14 heading56 v0_actual sample32A_to_store overlay_identity_matches',
    'observer_budget_complete': 'kind rand_ordinal rows_before pending_return',
}


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def unique_object(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise ValueError('Duplicate JSON field: ' + key)
        out[key] = value
    return out


def decode_snapshot(blob, allow_prefix, max_bytes=MAX_BYTES):
    if not isinstance(blob, bytes) or not blob or len(blob) > max_bytes:
        raise ValueError('Trace exceeds fixed byte budget or is empty')
    lines = blob.splitlines(keepends=True)
    if len(lines) > MAX_ROWS:
        raise ValueError('Trace exceeds fixed row budget')
    partial = bool(lines and not lines[-1].endswith(b'\n'))
    if partial:
        if not allow_prefix:
            raise ValueError('Incomplete JSONL tail requires explicit --capture-prefix')
        lines.pop()
    rows = []
    for line in lines:
        if not line.strip() or len(line) > 16384:
            raise ValueError('Empty or oversized JSONL row')
        row = json.loads(line.decode('utf-8'), object_pairs_hook=unique_object,
                         parse_constant=lambda _: (_ for _ in ()).throw(ValueError('Nonfinite JSON value')))
        if not isinstance(row, dict):
            raise ValueError('Trace row is not an object')
        rows.append(row)
    if not rows:
        raise ValueError('Missing complete observer start')
    return rows, lines, partial


def strict_context(rows):
    start = rows[0]
    for row in rows:
        fields = RNG_FIELDS.get(row.get('kind'))
        if fields is None or set(row) != set(fields.split()):
            raise ValueError('Unsupported/missing observed RNG row fields')
    maximum = reference.u32(start, 'max_rows')
    if not 16 <= maximum <= MAX_ROWS or len(rows) > maximum:
        raise ValueError('Invalid/exceeded measured observer row budget')
    if start.get('scope') != 'CURRENT_INSTALLATION' or start.get('game_boot_parity') is not False:
        raise ValueError('Unsupported observer installation/parity scope')
    entries = [(i, row) for i, row in enumerate(rows) if row.get('kind') == 'game_entry']
    if len(entries) != 1 or entries[0][0] != 1:
        raise ValueError('Exactly one observed game entry before consumers is required')
    origin = entries[0][1]
    if reference.u32(origin, 'rand_ordinal') != 0 or reference.u32(origin, 'game_entry_count') != 1:
        raise ValueError('Only the observed game-entry ordinal-zero boundary is supported')
    if reference.u32(start, 'seed32') != reference.u32(origin, 'seed32'):
        raise ValueError('Unobserved state change before game entry')
    previous_vsync = 0
    for row in rows[1:]:
        kind = row.get('kind')
        if kind == 'observer_budget_complete':
            continue
        vsync = reference.u32(row, 'vsync')
        if vsync < previous_vsync:
            raise ValueError('Reference vsync moves backwards')
        previous_vsync = vsync
        if row.get('retail_parity', False) is not False:
            raise ValueError('Unsupported retail parity claim')
        if kind == 'bios_rng_enter':
            if reference.u32(row, 'game_entry_count') != 1:
                raise ValueError('Random call has another/unobserved game-entry origin')
            reference.u32(row, 'a0_argument')
        if kind in ('bios_rng_enter', 'bios_rng_return'):
            caller = reference.u32(row, 'caller_ra')
            if caller & 3 or not 0x80000000 <= caller < 0x80200000:
                raise ValueError('Random caller outside aligned original RAM return domain')
        if kind == 'class03_overlay_not_admitted' and reference.u32(row, 'skipped_visits') == 0:
            raise ValueError('Overlay skip has no observed visit')
    return origin


def bounded(row, name, low, high):
    value = row.get(name)
    if not isinstance(value, int) or isinstance(value, bool) or not low <= value <= high:
        raise ValueError('Input scalar outside observed domain: ' + name)
    return value


def vector(row, name, low, high):
    value = row.get(name)
    if not isinstance(value, list) or len(value) != 3 or any(
            not isinstance(v, int) or isinstance(v, bool) or not low <= v <= high for v in value):
        raise ValueError('Input vector outside observed domain: ' + name)


def input_fields(row):
    for name in ('player_flags00', 'player_marker01', 'player_state08', 'player_action09', 'player_phase0A',
                 'stage_byte800C1B60', 'transition_byte800C356C', 'event_active00', 'event_id01', 'event_phase04'):
        bounded(row, name, 0, 255)
    for name in ('pad_held_buttons', 'player_heading56', 'player_held11C'):
        bounded(row, name, 0, 65535)
    for name in ('player_hp6E', 'busy_halfword80098A72'):
        bounded(row, name, -32768, 32767)
    if reference.u32(row, 'player_work') != 0x800b51b0:
        raise ValueError('Input player work identity differs')
    vector(row, 'player_position14', -32768, 32767)
    vector(row, 'player_velocity44', -32768, 32767)
    vector(row, 'player_words20', -2147483648, 2147483647)
    actors = row.get('admitted_actor_slots')
    if not isinstance(actors, list) or len(actors) > 16:
        raise ValueError('Input actor pool exceeds bounded source capacity')
    last_slot = -1
    for actor in actors:
        if not isinstance(actor, dict):
            raise ValueError('Input actor context is not an object')
        slot = bounded(actor, 'slot', 0, 15)
        if slot <= last_slot or reference.u32(actor, 'work') != 0x8009f108 + slot * 0x3c0:
            raise ValueError('Input actor pool identity/order differs')
        last_slot = slot
        for name in ('flags00', 'flags06', 'marker01', 'subtype02', 'state08', 'action09', 'phase0A'):
            bounded(actor, name, 0, 255)
        if not actor['flags00'] & 1 or not actor['flags06'] & 1:
            raise ValueError('Input actor was not originally admitted')
        vector(actor, 'position14', -32768, 32767)
        bounded(actor, 'heading56', 0, 65535)
    flags = row.get('story_flag_ids')
    if not isinstance(flags, list) or len(flags) > 2048 or any(
            not isinstance(v, int) or isinstance(v, bool) or not 0 <= v < 2048 for v in flags) or \
            any(a >= b for a, b in zip(flags, flags[1:])):
        raise ValueError('Input story flags outside observed sorted bit domain')


def audit_input(blob, allow_prefix):
    if blob is None:
        return {'provided': False, 'complete_native_chronology_matches': False}
    rows, lines, partial = decode_snapshot(blob, allow_prefix, 64 * 1024 * 1024)
    start = rows[0]
    if start.get('kind') != 'input_observer_start' or start.get('source_exe_sha256') != EXE_SHA or \
            start.get('scope') != 'CURRENT_INSTALLATION' or start.get('pad_override') is not False or \
            start.get('retail_parity') is not False:
        raise ValueError('Unsupported input observer origin')
    last_vsync = 0
    for ordinal, row in enumerate(rows[1:], 1):
        if row.get('kind') != 'source_update_input' or reference.u32(row, 'update_ordinal') != ordinal or \
                row.get('dispatcher_identity_matches') is not True or \
                row.get('complete_native_chronology_matches') is not False:
            raise ValueError('Unsupported input row or invented native chronology binding')
        vsync = reference.u32(row, 'reference_vsync')
        if vsync < last_vsync:
            raise ValueError('Input reference vsync moves backwards')
        last_vsync = vsync
        if not isinstance(row.get('rng_pending_return'), bool):
            raise ValueError('Input reference has no explicit RNG pending state')
        reference.u32(row, 'rng_rand_ordinal')
        reference.u32(row, 'rng_reference_vsync')
        input_fields(row)
    return {'provided': True, 'snapshot_sha256': sha(blob), 'snapshot_bytes': len(blob),
            'complete_rows_audited': len(rows), 'partial_tail_excluded': partial,
            'complete_prefix_sha256': sha(b''.join(lines)),
            'complete_native_chronology_matches': False,
            'limits': ['Input rows are audit provenance only; no replay or callback binding is inferred.']}


def cook(blob, capture_prefix=False, input_blob=None):
    rows, lines, partial = decode_snapshot(blob, capture_prefix)
    original_rows = len(rows)
    warnings = UNMODELED | NOT_RETAIL | (PARTIAL if partial else 0)
    failure = None
    failure_indices = [i for i, row in enumerate(rows) if row.get('kind') == 'observer_guard_failure']
    if failure_indices:
        if not capture_prefix or failure_indices != [len(rows) - 1] or partial:
            raise ValueError('Only one exact final observer failure is eligible for explicit prefix analysis')
        failure = rows[-1]
        if set(failure) != {'kind', 'reason'} or not isinstance(failure.get('reason'), str) or \
                not failure['reason'] or len(failure['reason']) > 96:
            raise ValueError('Malformed final observer failure')
        rows.pop(); lines.pop(); warnings |= FAILED
    # Validate ALL complete rows before any unfinished call exclusion. Unknown
    # kinds, wrong returns, interior failures and malformed pending identities
    # are rejected here, rather than hidden by a search for a useful prefix.
    proof = reference.validate(rows)
    origin = strict_context(rows)
    if original_rows > reference.u32(rows[0], 'max_rows'):
        raise ValueError('Complete snapshot exceeds observed row budget')
    if proof['unobserved_state_changes'] or proof['actual_reseed_returns']:
        raise ValueError('State gaps or reseeds are outside this continuous profile contract')
    if proof['pending_return_at_end']:
        if not capture_prefix or rows[-1].get('kind') != 'bios_rng_enter':
            raise ValueError('Unfinished original random call requires explicit final-call prefix exclusion')
        rows.pop(); lines.pop(); warnings |= PENDING
        proof = reference.validate(rows)
        strict_context(rows)
    whole = bool(proof['bounded_observer_completed'] and not partial and failure is None)
    if not whole and not capture_prefix:
        raise ValueError('Uncompleted capture requires explicit --capture-prefix')
    if proof['actual_rand_returns'] == 0 or not proof['continuous_since_installation']:
        raise ValueError('At least one continuous completed original return is required')
    if whole and (warnings != UNMODELED | NOT_RETAIL):
        raise ValueError('Completed observer boundary cannot hide excluded tail rows')
    if not whole:
        warnings |= INCOMPLETE
    prefix = b''.join(lines)
    returns = [r for r in rows if r.get('kind') == 'bios_rng_return' and r.get('service') == 0x2f]
    callers = sorted({r['caller_ra'] for r in returns})
    values = [1 if whole else 2, warnings, origin['seed32'], origin['rand_ordinal'],
              origin['vsync'], origin['game_entry_count'], len(returns), 0, original_rows,
              len(rows), original_rows-len(rows), len(blob), len(prefix),
              returns[-1]['rand_ordinal'], len(callers), 0]
    payload = b''.join(bytes.fromhex(d) for d in (BIOS_SHA, EXE_SHA, OVERLAY_SHA, sha(blob), sha(prefix))) + struct.pack('<16I', *values)
    header = struct.pack('<4sHH6I', b'L3RG', 1, 32, 1, 256, len(payload), zlib.crc32(payload) & 0xffffffff, 0, 0)
    package = header + payload
    assert len(package) == 256
    evidence = {
        'schema': 'mml-source-rng-candidate-profile-cook/1', 'status': 'CANDIDATE',
        'algorithm_evidence': 'SOURCE_BACKED_SELECTED_OPENBIOS', 'consumer_chronology': 'CANDIDATE_UNMODELED_CONSUMERS',
        'selected_bios_sha256': BIOS_SHA, 'source_exe_sha256': EXE_SHA, 'source_overlay_sha256': OVERLAY_SHA,
        'origin': dict(origin), 'origin_seed_invented': False,
        'scope': 'WHOLE_BOUNDED_CAPTURE' if whole else 'COMPLETED_CAPTURE_PREFIX',
        'whole_trace_accepted': whole, 'capture_prefix_explicit': capture_prefix,
        'snapshot_sha256': sha(blob), 'snapshot_bytes': len(blob),
        'validated_prefix_sha256': sha(prefix), 'validated_prefix_bytes': len(prefix),
        'complete_snapshot_rows': original_rows, 'validated_prefix_rows': len(rows),
        'excluded_complete_rows': original_rows-len(rows), 'partial_tail_excluded': partial,
        'terminating_observer_failure': failure,
        'unfinished_final_call_excluded': bool(warnings & PENDING),
        'actual_rand_returns': len(returns), 'actual_class03_samples': proof['actual_class03_samples'],
        'observed_original_caller_counts': {hex(c): sum(r['caller_ra'] == c for r in returns) for c in callers},
        'warnings_mask': warnings, 'input_reference_audit': audit_input(input_blob, capture_prefix),
        'output': {'name': 'source_rng.l3rg', 'bytes': len(package), 'sha256': sha(package), 'private_local_only': True},
        'retail_bios_profile_proved': False, 'shared_native_consumer_lifecycle_proved': False, 'retail_parity': False,
        'limits': [
            'Only the supplied selected OpenBIOS algorithm and observed game-entry origin are admitted.',
            'Candidate native freeplay omits competing original consumers and reseeding; generated actor samples are not measured actor samples.',
            'No input/update/callback chronology matching or retail BIOS parity is inferred.',
            'The strict RetailRngTape remains a separate measured-context replay gate.',
            'No BIOS image, instruction bytes, retail assets or random-return tape are serialized.',
            'The sidecar and owner-derived origin metadata stay outside source/release archives.'],
    }
    return package, evidence


def verify_input(path, expected, label):
    # Hash only; no BIOS/retail content is copied to outputs.
    if sha(path.read_bytes()) != expected:
        raise ValueError('Unsupported supplied ' + label + ' SHA-256')


def verify_st00(path):
    with path.open('rb') as file:
        header = file.read(2048)
        if len(header) != 2048 or struct.unpack_from('<I', header)[0] != 0 or \
                struct.unpack_from('<I', header, 4)[0] != 141056 or \
                struct.unpack_from('<I', header, 12)[0] != 0x80100000:
            raise ValueError('Unsupported supplied tutorial archive header')
        overlay = file.read(141056)
    if len(overlay) != 141056 or sha(overlay) != OVERLAY_SHA:
        raise ValueError('Unsupported supplied tutorial overlay SHA-256')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trace', required=True, type=Path)
    parser.add_argument('--input-trace', type=Path)
    parser.add_argument('--bios', required=True, type=Path)
    parser.add_argument('--exe', required=True, type=Path)
    overlay_group = parser.add_mutually_exclusive_group(required=True)
    overlay_group.add_argument('--st00', type=Path, help='Already supplied tutorial archive; hash selected overlay in memory')
    overlay_group.add_argument('--overlay', type=Path, help='Already supplied standalone selected overlay, if available')
    parser.add_argument('--out', required=True, type=Path, help='NEW private local directory outside source')
    parser.add_argument('--capture-prefix', action='store_true', help='Explicitly audit a completed prefix of an unfinished/failed snapshot')
    args = parser.parse_args()
    output = args.out.resolve()
    inputs = [args.trace, args.input_trace, args.bios, args.exe, args.overlay, args.st00]
    if output.exists() or output.is_relative_to(SOURCE) or any(p and (output == p.resolve() or p.resolve().is_relative_to(output)) for p in inputs):
        raise ValueError('Choose a NEW private local output directory outside source and inputs')
    for path, expected, label in ((args.bios, BIOS_SHA, 'BIOS'), (args.exe, EXE_SHA, 'EXE')):
        verify_input(path, expected, label)
    if args.st00:
        verify_st00(args.st00)
    else:
        verify_input(args.overlay, OVERLAY_SHA, 'overlay')
    if args.trace.stat().st_size > MAX_BYTES or args.input_trace and args.input_trace.stat().st_size > 64 * 1024 * 1024:
        raise ValueError('Trace exceeds fixed byte budget')
    package, evidence = cook(args.trace.read_bytes(), args.capture_prefix, args.input_trace.read_bytes() if args.input_trace else None)
    output.mkdir(parents=True, exist_ok=False)
    with (output / 'source_rng.l3rg').open('xb') as file:
        file.write(package)
    evidence['output_directory'] = str(output)
    with (output / 'cook-evidence.json').open('x') as file:
        file.write(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()

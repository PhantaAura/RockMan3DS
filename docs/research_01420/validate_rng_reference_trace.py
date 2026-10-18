#!/usr/bin/env python3
"""Validate bounded measured Redux RNG scalars without filling missing samples."""
import argparse
import hashlib
import json
from pathlib import Path

BIOS_SHA = '713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
# Original JAL801022F0 returns to801022F8. The later SH store breakpoint is
#801022FC. They are separate execution addresses, not interchangeable callers.
CLASS03_RAND_CALLER_RA = 0x801022F8
CLASS03_SAMPLE_STORE_PC = 0x801022FC


def u32(row, name):
    value = row.get(name)
    if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= 0xFFFFFFFF:
        raise ValueError('Invalid unsigned scalar ' + name)
    return value


def class03_context(row):
    """Validate captured pre-store context before a later tape may borrow it.

    Exact context fields do not establish input/frame/update chronology. No
    field is inferred from slot, position similarity or an animation number.
    """
    for name in ('state08', 'action09', 'phase0A', 'stats_index0C', 'context_high0F'):
        if u32(row, name) > 255:
            raise ValueError('Class03 byte context is outside domain: ' + name)
    if u32(row, 'heading56') > 65535:
        raise ValueError('Class03 unsigned heading is outside domain')
    position = row.get('position14')
    if not isinstance(position, list) or len(position) != 3 or any(
            not isinstance(value, int) or isinstance(value, bool) or
            not -32768 <= value <= 32767 for value in position):
        raise ValueError('Class03 raw position is outside three-short domain')


def validate(rows):
    if not rows or rows[0].get('kind') != 'observer_start':
        raise ValueError('Missing measured observer start')
    start = rows[0]
    if (start.get('bios_sha256'), start.get('source_exe_sha256'),
            start.get('overlay_sha256')) != (BIOS_SHA, EXE_SHA, OVERLAY_SHA):
        raise ValueError('Unsupported source/BIOS identities')
    if start.get('retail_parity') is not False:
        raise ValueError('Unsupported parity claim')
    previous = u32(start, 'seed32')
    pending, returns, samples, entries, gaps = None, {}, [], [], []
    admitted, skipped = False, 0
    sample_ordinals = set()
    ordinal, reseeds = 0, 0
    budget_complete = False
    for line, row in enumerate(rows[1:], 2):
        kind = row.get('kind')
        if kind == 'observer_budget_complete':
            if (line != len(rows) or pending is not None or budget_complete or
                    row.get('pending_return') is not False or
                    u32(row, 'rand_ordinal') != ordinal or
                    u32(row, 'rows_before') != line - 1):
                raise ValueError('Invalid completed observer budget boundary')
            budget_complete = True
            continue
        elif kind == 'observer_guard_failure':
            raise ValueError('Observer guard failure at line ' + str(line))
        if kind == 'game_entry':
            if row.get('source_identity_matches') is not True:
                raise ValueError('Game entry identity is not verified')
            entries.append(row)
            if previous != u32(row, 'seed32'):
                gaps.append({'line': line, 'kind': 'state_change_before_game_entry'})
            previous = u32(row, 'seed32')
        elif kind == 'bios_rng_enter':
            if pending is not None:
                raise ValueError('Nested or missing random return at line ' + str(line))
            service = u32(row, 'service')
            if service not in (0x2F, 0x30):
                raise ValueError('Unknown random service')
            ordinal += service == 0x2F
            if u32(row, 'rand_ordinal') != ordinal:
                raise ValueError('Random ordinal is discontinuous')
            before = u32(row, 'seed_before32')
            if previous != before:
                gaps.append({'line': line, 'kind': 'unobserved_state_change',
                             'prior_after': previous, 'next_before': before})
            pending = row
        elif kind == 'bios_rng_return':
            if pending is None:
                raise ValueError('Return without an observed call')
            for name in ('service', 'rand_ordinal', 'caller_ra', 'seed_before32'):
                if u32(row, name) != u32(pending, name):
                    raise ValueError('Return identity differs for ' + name)
            before, after = u32(row, 'seed_before32'), u32(row, 'seed_after32')
            if pending['service'] == 0x2F:
                expected = (before * 1103515245 + 12345) & 0xFFFFFFFF
                if after != expected or u32(row, 'v0_actual') != (after >> 16) & 0x7FFF:
                    raise ValueError('Actual BIOS return/state differs from installed profile')
                returns[ordinal] = row
            else:
                if after != u32(pending, 'a0_argument'):
                    raise ValueError('Actual srand state differs from observed argument')
                reseeds += 1
            pending, previous = None, after
        elif kind == 'class03_sample_store':
            if not admitted:
                raise ValueError('Class03 sample before proved overlay admission')
            if row.get('overlay_identity_matches') is not True:
                raise ValueError('Class03 overlay identity is not verified')
            ordinal_for_store = u32(row, 'rand_ordinal')
            # The observed return is at22F8 and must precede the later22FC
            # store. The old shared-PC/log-order premise conflated addresses.
            if ordinal_for_store not in returns:
                raise ValueError('Class03 sample precedes its actual BIOS return')
            if ordinal_for_store != ordinal or pending is not None:
                raise ValueError('Class03 sample lacks the current completed random ordinal')
            if ordinal_for_store in sample_ordinals:
                raise ValueError('Repeated class03 store for one random ordinal')
            sample_ordinals.add(ordinal_for_store)
            class03_context(row)
            samples.append(row)
        elif kind == 'class03_overlay_admitted':
            if row.get('overlay_identity_matches') is not True or \
                    row.get('source_overlay_sha256') != OVERLAY_SHA or admitted:
                raise ValueError('Invalid or repeated class03 overlay admission')
            admitted = True
        elif kind == 'class03_overlay_not_admitted':
            if admitted or row.get('overlay_identity_matches') is not False or \
                    row.get('actor_fields_read') is not False:
                raise ValueError('Invalid overlay skip or code changed after admission')
            skipped += 1
        else:
            raise ValueError('Unsupported trace row kind at line ' + str(line))
    for row in samples:
        record = returns.get(u32(row, 'rand_ordinal'))
        if record is None or record['caller_ra'] != CLASS03_RAND_CALLER_RA:
            raise ValueError('Class03 sample lacks its actual original BIOS return')
        if u32(row, 'v0_actual') != record['v0_actual'] or \
                u32(row, 'sample32A_to_store') != record['v0_actual'] & 65535:
            raise ValueError('Class03 sample differs from its actual return')
        slot = u32(row, 'slot')
        if slot >= 16 or u32(row, 'work') != 0x8009F108 + slot * 0x3C0:
            raise ValueError('Class03 work is outside original pool identity')
    return {
        'schema': 'mml-measured-rng-reference-validation/1',
        'status': 'MATCHES_MEASURED_FIELDS' if returns else 'UNKNOWN',
        'trace_rows': len(rows), 'actual_rand_returns': len(returns),
        'actual_reseed_returns': reseeds, 'actual_class03_samples': len(samples),
        'class03_overlay_admitted': admitted, 'not_admitted_overlay_markers': skipped,
        'class03_rand_caller_ra': CLASS03_RAND_CALLER_RA,
        'class03_sample_store_pc': CLASS03_SAMPLE_STORE_PC,
        'class03_captured_context_fields_validated': bool(samples),
        'class03_input_update_chronology_proved': False,
        'game_entry_boundaries': len(entries), 'unobserved_state_changes': gaps,
        'pending_return_at_end': pending is not None,
        'bounded_observer_completed': budget_complete,
        'continuous_since_installation': not gaps and pending is None,
        'game_entry_observed': bool(entries),
        'game_seed_for_retail_bios_proved': False,
        'shared_native_consumer_lifecycle_proved': False, 'retail_parity': False,
        'limits': [
            'This validates supplied observed scalars; it generates no missing samples.',
            'The installed OpenBIOS reference is not a proved retail BIOS profile.',
            'A game-entry boundary does not reconstruct BIOS consumers before installation.',
            'Imported actor identity, input chronology and native scheduling need independent matching.',
            'No seed or scalar is admitted automatically into the native runtime.',
        ],
    }


def validate_failed_prefix(rows):
    failure = next((index for index, row in enumerate(rows)
                    if row.get('kind') == 'observer_guard_failure'), None)
    if failure is None or failure != len(rows) - 1:
        raise ValueError('Expected one final observer failure for prefix-only analysis')
    result = validate(rows[:failure])
    result['whole_trace_accepted'] = False
    result['prefix_only_validation'] = True
    result['terminating_observer_failure'] = rows[failure]
    result['unvalidated_failure_line'] = failure + 1
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--prefix-before-failure', action='store_true',
                   help='Validate only the explicitly failed trace prefix; never accept the whole trace')
    a = p.parse_args()
    if a.out.exists():
        raise SystemExit('Refusing to overwrite measured evidence')
    if a.trace.stat().st_size > 16 * 1024 * 1024:
        raise SystemExit('Reference trace exceeds fixed byte budget')
    blob = a.trace.read_bytes()
    lines = blob.decode('utf-8').splitlines()
    if len(lines) > 65536:
        raise SystemExit('Reference trace exceeds fixed row budget')
    decoded = [json.loads(line) for line in lines]
    result = validate_failed_prefix(decoded) if a.prefix_before_failure else validate(decoded)
    result['trace_sha256'] = hashlib.sha256(blob).hexdigest()
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()

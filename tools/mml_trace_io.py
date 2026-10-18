#!/usr/bin/env python3
"""Wrap actual native replays or observed retail rows with explicit provenance.

This tool normalizes transport/input shape only. It does not rescale, interpolate,
align, offset, infer, or synthesize any gameplay measurement.
"""
import argparse
import copy
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / 'p2'))
from trace_compare import FORMAT, TraceProfileError, input_sha256, load_json, normalize_input, validate_trace


def assemble_trace(rows, metadata, source_kind):
    if not isinstance(rows, list) or not rows:
        raise TraceProfileError('capture input must be a nonempty JSON array of actual frame rows')
    if not isinstance(metadata, dict):
        raise TraceProfileError('metadata must be an object')
    meta = copy.deepcopy(metadata)
    if 'source_kind' in meta and meta['source_kind'] != source_kind:
        raise TraceProfileError('metadata source_kind contradicts selected ingest mode')
    meta['source_kind'] = source_kind
    frames = copy.deepcopy(rows)
    for row in frames:
        if not isinstance(row, dict):
            raise TraceProfileError('each capture row must be an object')
        row['input'] = normalize_input(row.get('input'))
    digest = input_sha256(frames)
    if 'input_sha256' in meta and meta['input_sha256'] != digest:
        raise TraceProfileError('provided input hash contradicts captured input sequence')
    meta['input_sha256'] = digest
    trace = {'format': FORMAT, 'metadata': meta, 'frames': frames}
    validate_trace(trace)
    return trace


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('wrap-native', 'ingest-retail'))
    parser.add_argument('rows', type=Path)
    parser.add_argument('metadata', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try:
        trace = assemble_trace(load_json(args.rows), load_json(args.metadata),
                               'native_replay' if args.mode == 'wrap-native' else 'retail_reference')
        # Refuse replacement so original measurements/checkpoints stay recoverable.
        with args.output.open('x') as output:
            output.write(json.dumps(trace, indent=2, allow_nan=False) + '\n')
    except (TraceProfileError, OSError, KeyError, TypeError) as error:
        parser.exit(2, 'trace ingest rejected: ' + str(error) + '\n')
    print(str(args.output))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

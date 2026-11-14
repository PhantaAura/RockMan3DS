#!/usr/bin/env python3
"""Compare a psxrecomp oracle run with a 3DS run (story flags, stage, event).

  record  Read the oracle's per-frame ring (set_snapshot regions armed with
          `arm` before the run) and write an oracle log CSV.
  compare Diff an oracle log against a 3DS hardware_01516_*.csv trace
          ("# story_flag,frame=..,flag=0x..,value=..,area=.." lines).

Flag n lives at 0x800BE378 + (n >> 3), bit 0x80 >> (n & 7). The arm step
snapshots flags 0x600..0x9FF (0x800BE438, 128 bytes) and the stage/event
bytes. Outputs are local evidence only; keep them out of the repo.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from psxrecomp_dbg import call  # noqa: E402

FLAGS_ADDR, FLAG_BASE = 0x800BE438, 0x600
STAGE_ADDR = 0x800C1B60
EVENT_ADDR = 0x800C4C48


def arm():
    for slot, addr in ((0, FLAGS_ADDR), (1, STAGE_ADDR & ~0x7F), (2, EVENT_ADDR & ~0x7F)):
        print(call('set_snapshot', slot=slot, addr=hex(addr)))


def frame_bytes(frame, addr, length):
    r = call('read_frame_ram', frame=frame, addr=hex(addr), len=length)
    return bytes.fromhex(r['hex']) if r.get('ok') else None


def record(lo, hi, out):
    prev = None
    rows = []
    for f in range(lo, hi):
        flags = frame_bytes(f, FLAGS_ADDR, 128)
        if flags is None:
            continue
        stage = frame_bytes(f, STAGE_ADDR, 1)
        event = frame_bytes(f, EVENT_ADDR, 4)
        if prev is not None:
            for i, (a, b) in enumerate(zip(prev, flags)):
                for bit in range(8):
                    m = 0x80 >> bit
                    if (a ^ b) & m:
                        rows.append((f, FLAG_BASE + i * 8 + bit, 1 if b & m else 0,
                                     stage[0] if stage else -1, event.hex() if event else ''))
        prev = flags
    with open(out, 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['frame', 'flag', 'value', 'stage', 'event'])
        w.writerows(rows)
    print(f'{len(rows)} flag changes written to {out}')


def load_oracle(path):
    with open(path) as fh:
        return [(int(r['frame']), int(r['flag']), int(r['value'])) for r in csv.DictReader(fh)]


def load_3ds(path):
    pat = re.compile(r'^# story_flag,frame=(\d+),flag=0x([0-9a-f]+),value=(\d)')
    out = []
    for line in open(path, errors='replace'):
        m = pat.match(line)
        if m:
            out.append((int(m.group(1)), int(m.group(2), 16), int(m.group(3))))
    return out


def compare(oracle_path, ds_path):
    o = [x for x in load_oracle(oracle_path) if x[2] == 1]
    d = [x for x in load_3ds(ds_path) if x[2] == 1]
    o_order = [f for _, f, _ in o]
    d_order = [f for _, f, _ in d]
    missing = [f for f in o_order if f not in d_order]
    extra = [f for f in d_order if f not in o_order]
    common = [f for f in o_order if f in d_order]
    d_common = [f for f in d_order if f in o_order]
    print(f'oracle sets {len(o_order)} flags, 3DS sets {len(d_order)}')
    print('missing on 3DS:', ' '.join(f'0x{f:03x}' for f in missing) or 'none')
    print('extra on 3DS:  ', ' '.join(f'0x{f:03x}' for f in extra) or 'none')
    print('order matches:', 'yes' if common == d_common else 'no')
    if common != d_common:
        for i, (a, b) in enumerate(zip(common, d_common)):
            if a != b:
                print(f'  first order difference at #{i}: oracle 0x{a:03x}, 3DS 0x{b:03x}')
                break
    of = {f: fr for fr, f, _ in o}
    df = {f: fr for fr, f, _ in d}
    print('flag      oracle_frame  3ds_frame')
    for f in common:
        print(f'0x{f:03x}  {of[f]:>12}  {df[f]:>9}')
    return 1 if missing or common != d_common else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('arm')
    r = sub.add_parser('record'); r.add_argument('lo', type=int); r.add_argument('hi', type=int); r.add_argument('out')
    c = sub.add_parser('compare'); c.add_argument('oracle'); c.add_argument('trace')
    a = ap.parse_args()
    if a.cmd == 'arm':
        arm()
    elif a.cmd == 'record':
        record(a.lo, a.hi, a.out)
    else:
        sys.exit(compare(a.oracle, a.trace))


if __name__ == '__main__':
    main()

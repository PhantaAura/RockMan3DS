#!/usr/bin/env python3
"""Compare a psxrecomp oracle run with a 3DS run (story flags, stage, event).

  record  Read the oracle's per-frame ring (set_snapshot regions armed with
          `arm` before the run) and write an oracle log CSV.
  compare Diff an oracle log against a 3DS hardware_01516_*.csv trace
          ("# story_flag,frame=..,flag=0x..,value=..,area=.." lines).

Flag n lives at 0x800BE378 + (n >> 3), bit 0x80 >> (n & 7). The arm step
snapshots flags 0x600..0x9FF (0x800BE438, 128 bytes), flags 0x000..0x3FF
(0x800BE378, 128 bytes; M3's story flags such as 0x03, 0x0A, 0x384) and the
stage/event bytes. The ring keeps 36000 frames. Outputs are local evidence only; keep them out of the repo.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from psxrecomp_dbg import call  # noqa: E402

FLAGS_ADDR = 0x800BE438         # flags 0x600..0x9FF
LOW_FLAGS_ADDR = 0x800BE378   # flags 0x000..0x3FF
STAGE_ADDR = 0x800C1B60
EVENT_ADDR = 0x800C4C48


def arm():
    for slot, addr in ((0, FLAGS_ADDR), (1, STAGE_ADDR & ~0x7F), (2, EVENT_ADDR & ~0x7F),
                       (3, LOW_FLAGS_ADDR)):
        print(call('set_snapshot', slot=slot, addr=hex(addr)))


def frame_bytes(frame, addr, length):
    r = call('read_frame_ram', frame=frame, addr=hex(addr), len=length)
    return bytes.fromhex(r['hex']) if r.get('ok') else None


def record(lo, hi, out):
    prev = None
    rows = []
    for f in range(lo, hi):
        hi_flags = frame_bytes(f, FLAGS_ADDR, 128)
        lo_flags = frame_bytes(f, LOW_FLAGS_ADDR, 128)
        if hi_flags is None or lo_flags is None:
            continue
        flags = lo_flags + bytes(64) + hi_flags   # 0x000..0x3FF, gap, 0x600..0x9FF
        stage = frame_bytes(f, STAGE_ADDR, 1)
        event = frame_bytes(f, EVENT_ADDR, 4)
        if prev is not None:
            for i, (a, b) in enumerate(zip(prev, flags)):
                for bit in range(8):
                    m = 0x80 >> bit
                    if (a ^ b) & m:
                        rows.append((f, i * 8 + bit, 1 if b & m else 0,
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


def _fmt(ev):
    return f"{'set' if ev[1] else 'clr'} 0x{ev[0]:03x}"


def compare(oracle_path, ds_path, allow_extra=False):
    """Compare set AND clear events in order. Fails on missing, extra
    (unless allow_extra) or out-of-order events. Flags only: matching here
    says nothing about movement, combat, camera or animation parity."""
    o = load_oracle(oracle_path)
    d = load_3ds(ds_path)
    o_ev = [(f, v) for _, f, v in o]
    d_ev = [(f, v) for _, f, v in d]
    o_left = list(o_ev)
    d_left = list(d_ev)
    for ev in d_ev:
        if ev in o_left:
            o_left.remove(ev)
    for ev in o_ev:
        if ev in d_left:
            d_left.remove(ev)
    missing, extra = o_left, d_left
    o_rest = list(missing)
    common = []
    for ev in o_ev:
        if ev in o_rest:
            o_rest.remove(ev)
        else:
            common.append(ev)
    d_rest = list(extra)
    d_common = []
    for ev in d_ev:
        if ev in d_rest:
            d_rest.remove(ev)
        else:
            d_common.append(ev)
    sets = lambda evs: sum(1 for _, v in evs if v)
    print(f'oracle: {sets(o_ev)} sets, {len(o_ev) - sets(o_ev)} clears; '
          f'3DS: {sets(d_ev)} sets, {len(d_ev) - sets(d_ev)} clears')
    print('missing on 3DS:', ', '.join(map(_fmt, missing)) or 'none')
    print('extra on 3DS:  ', ', '.join(map(_fmt, extra)) or 'none',
          '(allowed)' if extra and allow_extra else '')
    order_ok = common == d_common
    print('order matches:', 'yes' if order_ok else 'no')
    if not order_ok:
        for i, (a, b) in enumerate(zip(common, d_common)):
            if a != b:
                print(f'  first order difference at #{i}: oracle {_fmt(a)}, 3DS {_fmt(b)}')
                break
    print('event     oracle_frame  3ds_frame')
    o_fr = [(f, v, fr) for fr, f, v in o]
    d_fr = [(f, v, fr) for fr, f, v in d]
    for ev in common:
        a = next(x for x in o_fr if x[:2] == ev); o_fr.remove(a)
        b = next(x for x in d_fr if x[:2] == ev); d_fr.remove(b)
        print(f'{_fmt(ev)}  {a[2]:>12}  {b[2]:>9}')
    print('note: flag events only; not evidence of movement/combat/camera/animation parity')
    return 1 if missing or (extra and not allow_extra) or not order_ok else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('arm')
    r = sub.add_parser('record'); r.add_argument('lo', type=int); r.add_argument('hi', type=int); r.add_argument('out')
    c = sub.add_parser('compare'); c.add_argument('oracle'); c.add_argument('trace')
    c.add_argument('--allow-extra', action='store_true', help='do not fail on flag events the oracle run lacks')
    a = ap.parse_args()
    if a.cmd == 'arm':
        arm()
    elif a.cmd == 'record':
        record(a.lo, a.hi, a.out)
    else:
        sys.exit(compare(a.oracle, a.trace, a.allow_extra))


if __name__ == '__main__':
    main()

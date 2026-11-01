#!/usr/bin/env python3
"""Cutscene importer: find a stage event's data-driven parts and cook L3CS.

Every cutscene runs on the same engine services, configured by constants the
event code stores. Given the owner EXE, a stage overlay (loaded at
0x80100000) and an event id, this follows the EXE event table 0x80089024 to
the handler, its dispatch tables, the overlay functions it calls and the
timeline callbacks, and records:
  - camera programs: words stored to the camera block +0xB8 (0x800BC7F0),
    read by 0x8001EC0C up to 0xFFFFFFFF;
  - timelines: words stored to event+0xA4, 8-byte records {u16 event+8,
    u16 ticks, callback} up to the 0xFF terminator (0x8001F070);
  - spawn records: constant arguments of 0x8001EAE8 (20 bytes each);
  - CD/XA requests: constant arguments of 0x8001D58C;
  - area requests: constant arguments of overlay functions that store the
    stage loader's area byte 0x800C356F;
  - the stage's background-layer table: the indexed word the stage init
    passes to 0x80022FD4 (one pointer per area).
The scan follows straight-line register constants (lui/addiu/ori/moves),
which is how these setups are written; per-scene behaviour (the callbacks
themselves) is not imported and stays hand-ported. --report writes the
findings as JSON. Output is owner-local data, never part of the source release.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
BASE = 0x80100000
EVENT_TABLE = 0x80089024
CAMERA_PC = 0x800BC7F0
SPAWN = 0x8001EAE8
CD_REQUEST = 0x8001D58C
BG_INIT = 0x80022FD4
AREA_BYTE = 0x800C356F
MAX_RECORDS, MAX_WORDS, MAX_SPAWNS = 32, 512, 64
CALLER_SAVED = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 31)


class Image:
    def __init__(self, data, base):
        self.data, self.base = data, base
    def contains(self, a, n=4):
        return self.base <= a and a + n <= self.base + len(self.data)
    def word(self, a):
        return struct.unpack_from('<I', self.data, a - self.base)[0]


def exe_image(exe):
    if hashlib.sha256(exe).hexdigest() != EXE_SHA:
        raise ValueError('Unsupported owner executable identity')
    base, size = struct.unpack_from('<II', exe, 0x18)
    return Image(exe[2048:2048 + size], base)


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


class Scan:
    """Linear constant tracking over one function (to its first jr ra)."""
    def __init__(self, ov, start):
        self.calls, self.stores, self.indexed_loads, self.jumps, self.bounds = [], [], [], [], []
        regs = {0: 0}            # register -> constant
        indexed = {}             # register -> base of an indexed address
        a = start
        pending = None           # a call/jump waiting for its delay slot
        for _ in range(2048):
            if not ov.contains(a): break
            w = ov.word(a)
            op, rs, rt, rd, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
            imm = w & 0xFFFF
            def put(r, v):
                if r == 0: return
                indexed.pop(r, None)
                if v is None: regs.pop(r, None)
                else: regs[r] = v & 0xFFFFFFFF
            done = False
            if op == 0x0F: put(rt, imm << 16)                                     # lui
            elif op == 0x09:                                                       # addiu
                put(rt, None if rs not in regs else regs[rs] + s16(imm))
            elif op == 0x0D: put(rt, None if rs not in regs else regs[rs] | imm)  # ori
            elif op == 0 and fn in (0x21, 0x25):                                  # addu / or
                if rt == 0 and rs in regs: put(rd, regs[rs])
                elif rs == 0 and rt in regs: put(rd, regs[rt])
                elif rs in regs and rt in regs: put(rd, regs[rs] + regs[rt] if fn == 0x21 else regs[rs] | regs[rt])
                elif fn == 0x21 and (rs in regs) != (rt in regs):
                    base = regs[rs] if rs in regs else regs[rt]
                    put(rd, None); indexed[rd] = base
                else: put(rd, None)
            elif op == 0x0B:                                                      # sltiu: a dispatch bound
                self.bounds.append((a, imm)); put(rt, None)
            elif op in (0x23, 0x24, 0x25, 0x20, 0x21):                            # lw lbu lhu lb lh
                if rs in indexed: self.indexed_loads.append((a, op, rt, (indexed[rs] + s16(imm)) & 0xFFFFFFFF))
                put(rt, None)
            elif op in (0x2B, 0x29, 0x28):                                        # sw sh sb
                addr = (regs[rs] + s16(imm)) & 0xFFFFFFFF if rs in regs else None
                self.stores.append((a, op, rs, s16(imm), addr, regs.get(rt)))
            elif op == 0x03:                                                      # jal
                pending = ('call', ((a + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2))
            elif op == 0 and fn == 0x09: pending = ('jalr', None)                 # jalr
            elif op == 0 and fn == 0x08:                                          # jr
                pending = ('ret', None) if rs == 31 else ('jr', None)
            elif op == 0 and fn not in (0x08, 0x09): put(rd, None)                # other R-type writes rd
            elif op in (0x08, 0x0A, 0x0C, 0x0E, 0x22, 0x26): put(rt, None)  # other I-type writes rt
            if pending and pending[2:] == ('slot',):
                kind, target = pending[0], pending[1]
                if kind == 'call': self.calls.append((a - 4, target, regs.get(4)))
                if kind == 'jalr': self.jumps.append(a - 4)
                if kind in ('call', 'jalr'):
                    for r in CALLER_SAVED: regs.pop(r, None); indexed.pop(r, None)
                if kind == 'ret': done = True
                pending = None
            elif pending:
                pending = pending + ('slot',)
            if done: break
            a += 4
        self.end = a


def code_pointer(ov, v):
    return ov.contains(v) and v % 4 == 0 and v < ov.base + len(ov.data)


def read_timeline(ov, t):
    records = []
    for k in range(MAX_RECORDS):
        if not ov.contains(t + 8 * k, 8): raise ValueError(f'timeline {t:#x} leaves the overlay')
        header, callback = ov.word(t + 8 * k), ov.word(t + 8 * k + 4)
        records.append((header, callback))
        if header & 0xFF == 0xFF: return records
    raise ValueError(f'timeline {t:#x} exceeds {MAX_RECORDS} records')


def read_camera(ov, c):
    """Commands as 0x8001EC0C walks them: ops 0-6 and 32-34 take one word,
    16-22 take three operand words; op 0xFF (incl. 0xFFFFFFFF) ends."""
    words, k = [], 0
    while k < MAX_WORDS:
        if not ov.contains(c + 4 * k): raise ValueError(f'camera program {c:#x} leaves the overlay')
        command = ov.word(c + 4 * k)
        op = command >> 24
        if op == 0xFF:
            words.append(command); return words
        size = 4 if 16 <= op <= 22 else 1 if op <= 6 or 32 <= op <= 34 else 0
        if not size: raise ValueError(f'camera program {c:#x}: unsupported op {op:#x} at word {k}')
        words.extend(ov.word(c + 4 * (k + i)) for i in range(size))
        k += size
    raise ValueError(f'camera program {c:#x} exceeds {MAX_WORDS} words')


def scan(exe, overlay, event, extra_data=()):
    ex = exe_image(exe)
    ov = Image(overlay, BASE)
    handler = ex.word(EVENT_TABLE + 4 * event)
    if not code_pointer(ov, handler): raise ValueError(f'event {event} handler {handler:#x} is not in this overlay')
    seen, queue, scans = set(), [handler], {}
    tables, timelines, cameras, spawns, requests, calls_with_arg = {}, {}, {}, {}, [], []
    while queue:
        f = queue.pop(0)
        if f in seen or not code_pointer(ov, f) or len(seen) > 256: continue
        seen.add(f)
        s = Scan(ov, f); scans[f] = s
        for (load, op, rt, addr) in s.indexed_loads:                  # dispatch tables
            if op == 0x23 and ov.contains(addr):
                # The handler's own bound (sltiu just before the load) caps the
                # table; without one, entries run while they are code pointers.
                bound = [n for (at, n) in s.bounds if load - 32 <= at < load]
                entries = []
                for k in range(bound[-1] if bound else 64):
                    if not ov.contains(addr + 4 * k): break
                    v = ov.word(addr + 4 * k)
                    if not code_pointer(ov, v): break
                    entries.append(v)
                if entries and (s.jumps or True):
                    tables[addr] = entries; queue.extend(entries)
        for (site, target, a0) in s.calls:
            if code_pointer(ov, target): queue.append(target)
            if target == SPAWN and a0 is not None: spawns[a0] = None
            if target == CD_REQUEST and a0 is not None: requests.append(a0)
            if code_pointer(ov, target) and a0 is not None: calls_with_arg.append((f, target, a0))
        for (site, op, rs, off, addr, value) in s.stores:
            if value is None: continue
            if op == 0x2B and addr == CAMERA_PC and ov.contains(value): cameras[value] = None
            if op == 0x2B and addr is None and off == 0xA4 and ov.contains(value): timelines[value] = None
    for t in list(timelines):
        for header, cb in read_timeline(ov, t):
            if code_pointer(ov, cb) and cb not in seen: queue.append(cb)
    # Second pass for the callbacks the timelines named.
    while queue:
        f = queue.pop(0)
        if f in seen or not code_pointer(ov, f): continue
        seen.add(f); s = Scan(ov, f); scans[f] = s
        for (site, target, a0) in s.calls:
            if code_pointer(ov, target) and target not in seen: queue.append(target)
            if target == SPAWN and a0 is not None: spawns[a0] = None
            if target == CD_REQUEST and a0 is not None: requests.append(a0)
            if code_pointer(ov, target) and a0 is not None: calls_with_arg.append((f, target, a0))
        for (site, op, rs, off, addr, value) in s.stores:
            if value is None: continue
            if op == 0x2B and addr == CAMERA_PC and ov.contains(value): cameras[value] = None
            if op == 0x2B and addr is None and off == 0xA4 and ov.contains(value): timelines[value] = None
    # Area requests: called overlay functions that store a0 to 0x800C356F.
    area_fns = set()
    for f, s in scans.items():
        for (site, op, rs, off, addr, value) in s.stores:
            if op == 0x28 and addr == AREA_BYTE: area_fns.add(f)
    for f in {t for (_, t, _) in calls_with_arg} - set(scans):
        s = Scan(ov, f)
        if any(op == 0x28 and addr == AREA_BYTE for (_, op, _, _, addr, _) in s.stores): area_fns.add(f)
    areas = sorted({(caller, a0) for (caller, target, a0) in calls_with_arg if target in area_fns})
    # The stage's background table: the indexed word passed to 0x80022FD4.
    bg_table = None
    for a in range(ov.base, ov.base + len(ov.data) - 4, 4):
        w = ov.word(a)
        if (w & 0xFFFF0000) == 0x27BD0000 or a == ov.base:
            s = Scan(ov, a)
            for (site, target, a0) in s.calls:
                if target == BG_INIT:
                    loads = [x for x in s.indexed_loads if x[0] < site and x[2] == 4 and x[1] == 0x23]
                    if loads: bg_table = loads[-1][3]
            if bg_table: break
    cams, rejected = {}, {}
    for c in sorted(cameras):   # a stored word that does not parse as a program is reported, not cooked
        try: cams[c] = read_camera(ov, c)
        except ValueError as error: rejected[c] = str(error)
    return {
        'event': event, 'handler': handler,
        'dispatch_tables': {k: v for k, v in tables.items()},
        'functions': sorted(seen),
        'timelines': {t: read_timeline(ov, t) for t in sorted(timelines)},
        'cameras': cams,
        'rejected_cameras': rejected,
        'spawns': sorted(spawns),
        'cd_requests': sorted(set(requests)),
        'area_requests': areas,
        'bg_table': bg_table,
        'extra_data': [(a, n) for a, n in extra_data],
    }, ov


def package(found, ov, stage):
    payload = bytes.fromhex(EXE_SHA) + hashlib.sha256(ov.data).digest()
    payload += struct.pack('<HHI', found['event'], stage, found['handler'])
    tl, cam = found['timelines'], found['cameras']
    payload += struct.pack('<I', len(tl))
    for t, records in tl.items():
        payload += struct.pack('<II', t, len(records)) + b''.join(struct.pack('<II', h, c) for h, c in records)
    payload += struct.pack('<I', len(cam))
    for c, words in cam.items():
        payload += struct.pack('<II', c, len(words)) + b''.join(struct.pack('<I', w) for w in words)
    payload += struct.pack('<I', len(found['spawns']))
    for s in found['spawns']:
        payload += struct.pack('<I', s) + ov.data[s - BASE:s - BASE + 20]
    payload += struct.pack('<I', len(found['cd_requests'])) + b''.join(struct.pack('<I', r) for r in found['cd_requests'])
    payload += struct.pack('<I', len(found['area_requests'])) + b''.join(struct.pack('<Ii', c, a) for c, a in found['area_requests'])
    payload += struct.pack('<I', found['bg_table'] or 0)
    payload += struct.pack('<I', len(found['extra_data']))
    for a, n in found['extra_data']:
        if not ov.contains(a, n): raise ValueError(f'extra data {a:#x}+{n} leaves the overlay')
        payload += struct.pack('<II', a, n) + ov.data[a - BASE:a - BASE + n]
    return struct.pack('<4sHHII', b'L3CS', 1, 16, 16 + len(payload), zlib.crc32(payload) & 0xFFFFFFFF) + payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--overlay', required=True, type=Path)
    parser.add_argument('--event', required=True, type=int)
    parser.add_argument('--stage', required=True, type=int)
    parser.add_argument('--extra', action='append', default=[], help='ADDR:BYTES of event data to carry (hex address)')
    parser.add_argument('--out', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    extra = [(int(x.split(':')[0], 16), int(x.split(':')[1], 0)) for x in args.extra]
    found, ov = scan(args.exe.read_bytes(), args.overlay.read_bytes(), args.event, extra)
    if args.report:
        hexed = {k: v for k, v in found.items()}
        hexed['handler'] = hex(found['handler'])
        hexed['dispatch_tables'] = {hex(k): [hex(x) for x in v] for k, v in found['dispatch_tables'].items()}
        hexed['functions'] = [hex(x) for x in found['functions']]
        hexed['timelines'] = {hex(t): [[hex(h), hex(c)] for h, c in r] for t, r in found['timelines'].items()}
        hexed['cameras'] = {hex(c): len(w) for c, w in found['cameras'].items()}
        hexed['rejected_cameras'] = {hex(c): e for c, e in found['rejected_cameras'].items()}
        hexed['spawns'] = [hex(s) for s in found['spawns']]
        hexed['cd_requests'] = [hex(r) for r in found['cd_requests']]
        hexed['area_requests'] = [[hex(c), a] for c, a in found['area_requests']]
        hexed['bg_table'] = hex(found['bg_table']) if found['bg_table'] else None
        hexed['extra_data'] = [[hex(a), n] for a, n in found['extra_data']]
        args.report.write_text(json.dumps(hexed, indent=1))
    if args.out: args.out.write_bytes(package(found, ov, args.stage))


if __name__ == '__main__':
    main()

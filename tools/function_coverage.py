#!/usr/bin/env python3
"""Function-coverage audit: original functions the 1.0 route runs that the port never cites.

There is no public Mega Man Legends decompilation, so this audit stands in for
one. It joins three lists:

1. All functions: Ghidra's function list of ROCK_NEO.EXE and of each 1.0 stage
   overlay R3_STxx.BIN (`ghidra` step).
2. Ported: every 0x8xxxxxxx address the port cites in src/, include/,
   platform/3ds/source/, tools/ and tests/. An address inside a function's
   range covers that function.
3. Executed: per-function hit counts from the psxrecomp oracle while it replays
   the M3 savestates (`collect` step; needs the debug-only fn_hist patch,
   tools/psxrecomp_fn_hist.patch).

and writes docs/validation_01516/FUNCTION_COVERAGE.md (`report` step).

Retail rule (AGENTS.md): the function lists, hit dumps and overlay bytes are
derived from the disc. They stay outside the repo (default: the work directory
given with --work). The report lists only addresses, counts and guesses.

Steps:
  function_coverage.py ghidra  --work DIR --dat <owner-data>/extracted/CDDATA/DAT
                               --exe-project <ghidra project dir> [--exe-name ROCK_NEO_mcp]
  function_coverage.py collect --work DIR --recomp <MMLUSRecomp dir> [--frames 3000]
  function_coverage.py report  --work DIR --dat ... --exe <ROCK_NEO.EXE>
"""
from __future__ import annotations

import argparse
import bisect
import collections
import csv
import json
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
CITE_DIRS = ('src', 'include', 'platform/3ds/source', 'tools', 'tests')
SELF_EXCLUDE = {'tools/function_coverage.py', 'tools/psxrecomp_fn_hist.patch'}
CITE_EXT = {'.c', '.cc', '.cpp', '.h', '.hpp', '.py', '.sh', '.inc', '.txt', '.md', '.json', '.lua'}

OVL_BASE = 0x80100000
EXE_LO, EXE_HI = 0x80010000, 0x800D9000          # ROCK_NEO.EXE text (load 0x80010000, size 0xC9000)
# 1.0 stages (docs/validation_01516/M3_ROADMAP.md): ST00 tutorial, ST02 sky,
# ST03 crash site/outskirts, ST0C forest path, ST04 Apple Market, ST09 ruin,
# ST05 Uptown + Tron fight (area 1), ST06 City Hall plaza, ST0F City Hall interiors.
STAGES = ['ST00', 'ST02', 'ST03', 'ST0C', 'ST04', 'ST09', 'ST05', 'ST06', 'ST0F']
STAGE_NOTE = {
    'ST00': 'tutorial ruin (M1)', 'ST02': 'sky / Flutter flight (S0)',
    'ST03': 'crash site, outskirts (S1, S3, S6, S9)', 'ST0C': 'forest path (S2)',
    'ST04': 'Apple Market (S4, S7, S9)', 'ST09': 'first ruin (S5)',
    'ST05': 'Uptown, tank mission, Tron fight (S8-S11)', 'ST06': 'City Hall plaza (S8)',
    'ST0F': 'City Hall interiors (S8)',
}

# Addresses with an established meaning (repo docs); used for the one-line guesses.
KNOWN_FUNCS = {
    0x8001DA58: 'story-flag test', 0x8001DA8C: 'story-flag set', 0x8001DCD0: 'story-flag clear',
    0x800519E4: 'door lock check', 0x800413F0: 'talk-target pick', 0x80015EE8: 'player spawn',
    0x80048898: 'area arrival', 0x80050C2C: 'door event', 0x800514FC: 'door sample test',
}
KNOWN_DATA = [  # (lo, hi, label)
    (0x800BE378, 0x800BE478, 'story flags'), (0x800B51B0, 0x800B5400, 'player work'),
    (0x8009F108, 0x800A2D08, 'NPC table'), (0x800C3580, 0x800C3800, 'large-actor table'),
    (0x800C1B10, 0x800C1B12, 'main phase'), (0x800C1B60, 0x800C1B62, 'stage/area'),
    (0x800C3560, 0x800C3572, 'stage-transition lanes'), (0x800C4C48, 0x800C4C50, 'event state'),
    (0x1F800000, 0x1F800400, 'scratchpad'),
]
HW_RANGES = [  # offsets from 0x1F800000
    (0x1040, 0x1060, 'pad/memory-card SIO'), (0x1070, 0x1078, 'IRQ control'),
    (0x1080, 0x1100, 'DMA'), (0x1100, 0x1130, 'root counters'), (0x1800, 0x1804, 'CD-ROM'),
    (0x1810, 0x1818, 'GPU'), (0x1820, 0x1828, 'MDEC'), (0x1C00, 0x2000, 'SPU'),
]
# PsyQ BIOS vectors (subset; names per the public nocash PSX spec).
BIOS_NAMES = {
    ('A0', 0x13): 'setjmp', ('A0', 0x14): 'longjmp', ('A0', 0x17): 'strcmp', ('A0', 0x19): 'strcpy',
    ('A0', 0x25): 'toupper', ('A0', 0x28): 'bzero', ('A0', 0x2A): 'memcpy', ('A0', 0x2B): 'memset',
    ('A0', 0x2F): 'rand', ('A0', 0x30): 'srand', ('A0', 0x33): 'malloc', ('A0', 0x34): 'free',
    ('A0', 0x39): 'InitHeap', ('A0', 0x3F): 'printf', ('A0', 0x44): 'FlushCache',
    ('A0', 0x49): 'GPU_cw', ('A0', 0x72): 'CdRemove', ('A0', 0x9F): 'SetMem',
    ('B0', 0x07): 'DeliverEvent', ('B0', 0x08): 'OpenEvent', ('B0', 0x09): 'CloseEvent',
    ('B0', 0x0A): 'WaitEvent', ('B0', 0x0B): 'TestEvent', ('B0', 0x0C): 'EnableEvent',
    ('B0', 0x0D): 'DisableEvent', ('B0', 0x12): 'InitPad', ('B0', 0x13): 'StartPad',
    ('B0', 0x14): 'StopPad', ('B0', 0x17): 'ReturnFromException', ('B0', 0x18): 'SetDefaultExitFromException',
    ('B0', 0x19): 'SetCustomExitFromException', ('B0', 0x32): 'open', ('B0', 0x33): 'lseek',
    ('B0', 0x34): 'read', ('B0', 0x35): 'write', ('B0', 0x36): 'close', ('B0', 0x41): 'format',
    ('B0', 0x42): 'firstfile', ('B0', 0x43): 'nextfile', ('B0', 0x45): 'erase',
    ('B0', 0x4A): 'InitCARD', ('B0', 0x4B): 'StartCARD', ('B0', 0x4C): 'StopCARD',
    ('B0', 0x4E): '_card_write', ('B0', 0x4F): '_card_read', ('B0', 0x50): '_new_card',
    ('B0', 0x56): 'GetC0Table', ('B0', 0x57): 'GetB0Table', ('B0', 0x58): '_card_chan',
    ('B0', 0x5B): 'ChangeClearPad', ('B0', 0x5C): '_card_status', ('B0', 0x5D): '_card_wait',
    ('C0', 0x02): 'SysEnqIntRP', ('C0', 0x03): 'SysDeqIntRP', ('C0', 0x0A): 'ChangeClearRCnt',
}


# ----------------------------------------------------------------- inputs

def overlay_payloads(dat: Path) -> dict[str, bytes]:
    """R3_STxx.BIN payload of each 1.0 stage container (2048-byte resource headers)."""
    out = {}
    for st in STAGES:
        d = (dat / f'{st}.BIN').read_bytes()
        for off in range(0, len(d) - 2048, 2048):
            z = d.find(b'\0', off + 0x40, off + 0x140)
            name = d[off + 0x40:z] if z > 0 else b''
            if b'R3_' in name.upper():
                size, load = struct.unpack_from('<I', d, off + 4)[0], struct.unpack_from('<I', d, off + 12)[0]
                if load != OVL_BASE:
                    raise SystemExit(f'{st}: overlay load 0x{load:08X}, expected 0x{OVL_BASE:08X}')
                out[st] = d[off + 2048:off + 2048 + size]
                break
        else:
            raise SystemExit(f'{st}: no R3_ overlay in {st}.BIN')
    return out


def exe_image(exe: Path) -> tuple[int, bytes]:
    d = exe.read_bytes()
    if d[:8] != b'PS-X EXE':
        raise SystemExit(f'{exe}: not a PS-X EXE')
    load, size = struct.unpack_from('<II', d, 0x18)
    return load, d[0x800:0x800 + size]


class Image:
    """One code image (EXE or one overlay) with its Ghidra functions."""

    def __init__(self, name: str, base: int, data: bytes | None, funcs: list[tuple[int, int, str]]):
        self.name, self.base, self.data = name, base, data
        self.funcs = sorted(funcs)
        self.starts = [f[0] for f in self.funcs]

    def find(self, addr: int):
        i = bisect.bisect_right(self.starts, addr) - 1
        if i >= 0 and self.funcs[i][0] <= addr <= self.funcs[i][1]:
            return self.funcs[i]
        return None

    def contains(self, addr: int) -> bool:
        return self.data is not None and self.base <= addr < self.base + len(self.data)

    def words(self, lo: int, hi: int):
        if self.data is None:
            return []
        a, b = max(lo, self.base), min(hi + 1, self.base + len(self.data))
        return [(x, struct.unpack_from('<I', self.data, x - self.base)[0]) for x in range(a & ~3, b, 4)]


def merge_funcs(*lists) -> list[tuple[int, int, str]]:
    """Union of function starts from several Ghidra projects; a start found by
    one project splits a range from another (end clipped to the next start).
    The first list's name wins unless it is a default FUN_ name."""
    by = {}
    for lst in lists:
        for lo, hi, nm in lst:
            if lo not in by:
                by[lo] = [hi, nm]
            elif by[lo][1].startswith('FUN_') and not nm.startswith('FUN_'):
                by[lo][1] = nm
    starts = sorted(by)
    out = []
    for i, lo in enumerate(starts):
        hi = by[lo][0]
        if i + 1 < len(starts):
            hi = min(hi, starts[i + 1] - 1)
        out.append((lo, max(lo, hi), by[lo][1]))
    return out


def load_syms(path: Path | None) -> dict[int, str]:
    """`name = 0xADDR;` lines (MML1x10 config/syms.us.rock_neo.txt)."""
    out = {}
    if path and path.exists():
        for m in re.finditer(r'^\s*([A-Za-z_][\w]*)\s*=\s*0x([0-9A-Fa-f]{8})\s*;', path.read_text(), re.M):
            out[int(m.group(2), 16)] = m.group(1)
    return out


def load_funcs(path: Path) -> list[tuple[int, int, str]]:
    out = []
    with path.open() as f:
        for row in csv.reader(f):
            if len(row) >= 3:
                out.append((int(row[0], 16), int(row[1], 16), row[2]))
    return out


# ----------------------------------------------------------------- step 1: Ghidra

DUMP_JAVA = r'''
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import java.io.*;
public class DumpFuncs extends GhidraScript {
  public void run() throws Exception {
    try (PrintWriter w = new PrintWriter(new FileWriter(getScriptArgs()[0]))) {
      for (Function f : currentProgram.getFunctionManager().getFunctions(true))
        w.printf("%s,%s,%s%n", f.getEntryPoint(), f.getBody().getMaxAddress(), f.getName());
    }
  }
}
'''
# Raw overlays have no entry points. Seed: the first word, every prologue
# (addiu sp,sp,-N) right after "jr ra; <delay>", and every aligned data word
# pointing at a prologue (handler tables). Ghidra's analysis follows the calls.
SEED_JAVA = r'''
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import java.util.*;
public class SeedOverlay extends GhidraScript {
  public void run() throws Exception {
    Memory m = currentProgram.getMemory();
    MemoryBlock b = m.getBlocks()[0];
    long lo = b.getStart().getOffset(), hi = b.getEnd().getOffset();
    int n = (int)((hi - lo + 1) / 4);
    int[] w = new int[n];
    for (int i = 0; i < n; i++) w[i] = m.getInt(b.getStart().add(4L*i));
    TreeSet<Long> seeds = new TreeSet<>();
    seeds.add(lo);
    for (int i = 2; i < n; i++)
      if ((w[i] & 0xFFFF8000) == 0x27BD8000 && w[i-2] == 0x03E00008) seeds.add(lo + 4L*i);
    for (int i = 0; i < n; i++) {
      long v = w[i] & 0xFFFFFFFFL;
      if (v >= lo && v <= hi && (v & 3) == 0 && (w[(int)((v - lo) / 4)] & 0xFFFF8000) == 0x27BD8000) seeds.add(v);
    }
    for (long s : seeds) new DisassembleCommand(toAddr(s), null, true).applyTo(currentProgram, monitor);
    for (long s : seeds) if (getFunctionAt(toAddr(s)) == null) new CreateFunctionCmd(toAddr(s)).applyTo(currentProgram, monitor);
    println("seeds=" + seeds.size());
  }
}
'''


def ghidra_env(a):
    env = dict(os.environ)
    if a.java_home:
        env['JAVA_HOME'] = a.java_home
        env['PATH'] = f'{a.java_home}/bin:' + env['PATH']
    return env


def cmd_ghidra(a):
    work = Path(a.work); funcs = work / 'funcs'; funcs.mkdir(parents=True, exist_ok=True)
    scripts = work / 'ghidra_scripts'; scripts.mkdir(exist_ok=True)
    (scripts / 'DumpFuncs.java').write_text(DUMP_JAVA)
    (scripts / 'SeedOverlay.java').write_text(SEED_JAVA)
    head = str(Path(a.ghidra) / 'support' / 'analyzeHeadless')
    env = ghidra_env(a)
    proj = work / 'ghidra'; proj.mkdir(exist_ok=True)
    # EXE: dump the saved, pre-analysed project. It is often locked by a running
    # Ghidra MCP server, so dump a copy.
    src = Path(a.exe_project)
    shutil.copy2(src / f'{a.exe_name}.gpr', proj)
    shutil.copytree(src / f'{a.exe_name}.rep', proj / f'{a.exe_name}.rep', dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('*.lock*', 'tmp*'))
    r = subprocess.run([head, str(proj), a.exe_name, '-process', '-noanalysis', '-readOnly',
                        '-scriptPath', str(scripts), '-postScript', 'DumpFuncs.java', str(funcs / 'EXE.csv')],
                       env=env, capture_output=True, text=True)
    print('EXE', r.returncode, len(load_funcs(funcs / 'EXE.csv')) if (funcs / 'EXE.csv').exists() else 0)
    if a.psx_name and (src / f'{a.psx_name}.gpr').exists():
        # Older project made with the ghidra_psx_ldr loader: PsyQ signature names.
        shutil.copy2(src / f'{a.psx_name}.gpr', proj)
        shutil.copytree(src / f'{a.psx_name}.rep', proj / f'{a.psx_name}.rep', dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('*.lock*', 'tmp*'))
        r = subprocess.run([head, str(proj), a.psx_name, '-process', '-noanalysis', '-readOnly',
                            '-scriptPath', str(scripts), '-postScript', 'DumpFuncs.java', str(funcs / 'EXE_psx.csv')],
                           env=env, capture_output=True, text=True)
        print('EXE_psx', r.returncode)
    ovl = overlay_payloads(Path(a.dat))
    tmp = work / 'ovl'; tmp.mkdir(exist_ok=True)
    for st, data in ovl.items():
        p = tmp / f'R3_{st}.BIN'; p.write_bytes(data)
        r = subprocess.run([head, str(proj), f'OV_{st}', '-import', str(p), '-loader', 'BinaryLoader',
                            '-loader-baseAddr', hex(OVL_BASE), '-processor', 'MIPS:LE:32:default',
                            '-scriptPath', str(scripts), '-preScript', 'SeedOverlay.java',
                            '-postScript', 'DumpFuncs.java', str(funcs / f'{st}.csv'), '-deleteProject'],
                           env=env, capture_output=True, text=True)
        p.unlink()
        print(st, r.returncode, len(load_funcs(funcs / f'{st}.csv')) if (funcs / f'{st}.csv').exists() else 0)


# ----------------------------------------------------------------- step 2: citations

ADDR_RE = re.compile(r'0x(8[0-9A-Fa-f]{7})\b')
STAGE_RE = re.compile(r'\b(?:R3_)?(?:ST|st)[_ ]?([0-9A-Fa-f]{2})\b')


def stage_tokens(text: str) -> list[str]:
    out = []
    for m in STAGE_RE.finditer(text):
        st = 'ST' + m.group(1).upper()
        if st in STAGES:
            out.append(st)
    return out


def _cite_files(ref):
    if ref is None:
        out = subprocess.run(['git', '-C', str(REPO), 'ls-files', *CITE_DIRS],
                             capture_output=True, text=True, check=True).stdout.split()
        return [(rel, lambda rel=rel: (REPO / rel).read_text(errors='replace')) for rel in out]
    r = subprocess.run(['git', '-C', str(REPO), 'grep', '-l', '-E', '0x8[0-9A-Fa-f]{7}', ref, '--', *CITE_DIRS],
                       capture_output=True, text=True)
    files = [ln.split(':', 1)[1] for ln in r.stdout.split() if ':' in ln]
    return [(rel, lambda rel=rel: subprocess.run(['git', '-C', str(REPO), 'show', f'{ref}:{rel}'],
                                                 capture_output=True).stdout.decode('utf-8', 'replace'))
            for rel in files]


def collect_citations(ref=None) -> tuple[set[int], dict[str, set[int]]]:
    """EXE-range citations, and overlay-range citations per stage, in the
    working tree (ref None) or in a git ref.

    An overlay address (0x801xxxxx) is the same number in every stage, so it is
    attributed to the stage named on its line or the 3 lines around it, else
    the stage named most often in the file (or its path), else ST00 (the only
    stage the port ran before M3)."""
    exe, ovl = set(), collections.defaultdict(set)
    for rel, read in _cite_files(ref):
        if rel in SELF_EXCLUDE or Path(rel).suffix.lower() not in CITE_EXT:
            continue
        try:
            text = read()
        except OSError:
            continue
        if not ADDR_RE.search(text):
            continue
        lines = text.splitlines()
        counts = collections.Counter(stage_tokens(rel.replace('/', ' ').replace('_', ' ')) * 3)
        for ln in lines:
            counts.update(stage_tokens(ln))
        file_default = counts.most_common(1)[0][0] if counts else 'ST00'
        for i, ln in enumerate(lines):
            for m in ADDR_RE.finditer(ln):
                v = int(m.group(1), 16)
                if EXE_LO <= v < EXE_HI:
                    exe.add(v)
                elif OVL_BASE <= v < OVL_BASE + 0x80000:
                    near = stage_tokens(ln) or stage_tokens(' '.join(lines[max(0, i - 3):i + 4]))
                    for st in (set(near) or {file_default}):
                        ovl[st].add(v)
    return exe, ovl


# ----------------------------------------------------------------- step 3: oracle

class Oracle:
    def __init__(self, port=4370):
        self.port = port

    def call(self, cmd, **kw):
        kw.update(id=1, cmd=cmd)
        with socket.create_connection(('127.0.0.1', self.port), timeout=60) as s:
            s.sendall((json.dumps(kw) + '\n').encode())
            return json.loads(s.makefile('rb').readline())

    def frame(self):
        return int(self.call('frame')['frame'])

    def ram(self, addr, n):
        return bytes.fromhex(self.call('read_ram', addr=f'0x{addr:08X}', len=n)['hex'])

    def wait_frames(self, n, timeout=900):
        f0 = self.frame(); t0 = time.time()
        while self.frame() - f0 < n:
            if time.time() - t0 > timeout:
                raise TimeoutError(f'{n} frames not reached')
            time.sleep(0.5)


# Pad script per checkpoint (active-low PS1 bits). After an idle stretch the
# player walks forward pressing Circle (talk/doors) and turns now and then, so
# talk, door and movement code runs as well as the event in progress.
PAD = {'up': 0x10, 'right': 0x20, 'left': 0x80, 'circle': 0x2000, 'cross': 0x4000, 'square': 0x8000, 'r1': 0x800}


def pad(*names):
    m = 0
    for n in names:
        m |= PAD[n]
    return f'0x{0xFFFF & ~m:04X}'


def identify_overlay(head: bytes, ovl: dict[str, bytes]):
    for st, data in ovl.items():
        if data[:len(head)] == head:
            return st
    return None


def cmd_collect(a):
    work = Path(a.work); hits = work / 'hits'; hits.mkdir(parents=True, exist_ok=True)
    recomp = Path(a.recomp)
    ovl = overlay_payloads(Path(a.dat))
    saves = sorted((recomp / 'saves' / 'm3').glob('*.pst'))
    # Extra states (e.g. other sessions' slots 1-9) are only read: each is
    # copied into our own slot before loading.
    for g in a.extra:
        saves += sorted(Path(x) for x in __import__('glob').glob(g))
    if a.only:
        saves = [s for s in saves if any(k in s.name for k in a.only.split(','))]
    slot_file = recomp / 'saves' / 'openbios' / f'state_8001262C_slot{a.slot:02d}.pst'
    o = Oracle(a.port)
    o.call('turbo', enabled=1)
    for sv in saves:
        if sv.resolve() == slot_file.resolve():
            continue
        out = hits / f'{sv.stem}.txt'
        if out.exists() and not a.force:
            print('skip', sv.stem); continue
        shutil.copy2(sv, slot_file)
        r = o.call('savestate', op='load', slot=a.slot)
        if not r.get('ok'):
            print('load failed', sv.stem, r); continue
        o.wait_frames(2)
        o.call('clear_input')
        o.call('fn_hist_reset')
        segments = []          # (stage overlay seen, frames)
        idle, walk = a.frames * 2 // 3, a.frames - a.frames * 2 // 3

        def seg_dump(tag, st=None):
            # A segment's hits belong to the overlay that was loaded while they
            # ran, so a split passes the old overlay in.
            if st is None:
                st = identify_overlay(o.ram(OVL_BASE, 64), ovl)
            stage = o.ram(0x800C1B60, 2)
            path = hits / f'{sv.stem}{tag}.txt'
            r = o.call('fn_hist_dump', path=str(path))
            meta = {'save': sv.stem, 'overlay': st, 'stage': stage[0], 'area': stage[1], 'resp': r}
            path.with_suffix('.json').write_text(json.dumps(meta))
            o.call('fn_hist_reset')
            return st

        # Idle part, split whenever the loaded overlay changes (events load stages).
        st0 = identify_overlay(o.ram(OVL_BASE, 64), ovl)
        done, seg = 0, 0
        while done < idle:
            step = min(60, idle - done); o.wait_frames(step); done += step
            st = identify_overlay(o.ram(OVL_BASE, 64), ovl)
            if st != st0:
                seg_dump(f'.seg{seg}', st0); seg += 1; st0 = st
        # Walk part: forward + Circle every 40 frames, turn every 400.
        done = 0
        while done < walk:
            keys = ['up'] + (['circle'] if (done // 20) % 2 else []) + (['right'] if (done // 400) % 3 == 2 else [])
            o.call('set_input', buttons=pad(*keys))
            o.wait_frames(20); done += 20
            st = identify_overlay(o.ram(OVL_BASE, 64), ovl)
            if st != st0:
                o.call('clear_input'); seg_dump(f'.seg{seg}', st0); seg += 1; st0 = st
        o.call('clear_input')
        st = seg_dump('')
        print(sv.stem, 'overlay', st, 'segments', seg + 1)
    slot_file.unlink(missing_ok=True)


def read_hits(work: Path):
    """[(overlay stage or None, save, {addr: (entry_hits, call_hits)})]."""
    runs = []
    for meta_p in sorted((work / 'hits').glob('*.json')):
        meta = json.loads(meta_p.read_text())
        txt = meta_p.with_suffix('.txt')
        if not txt.exists():
            continue
        h = collections.defaultdict(lambda: [0, 0])
        for ln in txt.read_text().split('\n'):
            p = ln.split()
            if len(p) == 3:
                h[int(p[1], 16)][int(p[0])] += int(p[2])
        runs.append((meta.get('overlay'), meta['save'], h))
    return runs


# ----------------------------------------------------------------- step 4: report

def s16(x):
    return x - 0x10000 if x & 0x8000 else x


def analyse(img: Image, f, all_images) -> tuple[str, str | None]:
    """(one-line CANDIDATE guess, library/BIOS tag or None) from the function's own words."""
    lo, hi, _ = f
    ws = img.words(lo, hi)
    if not ws:
        return 'no bytes', None
    w = [x for _, x in ws]
    # BIOS call stub: li t2,0xA0/B0/C0 ; jr t2 ; li t1,N
    for i in range(len(w) - 2):
        if (w[i] & 0xFFFFFF00) == 0x240A0000 and w[i] & 0xFF in (0xA0, 0xB0, 0xC0) and w[i + 1] == 0x01400008 \
                and (w[i + 2] & 0xFFFF0000) == 0x24090000:
            t = f'{w[i] & 0xFF:X}0'[:2]
            n = w[i + 2] & 0xFFFF
            return f'BIOS {t}:{n:02X} {BIOS_NAMES.get((t, n), "")}'.strip(), 'BIOS'
    tags, notes = set(), []
    hw = set()
    lui = {}
    callees = []
    gte = 0
    for addr, x in ws:
        op = x >> 26
        if op == 0x0F:  # lui
            lui[(x >> 16) & 31] = (x & 0xFFFF) << 16
        elif op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B, 0x09, 0x0D, 0x32, 0x3A):
            rs = (x >> 21) & 31
            if rs in lui:
                v = (lui[rs] + (s16(x & 0xFFFF) if op != 0x0D else (x & 0xFFFF))) & 0xFFFFFFFF
                if 0x1F801000 <= v < 0x1F802000:
                    for a0, a1, nm in HW_RANGES:
                        if 0x1F800000 + a0 <= v < 0x1F800000 + a1:
                            hw.add(nm)
                for a0, a1, nm in KNOWN_DATA:
                    if a0 <= v < a1:
                        tags.add(nm)
        elif op == 0x03:
            callees.append(((addr + 4) & 0xF0000000) | ((x & 0x3FFFFFF) << 2))
        if op in (0x12, 0x32, 0x3A):
            gte += 1
    if hw:
        notes.append('hardware: ' + ', '.join(sorted(hw)))
    if gte >= 3:
        notes.append(f'GTE math ({gte} COP2 ops)')
    known = [KNOWN_FUNCS[c] for c in callees if c in KNOWN_FUNCS]
    bios = []
    for c in callees:
        for im in all_images:
            if im.contains(c) and c in im.stub_names:
                bios.append(im.stub_names[c])
    if tags:
        notes.append('uses ' + ', '.join(sorted(tags)))
    if known:
        notes.append('calls ' + ', '.join(sorted(set(known))))
    if bios:
        notes.append('calls ' + ', '.join(sorted(set(bios))[:3]))
    size = hi - lo + 1
    if not notes:
        notes.append('leaf' if not callees else f'{len(set(callees))} callees')
    notes.append(f'{size} B')
    lib = None
    if hw:
        lib = 'library (hardware driver)'
    elif gte >= 3 and len(set(callees)) == 0:
        lib = 'library? (GTE leaf)'
    return '; '.join(notes), lib


def attach_stub_names(img: Image):
    img.stub_names = {}
    for f in img.funcs:
        ws = [x for _, x in img.words(f[0], min(f[1], f[0] + 15))]
        if len(ws) >= 3 and (ws[0] & 0xFFFFFF00) == 0x240A0000 and ws[1] == 0x01400008:
            t = f'{ws[0] & 0xFF:X}0'[:2]; n = ws[2] & 0xFFFF
            img.stub_names[f[0]] = BIOS_NAMES.get((t, n), f'BIOS {t}:{n:02X}')


def cmd_report(a):
    work = Path(a.work)
    funcs_dir = work / 'funcs'
    load, exe_data = exe_image(Path(a.exe))
    ovl = overlay_payloads(Path(a.dat))
    exe_lists = [load_funcs(funcs_dir / 'EXE.csv')]
    if (funcs_dir / 'EXE_psx.csv').exists():   # PsyQ-signature project: library names and starts
        exe_lists.append([f for f in load_funcs(funcs_dir / 'EXE_psx.csv') if EXE_LO <= f[0] < EXE_HI])
    exe = Image('EXE', load, exe_data, merge_funcs(*exe_lists))
    syms = load_syms(Path(a.syms) if a.syms else None)
    lib_start = min((f[0] for f in exe.funcs if f[2] == '__main'), default=None)
    imgs = {st: Image(st, OVL_BASE, ovl[st], load_funcs(funcs_dir / f'{st}.csv')) for st in STAGES}
    for im in [exe, *imgs.values()]:
        attach_stub_names(im)
    cited_exe, cited_ovl = collect_citations()

    def covered(img, cites):
        cov = set()
        for c in cites:
            f = img.find(c)
            if f:
                cov.add(f[0])
        return cov

    cov_exe = covered(exe, cited_exe)
    cov_ovl = {st: covered(imgs[st], cited_ovl.get(st, ())) for st in STAGES}
    # Unmerged work: functions cited only on other local branches get a note,
    # not a pass (main is what ships).
    branches = a.branch
    if branches is None:
        cur = subprocess.run(['git', '-C', str(REPO), 'branch', '--show-current'],
                             capture_output=True, text=True).stdout.strip()
        heads = subprocess.run(['git', '-C', str(REPO), 'for-each-ref', '--format=%(refname:short)', 'refs/heads'],
                               capture_output=True, text=True).stdout.split()
        branches = [h for h in heads if h not in ('main', cur)]
    on_branch = collections.defaultdict(set)   # (image key, func start) -> branches
    for br in branches:
        be, bo = collect_citations(br)
        for f0 in covered(exe, be) - cov_exe:
            on_branch[('EXE', f0)].add(br)
        for st in STAGES:
            for f0 in covered(imgs[st], bo.get(st, ())) - cov_ovl[st]:
                on_branch[(st, f0)].add(br)

    # Static call graph (jal targets) for caller context in the guesses.
    callers = collections.defaultdict(set)   # (key, target func start) -> {(key, caller start)}
    for key, img in [('EXE', exe)] + [(st, imgs[st]) for st in STAGES]:
        for f in img.funcs:
            for addr, x in img.words(f[0], f[1]):
                if x >> 26 == 0x03:
                    t = ((addr + 4) & 0xF0000000) | ((x & 0x3FFFFFF) << 2)
                    if exe.contains(t):
                        tf = exe.find(t); tk = 'EXE'
                    elif img is not exe and img.contains(t):
                        tf = img.find(t); tk = key
                    else:
                        continue
                    if tf:
                        callers[(tk, tf[0])].add((key, f[0]))

    def label(key, start):
        img = exe if key == 'EXE' else imgs[key]
        f = img.find(start)
        if key == 'EXE' and start in syms:
            return f'`{syms[start]}`'
        if f and not f[2].startswith('FUN_'):
            return f'`{f[2]}`'
        if start in (cov_exe if key == 'EXE' else cov_ovl[key]):
            return 'ported'
        return None

    def caller_note(key, start):
        cs = callers.get((key, start), set())
        if not cs:
            return 'no static caller (pointer-called)'
        named = [(k, c, label(k, c)) for k, c in sorted(cs)]
        named.sort(key=lambda t: (t[2] is None, t[1]))
        parts = [f'0x{c:08X}' + (f' {lb}' if lb else '') + ('' if k in ('EXE', key) else f' [{k}]')
                 for k, c, lb in named[:2]]
        more = f' +{len(cs) - 2}' if len(cs) > 2 else ''
        return 'called by ' + ', '.join(parts) + more

    runs = read_hits(work)
    # per image: func start -> [count, entered_at_start, saves]
    ex = {k: collections.defaultdict(lambda: [0, False, set()]) for k in ['EXE', *STAGES]}
    outside = {k: collections.Counter() for k in ['EXE', *STAGES]}
    other = collections.Counter()
    saves_by_stage = collections.defaultdict(set)
    for st, save, h in runs:
        if st:
            saves_by_stage[st].add(save)
        for addr, (ent, cal) in h.items():
            n = max(ent, cal)
            if exe.contains(addr):
                im, key = exe, 'EXE'
            elif st and imgs[st].contains(addr):
                im, key = imgs[st], st
            elif OVL_BASE <= addr < OVL_BASE + 0x80000:
                other['overlay (unidentified)'] += n; continue
            else:
                other['BIOS/kernel' if (addr & 0x1FFFFFFF) < 0x10000 or addr >= 0xBFC00000 else 'other RAM'] += n
                continue
            f = im.find(addr)
            if not f:
                outside[key][addr] += n; continue
            e = ex[key][f[0]]
            if addr == f[0]:
                e[0] += n; e[1] = True
            elif not e[1]:
                e[0] = max(e[0], n)
            e[2].add(save)

    def rows(key, img, cov):
        out = []
        for start, (n, at_start, saves) in ex[key].items():
            if start in cov:
                continue
            f = img.find(start)
            guess, lib = analyse(img, f, [exe, *imgs.values()])
            if key == 'EXE' and lib_start and start >= lib_start:
                lib = 'PsyQ library' + (f' `{f[2]}`' if not f[2].startswith('FUN_') else ' (unnamed)')
            elif key == 'EXE' and not f[2].startswith('FUN_') and f[2] not in ('main', 'default'):
                lib = lib or f'PsyQ `{f[2]}`'
            if not lib:
                guess = guess + '; ' + caller_note(key, start)
            if key == 'EXE' and start in syms:
                guess = f'MML1x10 name `{syms[start]}`; ' + guess
            out.append((n, start, f, at_start, saves, guess, lib, sorted(on_branch.get((key, start), ()))))
        out.sort(key=lambda r: (-r[0], r[1]))
        return out

    L = []
    W = L.append
    W('# Function coverage: original functions the 1.0 route runs vs. what the port cites')
    W('')
    W(f'Generated by `tools/function_coverage.py report` on {time.strftime("%Y-%m-%d")}. '
      'Addresses, counts and guesses only; no retail bytes. Every "guess" is CANDIDATE '
      '(heuristics over the function\'s own instructions: hardware registers, GTE ops, known '
      'globals and known callees), not a reading of the code.')
    W('')
    W('## How it was made')
    W('')
    W('- **All functions:** Ghidra 12.1.4. ROCK_NEO.EXE from the saved, pre-analysed project '
      '(`ROCK_NEO_mcp`); each overlay `R3_STxx.BIN` imported raw at 0x80100000 and seeded with '
      'prologues after `jr ra` plus data words pointing at prologues, then auto-analysed.')
    W('- **Ported:** every `0x8xxxxxxx` in `src/ include/ platform/3ds/source/ tools/ tests/` '
      '(git-tracked). An address inside a function covers it. Overlay addresses go to the stage '
      'named on or near the line, else the file\'s main stage, else ST00.')
    W('- **Executed:** psxrecomp oracle (US disc) with the debug-only per-function counter '
      '(`tools/psxrecomp_fn_hist.patch`): compiled-function entries plus psx_dispatch and '
      'interpreter JAL/JALR targets. Each M3 savestate (`saves/m3/*.pst`) was loaded and run for '
      f'{a.frames_note} frames: two thirds idle (the event in progress plays on), one third walking '
      'forward with Circle pressed every 40 frames and a turn every 400. The overlay was identified '
      'by its first 64 bytes; a run that changed stage was split. Counts are calls during those '
      'runs; overlay counts may include a few duplicates (an interpreted call into a compiled '
      'overlay function counts twice).')
    W('- **Extra states:** the oracle\'s savestate slots 1–10 (other sessions\' scratch states, '
      'mostly the M1 tutorial and the tutorial boss) were also replayed, read-only via a copy in '
      'slot 11; they give the ST00 rows. "Runs" counts savestate runs (segments) with that image loaded.')
    W('- **Names:** PsyQ library names come from the older `ROCK_NEO_psx` Ghidra project (ghidra_psx_ldr '
      'signatures); everything from `__main` (0x80067FF8) up is PsyQ SDK code. Game-function names '
      'marked "MML1x10" come from the public WIP decompilation '
      '[MML1x10](https://github.com/JoshuaDav10/MML1x10) (`config/syms.us.rock_neo.txt`, 337 symbols, '
      'not yet matching/shiftable), so a public decompilation exists but covers few functions.')
    W('- **Not covered by the oracle runs:** the title/menu/new-game path, ST00 areas no slot state '
      'sits in, rooms the M3 saves only pass through (teleport shortcuts, see M3_ROADMAP.md), and the '
      'Tron fight beyond its start. A function that runs only there cannot show as executed here, '
      'so "not executed" is not proof of "unused in 1.0".')
    W('')
    W('## Summary')
    W('')
    W('| Image | Ghidra functions | cited on main | executed in oracle runs | executed, not cited on main | of those, cited on an unmerged branch | runs |')
    W('|---|---:|---:|---:|---:|---:|---|')
    tables = {}
    for key, img, cov in [('EXE', exe, cov_exe)] + [(st, imgs[st], cov_ovl[st]) for st in STAGES]:
        r = rows(key, img, cov)
        tables[key] = r
        nsv = len(saves_by_stage.get(key, ())) if key != 'EXE' else len(runs)
        W(f'| {key}{" — " + STAGE_NOTE[key] if key in STAGE_NOTE else " (ROCK_NEO.EXE)"} | {len(img.funcs)} | {len(cov)} '
          f'| {len(ex[key])} | {len(r)} | {sum(1 for x in r if x[7])} | {nsv} |')
    W('')
    if other:
        W('Calls outside ROCK_NEO.EXE and the identified overlay: ' +
          ', '.join(f'{k} {v:,}' for k, v in other.most_common()) + '.')
        W('')

    def table(key, r, cap):
        img = exe if key == 'EXE' else imgs[key]
        lib_rows = [x for x in r if x[6]]
        game_rows = [x for x in r if not x[6]]
        W(f'Executed but not cited: {len(r)} ({len(game_rows)} game code, {len(lib_rows)} library/BIOS CANDIDATE).')
        W('')
        W('| # | Function | Calls | Runs | Guess (CANDIDATE) | Cited on unmerged branch |')
        W('|---:|---|---:|---:|---|---|')
        for i, (n, start, f, at_start, saves, guess, lib, brs) in enumerate(game_rows[:cap], 1):
            nm = '' if f[2].startswith('FUN_') else f' ({f[2]})'
            W(f'| {i} | `0x{start:08X}`{nm} | {n:,}{"" if at_start else "*"} | {len(saves)} | {guess} | {", ".join(brs)} |')
        if len(game_rows) > cap:
            W(f'| | … {len(game_rows) - cap} more in the full CSV | | | | |')
        W('')
        if lib_rows:
            W('Library / BIOS (CANDIDATE: BIOS call stubs, hardware-register drivers, GTE leaves):')
            W('')
            W('| Function | Kind | Calls | Guess |')
            W('|---|---|---:|---|')
            for n, start, f, at_start, saves, guess, lib, brs in lib_rows:
                W(f'| `0x{start:08X}` | {lib} | {n:,} | {guess} |')
            W('')
        if outside[key]:
            top = outside[key].most_common(15)
            W(f'Executed addresses outside every Ghidra function ({len(outside[key])}; possible missed '
              'function starts): ' + ', '.join(f'`0x{x:08X}` ({c:,})' for x, c in top) +
              (' …' if len(outside[key]) > 15 else '') + '.')
            W('')

    W('## ROCK_NEO.EXE (shared by every stage)')
    W('')
    table('EXE', tables['EXE'], a.cap)
    for st in STAGES:
        W(f'## {st} — {STAGE_NOTE[st]}')
        W('')
        if not saves_by_stage.get(st):
            W('No oracle run had this overlay loaded, so nothing is marked executed. '
              f'Ghidra functions: {len(imgs[st].funcs)}, cited: {len(cov_ovl[st])}.')
            W('')
            continue
        W('Saves with this overlay loaded: ' + ', '.join(f'`{s}`' for s in sorted(saves_by_stage[st])) + '.')
        W('')
        table(st, tables[st], a.cap)
    W('`*` = no hit at the function start (entered only through a split piece or a jump into its '
      'body); the count is the largest hit inside it.')
    W('')
    out = Path(a.out)
    out.write_text('\n'.join(L) + '\n')
    full = work / 'function_coverage_full.csv'
    with full.open('w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['image', 'start', 'end', 'calls', 'runs', 'library', 'guess', 'cited_on_branches'])
        for key, r in tables.items():
            for n, start, fn, at_start, saves, guess, lib, brs in r:
                w.writerow([key, f'0x{start:08X}', f'0x{fn[1]:08X}', n, len(saves), lib or '', guess, ' '.join(brs)])
    print('wrote', out, 'and', full)
    for key, r in tables.items():
        print(key, 'executed-not-cited', len(r))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='step', required=True)
    g = sub.add_parser('ghidra')
    g.add_argument('--work', required=True); g.add_argument('--dat', required=True)
    g.add_argument('--exe-project', required=True); g.add_argument('--exe-name', default='ROCK_NEO_mcp')
    g.add_argument('--psx-name', default='ROCK_NEO_psx')
    g.add_argument('--ghidra', default=str(Path.home() / 'Downloads/ghidra_12.1.4_PUBLIC'))
    g.add_argument('--java-home', default='/Library/Java/JavaVirtualMachines/ibm-semeru-open-21.jdk/Contents/Home')
    c = sub.add_parser('collect')
    c.add_argument('--work', required=True); c.add_argument('--dat', required=True)
    c.add_argument('--recomp', required=True); c.add_argument('--port', type=int, default=4370)
    c.add_argument('--slot', type=int, default=11); c.add_argument('--frames', type=int, default=3000)
    c.add_argument('--only', default=''); c.add_argument('--force', action='store_true')
    c.add_argument('--extra', action='append', default=[], help='glob of extra .pst files (read only)')
    r = sub.add_parser('report')
    r.add_argument('--work', required=True); r.add_argument('--dat', required=True)
    r.add_argument('--exe', required=True)
    r.add_argument('--out', default=str(REPO / 'docs/validation_01516/FUNCTION_COVERAGE.md'))
    r.add_argument('--cap', type=int, default=60)
    r.add_argument('--syms', default='', help='MML1x10 syms.us.rock_neo.txt (optional, local copy)')
    r.add_argument('--branch', action='append', default=None,
                   help='branch whose citations count as "unmerged" (default: every local branch but main)')
    r.add_argument('--frames-note', default='3000')
    a = ap.parse_args()
    {'ghidra': cmd_ghidra, 'collect': cmd_collect, 'report': cmd_report}[a.step](a)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Minimal client for the psxrecomp debug server (port 4370).

Usage: psxrecomp_dbg.py <cmd> [key=value ...]
  psxrecomp_dbg.py frame
  psxrecomp_dbg.py fntrace_arm target=0x80040818
  psxrecomp_dbg.py fntrace_dump target_lo=0x80040818 target_hi=0x8004081C count=50
  psxrecomp_dbg.py read_ram addr=0x80040818 len=16

See docs/research_01516/PSXRECOMP_ORACLE.md. Traces come from the retail
game: keep their output out of the repo.
"""
import json
import socket
import sys


def call(cmd, host='127.0.0.1', port=4370, **kw):
    kw.update(id=1, cmd=cmd)
    with socket.create_connection((host, port)) as s:
        s.sendall((json.dumps(kw) + '\n').encode())
        return json.loads(s.makefile('rb').readline())


if __name__ == '__main__':
    args = {}
    for a in sys.argv[2:]:
        k, v = a.split('=', 1)
        args[k] = int(v) if v.isdigit() else v
    print(json.dumps(call(sys.argv[1], **args)))

#!/usr/bin/env python3
"""Minimal R3000 disassembler for an owner-supplied PS-X EXE address range.

Usage: psx_mips_dis.py ROCK_NEO.EXE 0x80050400 0x80050808
Reads no data beyond the requested range and prints nothing from the EXE except
decoded instructions, so verification output can be kept without retail bytes.
"""
import struct
import sys

R = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
     "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]
SPECIAL = {0x00: "sll", 0x02: "srl", 0x03: "sra", 0x04: "sllv", 0x06: "srlv", 0x07: "srav", 0x08: "jr",
           0x09: "jalr", 0x0C: "syscall", 0x0D: "break", 0x10: "mfhi", 0x12: "mflo", 0x18: "mult",
           0x19: "multu", 0x1A: "div", 0x1B: "divu", 0x20: "add", 0x21: "addu", 0x22: "sub", 0x23: "subu",
           0x24: "and", 0x25: "or", 0x26: "xor", 0x27: "nor", 0x2A: "slt", 0x2B: "sltu"}
IMM = {0x08: "addi", 0x09: "addiu", 0x0A: "slti", 0x0B: "sltiu", 0x0C: "andi", 0x0D: "ori", 0x0E: "xori"}
MEM = {0x20: "lb", 0x21: "lh", 0x23: "lw", 0x24: "lbu", 0x25: "lhu", 0x28: "sb", 0x29: "sh", 0x2B: "sw"}
BR = {0x04: "beq", 0x05: "bne", 0x06: "blez", 0x07: "bgtz"}


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def decode(pc, w):
    op, rs, rt, rd, sh, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
    imm = w & 0xFFFF
    if w == 0:
        return "nop"
    if op == 0:
        m = SPECIAL.get(fn, f"special_{fn:02x}")
        if fn in (0, 2, 3):
            return f"{m} {R[rd]},{R[rt]},{sh}"
        if fn == 8:
            return f"jr {R[rs]}"
        if fn in (0x10, 0x12):
            return f"{m} {R[rd]}"
        if fn in (0x18, 0x19, 0x1A, 0x1B):
            return f"{m} {R[rs]},{R[rt]}"
        return f"{m} {R[rd]},{R[rs]},{R[rt]}"
    if op == 1:
        m = {0: "bltz", 1: "bgez", 16: "bltzal", 17: "bgezal"}.get(rt, "regimm")
        return f"{m} {R[rs]},0x{pc + 4 + (s16(imm) << 2) & 0xFFFFFFFF:08x}"
    if op in (2, 3):
        return f"{'j' if op == 2 else 'jal'} 0x{(pc & 0xF0000000) | ((w & 0x3FFFFFF) << 2):08x}"
    if op in BR:
        t = (pc + 4 + (s16(imm) << 2)) & 0xFFFFFFFF
        return f"{BR[op]} {R[rs]},{R[rt]},0x{t:08x}" if op < 6 else f"{BR[op]} {R[rs]},0x{t:08x}"
    if op == 0x0F:
        return f"lui {R[rt]},0x{imm:04x}"
    if op in IMM:
        v = imm if op >= 0x0C else s16(imm)
        return f"{IMM[op]} {R[rt]},{R[rs]},{v}"
    if op in MEM:
        return f"{MEM[op]} {R[rt]},{s16(imm)}({R[rs]})"
    return f".word 0x{w:08x}"


def main():
    path, lo, hi = sys.argv[1], int(sys.argv[2], 0), int(sys.argv[3], 0)
    data = open(path, "rb").read()
    if data[:8] != b"PS-X EXE":
        sys.exit("not a PS-X EXE")
    load = struct.unpack_from("<I", data, 0x18)[0]
    for pc in range(lo, hi, 4):
        w = struct.unpack_from("<I", data, 0x800 + pc - load)[0]
        print(f"{pc:08x}  {decode(pc, w)}")


if __name__ == "__main__":
    main()

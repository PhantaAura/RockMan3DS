#!/usr/bin/env python3
"""Wrap a user-owned PS-X EXE text segment in a minimal ELF32/MIPS container.

This is an analysis convenience for tools such as llvm-objdump. It does not
translate or redistribute game code. The generated ELF contains bytes copied
from the input executable and therefore MUST remain a local research artifact.
Do not commit or distribute generated ELFs.
"""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

PSX_EXE_HEADER_SIZE = 0x800
ELF_HEADER_SIZE = 52
SECTION_HEADER_SIZE = 40
ELFCLASS32 = 1
ELFDATA2LSB = 1
EV_CURRENT = 1
ET_EXEC = 2
EM_MIPS = 8
SHN_UNDEF = 0
SHN_ABS = 0xFFF1
SHT_NULL = 0
SHT_PROGBITS = 1
SHT_STRTAB = 3
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4


def u32le(data: bytes, off: int) -> int:
    return struct.unpack_from('<I', data, off)[0]


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def parse_psx_exe(data: bytes) -> tuple[int, int, int]:
    if len(data) < PSX_EXE_HEADER_SIZE:
        raise ValueError('file is smaller than the 0x800-byte PS-X EXE header')
    if data[:8] != b'PS-X EXE':
        raise ValueError('missing PS-X EXE magic')

    pc0 = u32le(data, 0x10)
    text_addr = u32le(data, 0x18)
    text_size = u32le(data, 0x1C)
    if text_size == 0:
        raise ValueError('PS-X EXE declares an empty text segment')
    if PSX_EXE_HEADER_SIZE + text_size > len(data):
        raise ValueError(
            f'declared text size 0x{text_size:X} exceeds input payload '
            f'(file size 0x{len(data):X})'
        )
    return pc0, text_addr, text_size


def build_elf(data: bytes) -> tuple[bytes, int, int, int]:
    pc0, text_addr, text_size = parse_psx_exe(data)
    text = data[PSX_EXE_HEADER_SIZE:PSX_EXE_HEADER_SIZE + text_size]

    # Minimal section-only ELF. llvm-objdump does not require program headers
    # to disassemble an allocated executable .text section.
    shstr = b'\x00.text\x00.shstrtab\x00'
    sh_name_text = 1
    sh_name_shstr = 7

    text_off = align(ELF_HEADER_SIZE, 16)
    shstr_off = text_off + len(text)
    shoff = align(shstr_off + len(shstr), 4)

    ident = bytearray(16)
    ident[0:4] = b'\x7fELF'
    ident[4] = ELFCLASS32
    ident[5] = ELFDATA2LSB
    ident[6] = EV_CURRENT

    # e_flags is intentionally zero. This wrapper exists only to expose the
    # original little-endian MIPS instructions to analysis tools.
    ehdr = struct.pack(
        '<16sHHIIIIIHHHHHH',
        bytes(ident),
        ET_EXEC,
        EM_MIPS,
        EV_CURRENT,
        pc0,
        0,               # e_phoff
        shoff,
        0,               # e_flags
        ELF_HEADER_SIZE,
        0,               # e_phentsize
        0,               # e_phnum
        SECTION_HEADER_SIZE,
        3,               # null, .text, .shstrtab
        2,               # shstrtab index
    )

    null_sh = bytes(SECTION_HEADER_SIZE)
    text_sh = struct.pack(
        '<IIIIIIIIII',
        sh_name_text,
        SHT_PROGBITS,
        SHF_ALLOC | SHF_EXECINSTR,
        text_addr,
        text_off,
        len(text),
        0,
        0,
        4,
        0,
    )
    shstr_sh = struct.pack(
        '<IIIIIIIIII',
        sh_name_shstr,
        SHT_STRTAB,
        0,
        0,
        shstr_off,
        len(shstr),
        0,
        0,
        1,
        0,
    )

    out = bytearray()
    out += ehdr
    out += b'\x00' * (text_off - len(out))
    out += text
    out += shstr
    out += b'\x00' * (shoff - len(out))
    out += null_sh + text_sh + shstr_sh
    return bytes(out), pc0, text_addr, text_size


def main() -> int:
    p = argparse.ArgumentParser(
        description='Wrap a user-owned PS-X EXE text segment as ELF32/MIPS for local disassembly.'
    )
    p.add_argument('input', type=pathlib.Path, help='path to the user-owned PS-X EXE')
    p.add_argument('output', type=pathlib.Path, nargs='?', help='output ELF path (default: INPUT.elf)')
    p.add_argument('--force', action='store_true', help='overwrite an existing output')
    args = p.parse_args()

    output = args.output or args.input.with_suffix(args.input.suffix + '.elf')
    if output.exists() and not args.force:
        p.error(f'output already exists: {output} (use --force to replace it)')

    try:
        src = args.input.read_bytes()
        wrapped, pc0, text_addr, text_size = build_elf(src)
    except (OSError, ValueError) as exc:
        print(f'error: {exc}', file=sys.stderr)
        return 2

    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(wrapped)
    except OSError as exc:
        print(f'error writing {output}: {exc}', file=sys.stderr)
        return 2

    print(f'PS-X EXE: {args.input}')
    print(f'entry:     0x{pc0:08X}')
    print(f'text:      0x{text_addr:08X}..0x{text_addr + text_size:08X} ({text_size} bytes)')
    print(f'ELF:       {output}')
    print('WARNING: generated ELF contains bytes from the input game executable; keep it local.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

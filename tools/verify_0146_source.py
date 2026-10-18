#!/usr/bin/env python3
"""Verify the cumulative native foundation and absence of source-tree payloads."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_NAMES = {'ROCK_NEO.EXE', 'SLUS_006.03', 'SYSTEM.CNF'}
FORBIDDEN_SUFFIXES = {'.chd', '.bin', '.dat', '.cue', '.iso', '.img', '.rom', '.exe',
    '.xa', '.str', '.mcr', '.gme', '.vgs', '.sav', '.mmlpkg', '.3ds', '.3dsx',
    '.cia', '.elf', '.smdh'}


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def verify_source(root: Path = ROOT) -> dict:
    root = Path(root)
    errors = []
    checks = {}
    # 01443 intentionally initializes the full enhanced UV output. Pin that
    # revision while preserving the original 0143 fallback shader hash.
    for name, relative, expected in (
        ('fallback_shader_sha256', 'platform/3ds/source/vshader0143.v.pica',
         'fd242f3023a04cb03ba80ee20ea716daff2f9921f54502f5eeefa1d3a2b373bb'),
        ('uv_shader_sha256', 'platform/3ds/source/vshader_uv.v.pica',
         'ce29518b567b3bfc3fd891a0d098a68de1aa7f186da2ce5cd888c86251178e25')):
        path = root / relative
        checks[name] = sha(path) if path.is_file() else None
        if checks[name] != expected:
            errors.append(f'{relative}: frozen shader hash mismatch')

    def text(relative: str) -> str:
        path = root / relative
        if not path.is_file():
            errors.append(f'missing required source: {relative}')
            return ''
        return path.read_text(errors='replace')

    mk = text('platform/3ds/Makefile')
    targets = re.findall(r'^TARGET\s*:?=\s*([A-Za-z0-9_.-]+)\s*$', mk, re.MULTILINE)
    checks['native_target'] = targets[0] if len(targets) == 1 else None
    if len(targets) != 1 or not targets[0].startswith('Legends3DS_'):
        errors.append('3DS Makefile must declare one literal Legends3DS TARGET')
    for token in ['vshader0143.v.pica vshader_uv.v.pica', 'mml_renderer_recovery.c',
                  'ctr_texture_layout.cpp']:
        if token not in mk:
            errors.append(f'3DS Makefile missing {token}')
    main = text('platform/3ds/source/main.cpp')
    for token in ['MML_RENDER_0143_FALLBACK', 'choose_boot_stage',
                  'configure_terrain_attributes_0143', 'configure_terrain_attributes_uv',
                  'kBootLogPath', 'boot_log', 'C3D_TexFlush']:
        if token not in main:
            errors.append(f'cumulative native main missing {token}')
    cm = text('CMakeLists.txt')
    for token in ['src/render/ctr_texture_layout.cpp', 'add_library(mml3ds_push2 STATIC',
                  'add_executable(test_push2']:
        if token not in cm:
            errors.append(f'CMake missing {token}')
    for relative in [
        'include/legends3ds/ctr_texture_layout.h',
        'tools/l3cook_player_material_cache.cpp',
        'tools/l3flight_material_probe.cpp',
        'tools/l3flight_animation_probe.cpp',
        'include/legends3ds/p2/mml_renderer_recovery.h',
        'src/p2/mml_renderer_recovery.c',
        'include/legends3ds/p2/mml_runtime_diag_v2.h',
        'src/p2/mml_runtime_diag_v2.c']:
        if not (root / relative).is_file():
            errors.append(f'missing required source: {relative}')

    owner_hits = []
    symlinks = []
    for path in sorted(root.rglob('*')):
        relative = path.relative_to(root)
        if '.git' in relative.parts or '__pycache__' in relative.parts:
            continue
        if path.is_symlink():
            symlinks.append(relative.as_posix())
        elif path.is_file() and (path.name.upper() in FORBIDDEN_NAMES or
                path.suffix.lower() in FORBIDDEN_SUFFIXES or
                path.suffix.lower().startswith('.l3')):
            owner_hits.append(relative.as_posix())
    if owner_hits:
        errors.append('forbidden/cooked payloads present: ' + ', '.join(owner_hits[:20]))
    if symlinks:
        errors.append('source symlinks present: ' + ', '.join(symlinks[:20]))
    checks.update(owner_payload_hits=owner_hits, symlinks=symlinks,
                  status='PASS' if not errors else 'FAIL', errors=errors)
    return checks


def main() -> int:
    checks = verify_source()
    print(json.dumps(checks, indent=2, sort_keys=True))
    return 0 if checks['status'] == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(main())

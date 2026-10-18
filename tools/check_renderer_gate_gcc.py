#!/usr/bin/env python3
"""Compile-only regression for the historical unsigned-enum GCC warning.

The renderer source already contains the correct unsigned bounds check. The
negative control restores only the expression recorded in the historical log
inside a temporary copy, then requires genuine GCC to reject it under -Werror.
No source file is changed and no hardware/emulator execution is implied.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIXED = 'if(!g||(unsigned)s>(unsigned)MML_RENDER_ANIMATION)return false;'
HISTORICAL = 'if(!g||s<MML_RENDER_0143_FALLBACK||s>MML_RENDER_ANIMATION)return false;'


def run_check(compiler: Path, output_dir: Path) -> dict:
    compiler = compiler.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    version = subprocess.check_output([str(compiler), '--version'], text=True).strip()
    if 'clang' in version.lower() or 'gcc' not in version.splitlines()[0].lower():
        raise RuntimeError('This regression requires genuine arm-none-eabi-gcc, not a Clang alias')
    source = ROOT / 'src/p2/mml_renderer_recovery.c'
    text = source.read_text()
    if text.count(FIXED) != 1:
        raise RuntimeError('Expected the existing unsigned bounds expression exactly once')
    historical = output_dir / 'historical-type-limits-reconstruction.c'
    historical.write_text(text.replace(FIXED, HISTORICAL, 1))
    flags = ['-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-march=armv6k', '-mtune=mpcore', '-mfloat-abi=hard', '-mtp=soft',
             '-I' + str(ROOT / 'include/legends3ds/p2')]
    checks = []
    for label, path in [('historical-before', historical), ('current-after', source)]:
        command = [str(compiler)] + flags + ['-c', str(path), '-o', str(output_dir / (label + '.o'))]
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log = output_dir / (label + '.log')
        log.write_text(result.stdout)
        expected = (result.returncode != 0 and '-Werror=type-limits' in result.stdout) if label == 'historical-before' else (result.returncode == 0 and not result.stdout)
        checks.append({'label': label, 'command': command, 'exit_code': result.returncode,
                       'log': str(log), 'expected_result_met': expected})
    report = {'schema_version': 1, 'compiler': str(compiler), 'compiler_version': version,
              'source': str(source), 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'historical_source_kind': 'temporary reconstruction of the exact expression in work/3ds-patch.log',
              'historical_expression': HISTORICAL, 'existing_fixed_expression': FIXED,
              'warning': '-Wtype-limits', 'suppressions_used': False,
              'checks': checks, 'passed': all(check['expected_result_met'] for check in checks)}
    (output_dir / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    default_compiler = shutil.which('arm-none-eabi-gcc')
    if not default_compiler and os.environ.get('DEVKITARM'):
        default_compiler = str(Path(os.environ['DEVKITARM']) / 'bin/arm-none-eabi-gcc')
    if not default_compiler:
        default_compiler = '/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc'
    parser.add_argument('--compiler', default=default_compiler)
    parser.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    if args.output_dir:
        report = run_check(Path(args.compiler), args.output_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='l3-gcc-renderer-gate-') as directory:
            report = run_check(Path(args.compiler), Path(directory))
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

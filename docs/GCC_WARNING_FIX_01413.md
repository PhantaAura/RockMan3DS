# GCC warning guard, 0.0.14.13

The warning supported by the saved build evidence is `-Wtype-limits` in `mml_renderer_gate_set`. It is recorded in the existing `work/3ds-patch.log` for the 0.0.14.6a build:

```text
mml_renderer_recovery.c:11:13: warning: comparison is always false due to limited range of data type [-Wtype-limits]
if(!g||s<MML_RENDER_0143_FALLBACK||s>MML_RENDER_ANIMATION)return false;
```

`MML_RENDER_0143_FALLBACK` is zero. Native GCC chooses an unsigned representation for this enum, so the lower-bound comparison is always false. The inherited source already fixes the expression:

```c
if(!g||(unsigned)s>(unsigned)MML_RENDER_ANIMATION)return false;
```

This rejects out-of-range stages, including a negative stage cast to the enum, without an always-false comparison. The existing integrity test covers the negative-stage rejection. The source expression was preserved; this checkpoint adds a guard against reintroducing the warning.

## Permanent guard

`platform/3ds/Makefile` now adds `-Werror` specifically to `mml_renderer_recovery.o`. The native recipe already enables `-Wall -Wextra`; its actual compile command now treats this GCC diagnostic as an error. No warning is suppressed and no unrelated serializer is rewritten.

`tools/check_renderer_gate_gcc.py` provides a targeted compile-only regression using genuine `arm-none-eabi-gcc`. It compiles the unchanged current source with `-Wall -Wextra -Wpedantic -Werror` and the ARMv6K/mpcore flags. Its negative control temporarily restores the exact historical bounds expression in a copy of the same source and requires rejection with `-Werror=type-limits`. This historical expression reconstruction is explicitly distinct from an untouched-baseline failure: the current baseline was already fixed. The script neither changes project source nor executes a hardware binary.

Run from the source root:

```sh
python3 -B tools/check_renderer_gate_gcc.py \
  --compiler /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc \
  --output-dir docs/validation_01413/gcc-warning
```

Without `--output-dir`, the script uses and removes a temporary directory, which suits CTest. It rejects Clang aliases instead of mislabeling an AppleClang check as GCC.

## Actual validation

Installed compiler: devkitARM GCC 16.1.0. The historical expression reconstruction failed at the same comparison under `-Werror=type-limits`. The existing unsigned check compiled successfully with no output. A native Makefile dry run separately confirmed `-Werror` appears in the renderer-gate compile recipe. These are focused compiler checks, not a full native link or hardware test.

Evidence and exact command arrays:

- `validation_01413/gcc-warning/results.json`
- `validation_01413/gcc-warning/historical-before.log`
- `validation_01413/gcc-warning/current-after.log`
- `validation_01413/gcc-warning/native-make-guard.json`
- `validation_01413/gcc-warning/native-make-guard.log`

The suspected initializer-list magic-byte warning was not found in the saved host validation logs and did not reproduce in the targeted GCC 16.1 serializer check. This work makes no claim to have reproduced or fixed that separate hypothesis.

# 0.0.15.10 — matrix half of 0x80037FB4 and original lock-on 0x80034EB8

Status: **SOURCE_BACKED_PARTIAL**. Owner EXE SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
Evidence came from the R3000 instructions (`docs/research_01503/psx_mips_dis.py`)
and the Ghidra export. No retail values are compiled in or committed.

## 0x80037FB4, matrix half

The code at `0x80037FB4..0x80038198`:

1. Stores the rotation `[(-pitch)&0xFFF, yaw&0xFFF, 0]` at `0x800A3A10`.
2. Calls `jal 0x8007B838` (Ghidra calls it RotMatrixYXZ) into `0x1F8000C0`.
   This is the same address that the direct-eye producer `0x800384F0` calls
   (RotMatrixYX in 01425), so with z = 0 the two are the same function. The
   existing native `source_rot_matrix_yx_transposed_8007b838` is reused.
3. Calls `jal 0x8007B568` (TransposeMatrix) into `0x1F8000E0`.
4. Builds the translation vector. `s = byte 0x1F800074`, and every product is
   `mult`/`mflo`/`sra 12`, so it keeps the low 32 bits and then shifts
   arithmetically:
   - `vx = (-tx << s) + (((d<<s)*rsin(yaw) >> 12) * rcos(pitch) >> 12)`
   - `vy = (-ty << s) + ((d<<s)*rsin(pitch) >> 12)`
   - `vz = (-tz << s) + (((d<<s)*rcos(yaw) >> 12) * rcos(pitch) >> 12)`

   `sllv` takes only the low five bits of the shift.
5. Calls `jal 0x8007A9B8` (the split-vector ApplyMatrixLV) on `0x1F8000E0` and
   the vector, writing to `0x1F8000F4`. The existing native helper is reused.
   It now takes raw vector bits, and `0x800384F0` does its own eye negation.
6. Copies 8 words from `0x1F8000E0..0x1F8000FC` to `0x1F800008..0x1F800024`
   (`0x8003814C..0x80038198`).

The eye tail (`0x8003819C..`) is unchanged from 0.0.15.9, except that it now
uses the correct trig table (next section). The trailing ApplyMatrixLV, which
writes the camera-space eye to `0x1F800030`, is not ported because nothing on
this path reads it.

## Two trig tables (fix to 0.0.15.9)

`rsin 0x8007A060` and `rcos 0x8007A12C` read a quarter-wave halfword table at
`0x80092348` (1025 entries, indexes 0..1024). `0x8007B838` reads the packed
sin|cos table at `0x80092EC4`. They are different tables: they disagree by up
to ±7 at 1944 of the 4096 angles. 0.0.15.9 used the packed table for the eye,
which was not faithful.

- L3RF version 2 (`tools/cook_reference_tables.py`) appends the quarter table.
  Version 1 files still load.
- `reference_rsin_8007a060` and `reference_rcos_8007a12c` follow the original
  branches, including the negative-angle paths.
- An L3RF v1 pack has no quarter table. The camera then falls back to the
  packed table and counts each such frame in `packed_trig_substitutions`; the
  lock-on trace reports the count. This fallback is not source behaviour.

## Scratch inputs

- **Shift `0x1F800074`.** The only writers are:
  - the boot store at `0x8001326C`: 0;
  - the area load at `0x8001F4C0`: 0 while the event byte `0x800C4C48` is
    clear;
  - scene-camera commands 0x20/0x21/0x22 (`0x8003697C`, `0x800369C0`,
    `0x80036A1C`): 0, 1 and 2.

  The session passes the phase-6 script value when one is known, and 0
  otherwise.
- **Pad `0x1F8000F2`.** Transpose writes 18 bytes (`0x1F8000E0..F1`) and
  ApplyMatrixLV writes from `0x1F8000F4`, so the halfword at `F2` is whatever an
  earlier scratch user left there. `0x8004B6AC` and others store to it. The
  pad ends up in the high half of word 4. No consumer can see it:
  - CTC2 R4 keeps only RT33, the low half (`0x80035508`, see
    `source_player_proxy.cpp`);
  - libgte MATRIX users never read the pad.

  The native camera keeps the pad (starting at 0) and labels its value UNKNOWN.
  A test shows that pad 0 and pad 0xFFFF give the same lock-on result.

## Differential

`docs/research_01510/gameplay_camera_matrix_probe.py` runs the unchanged
`0x80037FB4` and its callees with the 01425 CPU and restricted MVMVA adapter:
640 authored states, 443,689 instructions and 2,560 MVMVA. Both trig tables are
authored, so no retail value enters a fixture. The states cover:

- 64 gameplay-shaped cases;
- signed 32-bit extremes;
- shifts 0..255;
- random pad values.

The native producer matches every recorded word: `0x800A3A10`,
`0x1F800008..24` and `0x800A39F0`. See
`tests/test_source_gameplay_camera_matrix.cpp`,
`tests/fixtures/gameplay_camera_*_01510.tsv` and
`docs/validation_01510/gameplay-camera-proof.json`.

## Lock-on 0x80034EB8 with this camera

**Projection controls.** The only EXE writers of OFX/OFY/H are InitGeom
`0x8007A4F0` (H 1000, DQA −4194, DQB `0x01400000`, OFX/OFY 0) and boot
`0x80011F48` (OFX/OFY 160/120, H 384). The ST00 overlay has neither a direct
CTC2 to controls 24..28 nor a call to a setter. The 3DS loop binds that pair
once at boot. DQA/DQB stay at their InitGeom values. Fog writers
(`0x8007A1CC` callers) change only IR0, which `0x80034EB8` does not read.

**Target mode GP+0x49E.** Here gp is `0x80097864`, so the byte is at
`0x80097D02`. No store to it with gp or an absolute base exists in the EXE or
the ST00 overlay. Its image value is cooked into L3TG v2 and labelled
CANDIDATE.

**Proxy gate.** The class 20/03 proxy (`0x80102308`) registers extent
`80>>2 | 0x8000`, which is mode 4, and `stack20 = 0`. The mode test
`(mode_byte << 4) & 0x80` passes with the image byte, and the record is not
skipped by flag `0x80`.

**Selection windows.** `0x80034EB8` projects the player point (depth + 50 is
the near limit) and every eligible proxy. It then picks the proxy closest to
x = 160 in two lanes:

- the broad lane (bit 0x20): depth below `0x500`, x in `0x83..0xBD`, y in
  `1..0xEF`;
- the window lane (bit 0x10): limits from the private L3TG window at
  `0x8008994C`.

With the follow camera (distance 0x200, pitch 0x30) and the player at the
origin facing −z, the test gives:

| Enemy point | Broad lane | Window lane |
|---|---|---|
| (0,−80,−400), straight ahead | selected | selected |
| (600,−80,−400), to the side | out (x) | out (x) |
| (0,−80,300), behind | out (depth ≤ player + 50) | out |
| (0,−80,−1200), far | out (depth ≥ 0x500) | out |
| ahead, mode byte 0x01 | not projected | not projected |

The player's own depth + 50 was 573.

**3DS loop.** The loop now calls `0x80034EB8` in the original order: lease
consume `0x80035508`, then selection, then `0x80035724`. The native Y edge
stands in for `+0x174` bit 0x10, and a gameplay-owned player stands in for
work+00 bit 0. Both are CANDIDATE, because their writers are untraced. The
Buster then spawns toward the selected proxy. Without L3TG v2 or the camera,
the 0.0.15.9 path (spawn with target −1, then lease consume) runs as before,
now with the camera words.

## Not established

- No paired PS1 trace of the live pad, shift or lock-on outcome.
- No hardware GTE parity: the adapter models MVMVA only.
- The writers of `+0x174` bits 0x20/0x10, of player work+00 bit 0 and of
  `0x80097D02` are untraced.
- Scripted camera modes, first-person and blends, and therefore their
  `0x1F800008` producers, are still not ported.

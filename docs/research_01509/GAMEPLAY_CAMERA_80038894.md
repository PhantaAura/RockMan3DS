# Original gameplay follow camera (0.0.15.9)

Evidence: USA `ROCK_NEO.EXE`. Functions were located with the Ghidra export and
checked against `psx_mips_dis.py` output; that covers `0x8003A154`, `0x8003A218`,
the eye tail of `0x80037FB4`, and `0x8002FE74`. Constants were read from the EXE.

## Ported — SOURCE_BACKED_PARTIAL
- **`0x80038648` + `0x800397E8`, init.** Target is (player X, 0, player Z);
  distance 0x200, pitch 0x30, yaw 0. Pull-in delay 0x3C, distance goal 0x200.
  The init command at `0x80097D58` is 1, which selects mode 0. The tail clears
  player `+0x44D` (`0x800B55FD`).
- **`0x80038894`, the per-frame update, ordinary path.** This needs state 1, no
  first-person mode, mode 0 and a settled blend.
  - Target is player X/Z at a fixed Y of −0x96.
  - Frame 1 snaps the yaw to heading + 0x800. After that the rate is
    `gap/4`, clamped to at least 0x18 while the gap is under 0x200, otherwise
    0x80.
  - `0x8003A154` steps the yaw toward the goal by that rate without overshoot
    and keeps the lag at most 0x200.
- **Distance.** Only while player `+0x44D` is 1 (the native running owner
  sets it in action 2 phase 4) does the update test the
  previous eye with `0x80049C54`. Bounds at `0x80097D60` are ±32.
  - Eye free: delay = 60 and goal += 10.
  - Eye blocked: the delay counts down, then goal −= 10.
  - With `+0x44D` clear: the delay counts down, then goal −= 10.
  - The goal is clamped to 0x200..0x300.
- **`0x80037FB4` eye.**
  `eye = (int16)-(((d*sin(yaw)>>12)*cos(pitch)>>12) - target)`, and the same
  for Y and Z.
- **`0x80049C54` with identical endpoints.**
  - An empty or absent cell at the point gives 0x40000000, which counts as
    blocked.
  - Candidates come from `0x8004B1E0` mode 1: empty cells included, and the
    gather stops at the grid edge.
  - Primitives are filtered on flag 0x8000 (`0x8002FE74(0xFFFF)` = 0xE0).
  - Y ranges use the `0x8004A060` default (bounds subtracted) or slopes 8..11.
  - Negative and empty cells use the band test.

## Not ported (held, never guessed)
- **Other `0x80038894` paths:**
  - first-person `0x800392CC`;
  - mode-switch blends;
  - modes 2–4 (actor tracking, re-centre);
  - script commands from `0x80039A60` beyond the init command;
  - shake `0x800A395F`.
- **Matrix half of `0x80037FB4`** (`RotMatrixYXZ`, transpose, translation into
  `0x1F800008`). This is still needed before lock-on `0x80034EB8` can be fed a
  camera.
- **`0x8004A060` cases:** codes 0x0C–0x0E, 0x10–0x13, 0x18–0x1B and the x4/x5
  diagonal slopes. The distance holds and a counter records each hit.
- **Timing:** the native init runs on the first tick after a reset or area
  change. The exact original frame of the init and first update is CANDIDATE.

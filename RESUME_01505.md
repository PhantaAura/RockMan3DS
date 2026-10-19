# Resume 0.0.15.5 (M2 — Oct 19 playtest regression)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.4.

## Completed
- Root-caused the Oct 19 Azahar fall near the first Area 1 door. The 3DS boot
  package never applied the original ST00 startup map initialization (994
  writes), so floors in IDX 62/78/123/138 were missing. Fixed with
  `apply_sidecar_map_initialization` (area_package) and its call in `main.cpp`.
- Added `tools/l3door_replay`, an owner-local recorded-input regression: FAIL
  without the fix, PASS with it.
- Evidence: `docs/validation_01505/RESULTS.md`.

## Next
- Owner: run `docs/validation_01505/AZAHAR_TEST.md` (door approach, then
  32 s / 44 s camera spots) and send `boot_01505.log` and `hardware_01505_*.csv`.
  The interactive check was blocked here by a locked screen.
- Port door events: `0x80050C2C` case 3 (button-press door, aux record →
  `0x800C3560` request) with `0x800514FC`, so the Area 1 → Area 2 door works.
- Also pack section 35 into `ST00.mmlpkg` in the build pipeline.

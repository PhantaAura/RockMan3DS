# Resume 0.0.15.4 (World Comes Alive M2c — boot fix)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.3.

## Completed
- `platform/3ds/source/main.cpp`: the default boot now checks the 0x80101274 Area 1
  handoff before the terrain-XYZ inspection entry. Through 0.0.15.3 the
  handoff was an `else` of that default-on renderer branch, so the enhanced
  build never used it and booted into the Area 0 inspection camera. Inspection
  remains the fallback (handoff area unavailable), for area cycling, and for
  `diagnostic_spawn_01414.txt`. Source-event boot is unchanged.

## Validation
See `docs/validation_01504/RESULTS.md`. Host 147/147, native PASS, and Azahar boots
to Area 1 at (2822,-1,-4833).

## Next
- Owner test in Azahar, `docs/validation_01504/AZAHAR_TEST.md`: Area 1 corners,
  diagonal walls, jump/land beside walls, enemy combat. Send back `boot_01504.log`
  and `hardware_01504_*.csv`.
- Collision work carries over from `RESUME_01503.md`: paired PS1 traces, code 2,
  and the x5 diagonal fallback.

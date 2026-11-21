# Resume 0.0.15.9 (WCA integration — original gameplay camera)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.8 (`41c2687`).

## Baseline (verified this pass)
- 149/149 host tests passed at 0.0.15.8.
- The Area 1 audit was clean and `l3door_replay` passed: the fall stays fixed
  and both doors open into Area 2.
- The 3DS view still used the native rigid follow camera, which is where the
  camera clipping came from.

## Completed
- The original gameplay follow camera is ported and drives the 3DS view in
  gameplay. See `src/world/source_gameplay_camera.cpp` and
  `docs/research_01509/GAMEPLAY_CAMERA_80038894.md`.
- `runtime_point_embedded_80049c54` is the original eye-in-geometry test.
- New test `test_source_gameplay_camera`.
- `l3edge_audit` reports the camera distance range and unsupported eye tests.

## Validation
See `docs/validation_01509/RESULTS.md`. Host 150/150, Area 1 audit clean,
camera distance 512..768, 0 unsupported eye tests, door replay PASS, native
build PASS. Azahar not run (in use for another project).

## World Comes Alive status
| Component | Status |
|---|---|
| Environment rendering | Partially implemented (renders; doors look closed, leaf `0x801126DC` UNKNOWN) |
| Player movement and collision | Partially implemented (walls, diagonals, deep overlap, history; slopes/steps/codes 2/0x0B/0x0C missing; no PS1 trace) |
| Camera | Partially implemented: original follow path ported 0.0.15.9; first-person, scripted modes and matrix not ported; not checked in Azahar |
| Buster combat | Partially implemented (source Buster/hit path; fire cadence owner unknown) |
| Enemy AI and defeat | Partially implemented (class 20/03 subset, defeat transaction) |
| Area transitions | Partially implemented (Area 1 → 2 door with event 0 walk-in, host-validated; other links unaudited) |
| Door visuals | Experimental (descriptor swap redraw; doors still look closed) |
| Original lock-on | Blocked (needs the `0x80037FB4` matrix half for `0x1F800008`) |
| Enemy rewards and pickups | Not implemented (reward queued; `0x80046B34` expansion missing) |
| Dialogue and facial expressions | Not implemented in gameplay flow |
| Opening cutscene | Not implemented (bypassed by default) |
| Natural tutorial progression | Not implemented beyond the first door |
| Old 3DS hardware | Not run |

## Next
1. Port the matrix half of `0x80037FB4` (`RotMatrixYXZ`, transpose,
   `ApplyMatrixLV` into `0x1F800008`). Verify it, then feed `0x80034EB8` to
   enable original lock-on.
2. Owner: run `docs/validation_01509/AZAHAR_TEST.md` (turning, running, doors).
3. Rewards: `0x80046B34` pickup expansion and collection.

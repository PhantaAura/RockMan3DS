# Resume 0.0.15.10 (WCA integration — original lock-on)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.9 (`553c95a`).

## Completed
- **Camera matrix.** The matrix half of `0x80037FB4` is ported:
  `source_gameplay_camera_80037fb4`, which uses RotMatrixYXZ `0x8007B838`
  (the same function as RotMatrixYX), transpose, the shifted translation,
  ApplyMatrixLV and the copy to `0x1F800008`. The words are recomputed on
  every camera update.
- **Differential.** `docs/research_01510/gameplay_camera_matrix_probe.py` ran
  640 authored states through the unchanged original code. The native port
  matches every word.
- **Trig fix to 0.0.15.9.** `rsin`/`rcos` read the quarter table at
  `0x80092348`, not the packed table at `0x80092EC4`; the two differ at 1944
  angles. L3RF v2 now carries both tables (v1 still loads, but the camera then
  counts `packed_trig_substitutions`).
- **Projection.** The 3DS player GTE is bound once from InitGeom `0x8007A4F0`
  plus `0x80011F48`.
- **Lock-on in the 3DS loop.** `0x80034EB8` now runs there: lease consume,
  selection, then the Buster `0x80035724`. Its window and target mode come from
  L3TG v2 (`player/target.l3tg`). Without them, the 0.0.15.9 path runs, now
  with the camera words.
- **Docs.** `docs/research_01510/GAMEPLAY_CAMERA_MATRIX_80037FB4.md`.

## Validation
See `docs/validation_01510/RESULTS.md`.

## World Comes Alive status
| Component | Status |
|---|---|
| Environment rendering | Partially implemented (renders; doors look closed, leaf `0x801126DC` UNKNOWN) |
| Player movement and collision | Partially implemented (walls, diagonals, deep overlap, history; slopes/steps/codes 2/0x0B/0x0C missing; no PS1 trace) |
| Camera | Partially implemented: follow path (0.0.15.9) plus its `0x1F800008` matrix (0.0.15.10, original-code differential); first-person and scripted modes not ported; not checked in Azahar |
| Buster combat | Partially implemented (source Buster/hit path; the Y edge stands in for `+0x174`; fire cadence owner unknown) |
| Original lock-on | Partially implemented: `0x80034EB8` gets the source camera and projection and selects the class 20/03 enemy ahead (host tests). CANDIDATE: work+00 bit 0, `+0x174` bit 0x10, target mode `0x80097D02` image byte. Not checked in Azahar |
| Enemy AI and defeat | Partially implemented (class 20/03 subset, defeat transaction) |
| Area transitions | Partially implemented (Area 1 → 2 door with event 0 walk-in, host-validated; other links unaudited) |
| Door visuals | Experimental (descriptor swap redraw; doors still look closed) |
| Enemy rewards and pickups | Not implemented (reward queued; `0x80046B34` expansion missing) |
| Dialogue and facial expressions | Not implemented in gameplay flow |
| Opening cutscene | Not implemented (bypassed by default) |
| Natural tutorial progression | Not implemented beyond the first door |
| Old 3DS hardware | Not run |

## Next
1. Owner: run `docs/validation_01510/AZAHAR_TEST.md` (lock-on on the tutorial
   enemy, plus the 0.0.15.9 camera checks).
2. Trace the writers of player work+00 bit 0, `+0x174` bits 0x20/0x10 (the
   animation callback) and `0x80097D02`, so the CANDIDATE lock-on inputs can be
   replaced.
3. Rewards: `0x80046B34` pickup expansion and collection.

## Blockers
- No PS1 trace of a lock-on outcome. The live pad `0x1F8000F2` is UNKNOWN, but
  no consumer can observe it.
- Physical Old 3DS testing has not been done.

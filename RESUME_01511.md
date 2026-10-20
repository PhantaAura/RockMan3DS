# Resume 0.0.15.11 (WCA integration — visible Buster shot)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.10 (`ec1ab97`).

## Completed
- **Diagnosis.** In Azahar 0.0.15.10, Y produced no visible shot. A host replay
  of the main.cpp chain at the Azahar spawn shows the shot was spawned
  (`0x80034EB8` → `0x80035724`, 11-frame `0x800433C0` life, release). The
  diagnostic ribbon drew shot +2C → +14. The original code copies +14 over
  +2C right after `0x8003430C` (`0x80043708..24`, R3000-verified), so the
  ribbon always had zero length. This has been true since 0.0.15.0; it is not a
  0.0.15.10 regression. The CSV `projectiles` column counts the retired
  candidate projectile, not source shots.
- **Fix.** `ChildRuntime::buster_registered_segments()` records what
  `0x8003430C` received. The ribbon draws it: Mega Man `+0x14` → endpoint,
  on 10 of the 11 frames. New traces: `live=`, `# source_buster_draw`.
- **Regression.** A `test_child_runtime` unit test (synthetic) and
  `l3door_replay` phase 4 (owner data: 3 presses → 3 shots, 30 drawn frames).
- **Docs.** `docs/research_01511/BUSTER_SHOT_PRESENTATION.md`.

## Validation
See `docs/validation_01511/RESULTS.md`.

## World Comes Alive status
| Component | Status |
|---|---|
| Environment rendering | Partially implemented (renders; doors look closed, leaf `0x801126DC` UNKNOWN) |
| Player movement and collision | Partially implemented (walls, diagonals, deep overlap, history; slopes/steps/codes 2/0x0B/0x0C missing; no PS1 trace) |
| Camera | Partially implemented: follow path (0.0.15.9) plus its `0x1F800008` matrix (0.0.15.10, original-code differential); first-person and scripted modes not ported; passed in Azahar 0.0.15.10 |
| Buster combat | Partially implemented: source Buster/hit path; shots are now drawn as a FRONTEND_DIAGNOSTIC ribbon of the registered segment (0.0.15.11; Azahar: 5/5 spawned and drawn per trace, visually a short sliver because the ray is seen end-on). CANDIDATE: forward-ray endpoint, and the Y edge for `+0x174`. Fire cadence and action `+9` owners UNKNOWN |
| Original lock-on | Partially implemented: `0x80034EB8` gets the source camera and projection and selects the class 20/03 enemy ahead (host tests). In Azahar 0.0.15.10 it runs every frame, but no enemy was ahead. CANDIDATE: work+00 bit 0, `+0x174` bit 0x10, target mode `0x80097D02` image byte |
| Enemy AI and defeat | Partially implemented (class 20/03 subset, defeat transaction) |
| Area transitions | Partially implemented (Area 1 → 2 door with event 0 walk-in, host-validated and passed in Azahar 0.0.15.10; other links unaudited) |
| Door visuals | Experimental (descriptor swap redraw; doors still look closed) |
| Enemy rewards and pickups | Not implemented (reward queued; `0x80046B34` expansion missing) |
| Dialogue and facial expressions | Not implemented in gameplay flow |
| Opening cutscene | Not implemented (bypassed by default) |
| Natural tutorial progression | Not implemented beyond the first door |
| Old 3DS hardware | Not run |

## Next
1. Find the writers of player `+0x400/+0x418` (the shot endpoint) and of
   `+0x174` bits 0x10/0x20 (fire cadence), so the forward ray and the Y edge
   can be replaced.
2. Lock-on in Azahar with an enemy straight ahead.
3. Rewards: `0x80046B34` pickup expansion and collection.

## Blockers
- The original shot sprite/effect renderer is not ported; the ribbon is
  diagnostic.
- Physical Old 3DS testing has not been done.

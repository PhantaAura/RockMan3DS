# Stage loading beyond ST00 (stage loading v1)

Branch `claude/stage-loading-9d47ea` (based on `claude/tutorial-boss-4a9127`,
which carries ST00 events 3 and 2). First non-ST00 stage: **ST02**, the stage
ST00 event 2 requests after the tutorial boss.

## The request (SOURCE_BACKED_PARTIAL)

ST00 event 2 (`0x8010194C`) ends at fade-busy 0 with the transition lanes
`0x800C356C = 2`, `356E = 2` (stage), `356F = 0` (area), `3560..3565 = 0`,
`356A = 0`, `356D = 0`, `3570/3571 = 0xFF` (AREA3_BOSS_SOURCE_SLICE.md).
The stage loader then reloads the overlay, map and area set; `0x800C1B60` /
`0x800C1B61` become 2 / 0, and the player is spawned through `0x80015EE8`
at the lanes: **(0,0,0), heading 0**. The ST02 area setup (`0x801000B0`,
below) writes no player field, so that is the arrival until ST02 event 4
(poll `0x600`, the Flutter flight) takes over. Oracle comparison of the
arrival frame is still pending: the shared oracle was in use by another
session during this work (it was in stage 3). The in-game main phase
`0x800C1B10/11 = 03 02` is observed (M3_ROADMAP.md); the intermediate
load phases are UNKNOWN and not modelled.

What survives the load (globals the loader does not touch): story flags,
life `+0x6E/+0xB0`, the frame count and the statistics selector.

## ST02 area setup tables (VERIFIED from the overlay's instructions)

`R3_ST02.BIN` (load `0x80100000`) has the same area setup shape as ST00's
`0x8010038C`, at `0x801000B0`, indexed by the signed area byte `0x800C1B61`
(no bound check):

| Table | ST00 | ST02 |
|---|---|---|
| resource → `0x80022FD4` | `0x8011A6F0` | `0x801068E0` |
| object list → `0x8002EA50` | `0x8011AB3C` | `0x80106940` |
| aux list → `0x1F800050` | `0x8011AC40` | `0x80106968` |
| 40-byte setup blob | `0x8011AC54` | `0x80106978` |
| selector (*8 into seeds, `0x800371A0`) | `0x8011ADE4` | `0x80106C18` |
| seed table | `0x8011AD1C` | `0x80106A18` |
| `0x8005DC10` argument | per area (`0x8011A0FC`) | 0 |

Each ST02 table has **four** rows (word 5 ends it: `0xFF`, `0xFFFF`,
`0x40600`); selectors are 0/0x10/0x20/0x30. All four object lists are empty
(ST02 is a scene stage; its cast comes from event 4), and all areas share one
empty aux list and one resource block. Event 4 visits area byte 3, which has
no `ST02_03.BIN`; the pack cooks area 3 from `ST02.BIN` (CANDIDATE).

## What the port does

- `src/flow/st00_flow.cpp`: `parse_stage_flow_us(payload, StageFlowAddresses)`
  with `stage_flow_addresses_us(0|2)`; L3SF carries 1..5 areas.
- `tools/build_tutorial_pack.py`: `EXTRA_STAGES` (one entry per stage) cooks
  map, render, map init, flow, per-area EBD and the terrain/player/actor
  material caches into `legends3ds/tutorial/stages/stNN/`.
- `source_stage_load.h`, `RuntimeSession::source_stage_request()`,
  `make_stage_carry()`, `apply_stage_load()`.
- `platform/3ds/source/main.cpp`: on a stage request the loop exits, the
  stage's resources are released, and the boot block reruns for the new stage
  (`stage_path()`), with the ST00-only owners (event tables, opening, title,
  triggers, messages, music, spawn catalog, negative materials) left unbound.

## Missing in ST02

Event 4 and its `0x600` poll (the whole visible content of the stage: the
Flutter, sea and crash), the ST02 BG sky (`ST02BG*.TIM`), the stage's
negative-material table, music/SFX, and the request to stage 3 area 1.
Without event 4 the player stands at (0,0,0) with control; the original
never gives control in ST02.

## Azahar evidence (2026-11-15, run 11)

Diagnostic boot (Area 3 boss spawn; the tutorial is skipped), then the real
post-win chain: 0x606 at frame 629, event 3 -> Area 4 at 1617, event 2 ->
stage request at 4419 (stage 2, area 0, lanes (0,0,0), heading 0), ST00
unloaded, ST02 loaded, `apply_stage_load` applied (area 0, (0,0,0), HP 20).
Terrain faces are drawn (743 at frame 4680). Mega Man then free-falls: ST02
area 0 has no floor for a standing player (y wraps through int16). Found on
the way: the fade budget byte 0x1F800001 = 2 (stage initialization,
0x8001201C/0x80013440) was only rebound on an area change, so fade 10 held
at its endpoint after any direct boot; now bound at boot.

ST02 event 4 cooks with `tools/cook_cutscene.py --event 4 --stage 2`
(MAX_RECORDS raised to 256): handler 0x80101CC0, phase table 0x80100004
(7), timeline 0x8010738C (37 records), camera 0x80106C30 (446 words), spawns
0x80107328..0x80107378 (5), XA 0x20000004, and 12 area requests from its
callbacks (areas 0/1/2/3). The handler has event 2's shape: player-ready
wait, XA, CD wait, 20-tick countdown, fade 2, constructor 0x80101B2C,
timeline until fade 10.

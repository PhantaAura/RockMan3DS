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

## ST02 event 4 records and area cuts (SOURCE_BACKED_PARTIAL)

The 36 record callbacks (0x80100450..0x801019DC) are ported in
`source_st00_post_win.cpp` (`run_st02_event4_callback`; the ST02 overlay
loads at 0x80100000 like ST00's, so 0x801014BC exists in both event 2 and
event 4 and the callbacks dispatch by event). Ported: the slot commands
(+0 bit 1 drawn, +9/+0x0A state, +0x0C, +0x0D, +0x1C..+0x24 positions,
+0x56 heading, +0x15E), the 0x8002EE90(64, 128) sounds and the area cuts.
Not ported (no native owner): the 0x800BBA90.. / 0x800BBC88.. globals, the
0x8010A5D4 object (+0x0D, +0x1E), the 0x80105884 / 0x80105724 / 0x80104448
effects and the 0x8010A5E0.. overlay bytes.

0x801002C8(area) is an **area cut inside the running event**: 356C = 0xFF,
356E = 2, 356F = area, 3560..3565 = 0, 356A = 0, 356D = 0, 3570 = 3571 = 0xFF.
The main phase (0x800163DC) sends any nonzero 356C to the load subphase 6;
with 356C = 0xFF the loader 0x8001F408 skips 0x8001D440, 0x8001F5E4,
0x8001D888, the 0x3FE sound and 0x80031AA4 (player spawn, music and camera
set-up) but still calls the area's set-up routine, and 0x80022FD4 skips the
area resources while an event runs. The timeline cuts 1, 0, 1, 0, 1, 0, 1,
0, 2, 1, 3, 1 (records 3, 13, 14, 15, 16, 21, 25, 30, 31, 32, 34, 35).

The port keeps the cut as a post-win hand-off with `event_cut()`: the event
waits while it is pending (the load subphase does not tick events), the
loader commit (`commit_source_post_win_transition`) switches the area with
the player and the fade work untouched, and main.cpp skips the music change.
A cut to the current area is kept (the original reloads it; ST02's areas own
no objects; CANDIDATE).

## ST02 event 4 actors: dispatch (VERIFIED from the instructions)

The object update 0x8001E070 takes the family from object byte [1]
(0x8002FE74) and dispatches through the stage's scratch tables, which the
stage init writes (ST02 0x80100054..0x801000A4, ST00 0x80100330..0x80100380):
family 0x40 -> `1F80005C[byte 2][byte 4]`, family 0x20 ->
`1F800058[byte 2][byte 4]` (byte 2 == 0x10 goes to 0x80031824 instead),
0x60 -> `1F800060[byte 2]`, 0xE0 -> `1F80006C[byte 2]`. Byte 2 is the
subtype (spawn record [1]); byte 4 is the variant (spawn record [2]).

| Table | ST00 | ST02 |
|---|---|---|
| 1F800058 (family 0x20) | 0x8011A4D0 | 0x801067A4 |
| 1F80005C (family 0x40) | 0x8011A5B0 | 0x80106844 |
| 1F800060 / 64 / 68 / 6C | 0x8011A5D4 / 0x8011A604 / 0x8011A644 / 0x8011A6CC | 0x80106864 / 0x80106868 / 0x8010688C / 0x801068A8 |

Event 4's five spawns all use variant 4: 40/05 -> 0x80103360,
40/06 -> 0x80103D18, 40/07 -> 0x80104154, 20/0A -> 0x80102888,
20/27 -> 0x80102990, all ST02 overlay code. Correction for ST00
(AREA3_BOSS_SOURCE_SLICE.md "fitted on 40/02"): the class-40 row is chosen
by the subtype and indexed by the variant; ST00 row 5 is
[0, 0x8010D824, 0x8010E500, 0x8010EC54], so event 3's 40/05 with record
[2] = 3 runs 0x8010EC54, and the opening's variant 1 runs 0x8010D824.

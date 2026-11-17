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

## ST02 event 4 actors: 20/0A and 20/27 (SOURCE_BACKED_PARTIAL)

Spawn records 0x80107328: slot 0 = 40/05, 1 = 40/06, 2 = 40/07, 3 = 20/0A
(record [4] = 1), 4 = 20/27 (record [8] = 1, so work +0x0C = 1). 0x8001EAE8
copies record [4] -> work +3, [2] -> +4, [8..11] -> +0x0C..+0x0F.

20/0A (0x80102888, the Flutter seen from outside, slot 3): +8 == 0 runs
0x80101FD8 (0x8002F9C4 null variant, +0x158 = 0x02000200, +0x15C =
0x21080200, +0x78 = 0, pose 0). Then 0x80102830 dispatches +9 through
0x80107504 and steps the sequence (0x80032538) and draws (0x800334C4).

| +9 | Handler / substates (+0x0A) | Effect |
|---|---|---|
| 0 | 0x80102040 | position shorts; an extra sequence step (two per update) |
| 1 | 0x801021D8 / 0x80102080, 0x801020B0, 0x801021D0 | 180 frames of soft-float adds (`__floatsidf`/`__adddf3`/`__fixdfsi`) to +0x1C, +0x44, +0x48, +0x4C; rotation shorts from the accumulators' high halves. Truncation drift leaves rot z at -43 (high half 0xFFFF). |
| 2 | 0x80102278 / 0x80102244, 0x80102258 | +0x24 += 0xFFF36886 every update |
| 3 | 0x80102338 / 0x801022CC, 0x801022F0 | 75 frames heading += 5; 0x8002FEA4(work, 256, 0, 0) every update |
| 4 | 0x80102458 / 0x80102394, 0x801023C0, 0x801023F4 | 45-frame yaw/roll ease; 0x80105724 smoke pair when (1F800002 & 3) == 3 |
| 5 | 0x801026E8 / 0x80102514, 0x80102538, 0x8010262C | climb (+0x24 += 0xA00000, +0x20 += 0xA0000); exhaust triples from count 70 (odd counts), 0x80105C28 at 90, then 50 frames |
| 6 | 0x801027A8 | +0x20 = rsin((int8)+0x0C * 64) * 800 bob |

20/27 (0x80102990, slot 4): +8 through 0x80107520: 0 = 0x801028C8
(rotation 0, +0x15C = 0x42100200), 1 = 0x80102914 (when (int8)+0x0C !=
+0x0D: 0x8002F9C4 with variant +0x0C, pose 0; then +0x0D = +0x0C), 2 =
0x80102970 (0x80031824 release).

Port: `source_st02_event4_actors.{h,cpp}`, run by
`RuntimeSession::update_source_st02_event4_actors` after the event step
(CANDIDATE order); 1F800002 is the frame counter (CANDIDATE phase). Effects
and model/pose requests are reported per update, not drawn yet. 40/05..07
are the next slice.

## ST02 event 4 actors: 40/05, 40/06, 40/07 (SOURCE_BACKED_PARTIAL)

All three resolve their model with a null variant and key their state
changes on byte 0 of the current pose record (`*(+0x98)`); 0x800322A8
stores the request at +0xAC and points +0x98 at that sequence's first
record, and 0x80032488 steps it. A key test therefore sees the record the
previous frame's step landed on.

- 40/05 (slot 0, 0x80103360; runs only when work +3 == 0): init 0x80102D54
  (pose 16, heading 0x800). +9 via 0x80107720: 0 = rise (+0x20 += 0x23333)
  counting frames on key 27 until 4, pose 17, wait key 7, rise 0x30000 until
  key 17; 1..8 = poses 18..25 once; 9 = pose 26, wait key 16, +0x24 +=
  0x268000 until key 24. Then, unless (int8)+0x0D (event 0x80100970 freeze),
  shorts and a step. 0x80102C44 runs the face cues (streams 0x80107644 /
  0x80107688, scripts 0x801075F0 / 0x80107614; not ported).
- 40/06 (slot 1, 0x80103D18): init 0x80103704 (no pose; +0x143 = +0x147 =
  0xFF; face streams 0x80107868 / 0x801078D0). +9 via 0x801079BC: 0/1 =
  pose 1, 2 = pose 7, 3 = key 0 -> pose 2, key 34 -> pose 4, 4 = key 0 ->
  pose 3, key 0 -> pose 1, 5..8 = poses 5..8, 9 = pose 9, key 18, +0x24 +=
  0x2C0000 until key 23. Step unless (int8)+0x0C (event 0x80100D78 freeze).
  Then 0x801035F4 (face) and 0x80103D8C, which writes event slot 4
  (`*(0x800C4C7C)`, the 20/27 prop): +0x0C = 1 and, by +0xAC, +0x58 =
  -0x801079E4[key] (1..3), -0x80107A26[key] (4), 318 (5), -63 (6),
  -0x80107A6C[key] below key 31 else -284 per frame with +0x0C = 0 (7),
  -318 (8), -318 with +0x0C = 0 from key 18 (9). The 20/27 then re-resolves
  its model when +0x0C changed. The table at 0x80107984 has no reference.
- 40/07 (slot 2, 0x80104154; work +3 == 0): init 0x80103F08 (no pose).
  0 = pose 1, 30 frames, then state 1; 1 = pose 2 until key 71, then state 0
  (an idle loop); 2 = pose 3, stepping.

The roll tables are cooked as an event-4 extra (`--extra 801079E4:198`).
The runtime binds no class-40 sequence data yet, so these actors' key tests
report SequenceUnavailable and wait; the drawing slice supplies it.

### Class-40 sequence binding (SOURCE_BACKED_PARTIAL)

0x800322A8(work, request, b) stores the request at +0xAC, +0x94 = the
request's sequence (+0x90 table), +0x98 = +0x94 + 8b, +0xA4 = that
record's first word; 0x80032488 counts +0xA5 down and at zero follows
record byte 3 (>= 0 next, -1 replay, else record & 0x7F), reloading +0xA4.
The port's `actor_sequence_step` implements this; `St02ActorBankSequence`
runs it over `decode_ebd_scene_actor40_animation_bank` for the first
(0x40, subtype) row of the stage EBD (0x8002F9C4, null variant), with the
requests the handlers issue. In the cooked ST02_01.EBD (same for every area):
40/05 = row 1 (18 parts), 40/06 = row 2 (18), 40/07 = row 3 (13), 20/0A =
row 4, 20/27 = rows 5 and 6 (its two variants); all 23 requests decode
(about 297 KB with poses). The keys match the handlers: 40/05 request 16
loops on key 27, 40/06 request 3 counts 34 down to 0, 40/07 request 2
reaches 71.

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

### Drawing the event-4 actors (CANDIDATE presentation)

Each slot's model is the 0x8002F9C4 row (first match; 20/27's +0x0C picks
the first or second row, CANDIDATE order) decoded once with its pose bank:
class 40 as scene actors with the handlers' requests, class 20 with request
0 (0x800322A8(work, 0, 0), then 0x80032538 steps under 0x80032488's rules).
The draw list takes live slots whose +0 bit 1 is set, the pose record the
slot's cursor points at, and the float 0x80033598 hierarchy of
`build_source_scene_actor40_pose_matrices` with RotMatrix(+0x54) in place
of yaw alone and the +0x158/+0x15C Q12 scale (0x0200 = 1/8 for all five).
The 3DS renderer draws them through the area actor material cache.

On the cooked ST02_01.EBD: 40/05 18 parts, about 139 units tall; 40/06 18
parts, 157; 40/07 13 parts, 155; 20/0A 2 parts, 1920 (the ship); 20/27 2
parts, 79 per variant; 352 KB for all six models (768 KB budget). Shading,
GTE rounding, OT order and the face UV channels are not covered; nothing
has been seen on hardware yet.

## ST03 area 1 and event 8 (VERIFIED from the instructions unless marked)

**Start.** ST03 area 1's set-up 0x80100990 (state byte +4) tests story
flags with 0x8001DA58: flag 12 -> object set 5; 14 -> set 4; 13 -> set 3;
0 -> set 2; 4 -> set 1; 3 -> set 0; none of them (the first arrival) ->
set 0, then 0x8001DDE4(8) with event +4 = 0 (scene 0). Object sets
(0x80103AB0(n)): table 0x80116578 + 8n = {count, records}; each 20-byte
record goes through 0x8001EAE8 and its object pointer is kept at
0x8011CB30 + 4 * record[0], with a bitmask at 0x8011536C. 0x80103BA0
releases the current set (family 0x20: +8 = 2; family 0x60: own path).

**Event 8** (handler 0x80101D18) runs one of five scenes by event +4
(table 0x80115370: 0x80101D54, 0x80102224, 0x8010263C, 0x80103578,
0x80102DBC), each a state machine on +6. Each scene has its own
timelines and camera programs (cook_cutscene: six timelines, every record
calling 0x80103A88 = 0x8001EC0C camera step + 0x8001F070 timeline step).
Records with duration 0xFFFF hold; the scene moves past them with
0x8001F158 (unconditional next record: +0xA4 += 8, +8/+9 from the new
record with the same counter resets as 0x8001F070, +0x10 = 0).
0x8001F1DC zeroes +8, +9, +0x10, +0x18, +0x1C.

**Scene 0** (the crash site, 0x80101D54, states via 0x80100004):
- 0: next. 1: when 0x80042238(player) is ready: 0x8001F1DC,
  0x8001E6C0(0, 0, 0), camera program 0x80115FC4, timeline 0x80115FB4;
  falls into 2.
- 2: player +8 = 3, +9 = 1 (walk to target), target 16.16 at +0x428 =
  (-4584, -1, 1431), shorts via 0x80030150 into +0x438, +0x440..+0x444 =
  0, +0x448 = 48, +0xBD |= 0x10; scene object 0 (0x8011CB30): +4 = 2,
  +8 = 0, heading 3703; object 1 (0x8011CB34): +4 = 1, +8 = 0, heading
  3790.
- 3: waits for the main-loop subphase byte 0x800C1B11 == 2; +7 = 1;
  0x8001F1DC, 0x8001E6C0(0, 0, 0), camera 0x80116034, timeline 0x8011600C;
  falls into 4.
- 4: when the current record's byte 1 (event +9) is 1: target again,
  heading +0x442 = 1255, countdown +0x0C = 15.
- 5: countdown below 0: message block 1 (0x80053A08(0, 1)); object 1
  +9 = 3, +0x0A = 0.
- 6..8 (fall through while the slot-0 window flag 0x80098A58 &
  0x08000000 is clear; opening a message sets it at once, so one message
  per frame): message 2 with object 0 +9 = 1, object 1 +9 = 0; next record
  and message 3 with object 0 +9 = 0, object 1 +9 = 3; next record with
  object 1 +9 = 4.
- At the timeline's end record (byte 0 == 0xFF): message 4; then once the
  window closes, countdown 8; at zero: 0x80042314(player) release, event
  +0 = 0, 0x8009395C = 1, flag 3 (0x8001DA8C), release the set
  (0x80103BA0) and spawn set 0.
- Every frame from state 2 the current record's callback runs; in states
  3..10 Start (0x8001DE84) skips: 0x80053A30, 0x80063BA8, 0x8001D494(0,
  1, 0), 0x8001B314, 0x8001A008(264, 0, 0), flag 10, state 11 with
  countdown 1, and the player target is placed again.

The roadmap's oracle run (M3_ROADMAP.md S1) matches: control at
(-4584, -1, 1431), flags 0x0A then 0x03. Area 1's messages are
`..\mess\st03_00.msg` inside ST03_00.BIN, cooked as the stage's
`messages/area_01.l3msg`. Scenes 1..4 (police car, Inspector, ...) and the
ST03 scene objects' handlers are not read yet.

**No walk (VERIFIED_1TO1, oracle from `m3_00_stage2_event4`).** The player
is not walked to the target: two frames after event 8 starts, player state
is 3/2 with the pose already at (-4584, -1, 1431) (heading 0), because
state 3 (0x80041B8C) copies the targets +0x438/+0x428/+0x440 into the pose
every frame. Heading becomes 1255 when state 4 ends; after the event ends
the player returns to state 1/0. The port now writes the same targets and
state 3/1 and lets the ported state 3 move the pose (no direct pose write).

## Recomp comparison: ST02 event 4 → ST03 event 8 (PROCESS rule 4)

Oracle: `m3_00_stage2_event4` loaded into slot 10, `tools/compare_recomp_3ds.py`
(now also snapshotting flags 0x000..0x3FF at 0x800BE378), Circle taps to
advance messages, no Start skip. 3DS: run21 (f4e5687) and the run after
this change; the 3DS trace now logs story flags 0x000..0x9FF. A flag write
trace (`wtrace_arm 0x800BE378..0x800BE4B8`) named each writer:

| Oracle frame | Flag | Writer | Port before | Port now |
|---|---|---|---|---|
| event 4 end | clear 0x600 | ST02 event 4 (0x80101F8C) | yes | yes |
| event 4 end | set 0x384 | dispatcher 0x8001D9F0: event +0 went 0, so 0x380 + event id | missing | set (also 0x383/0x382 for ST00 events 3/2) |
| ST03 load | set 0x7E1, 0x7E2, 0x7E4, 0x7E8 | ST03 stage init 0x801007B4 → 0x8001E4C4 → 0x8001E460 (door list 0x801152E4, states 0x801152EC + 8 × byte 0x800C1B62, overrides by flags 2 / 14 / 0) | missing | missing: doors slice |
| ST03 load + 1 | set 0x1FF | area-enter table 0x80115124[area] (area 1: 0x80101778; door locks there need flag 0x235) via 0x8010089C | missing | missing: doors slice |
| event 8, message 4 | set 0x0A | message opcode 0x9A (0x800553A8; 0x9B clears, 0x800553F0) inside the scene's message | missing | set |
| event 8 end | set 0x03 | scene 0 state 11 (0x80102110) | yes | yes |
| event 8 end | set 0x388 | dispatcher 0x8001D9F0 (0x380 + 8) | missing | set |

Oracle frames (ring frames from the slot-10 load): 0x384 / 0x600 at 1870,
door locks 2167, 0x1FF 2168, 0x0A 4103, 0x03 + 0x388 4746. Message opcode
0x9A is the table entry 0x8008AB84 (base 0x8008AB1C, op − 0x80); the draw
table 0x8008ACC4 skips 0x9A/0x9B as 3 bytes (0x80054324), as the port's
message cursor already does.

**3DS result (run29, 7304def, Azahar):** `compare_recomp_3ds.py` reports
order matches; the port sets 0x384 with 0x600's clear (frame 4392), 0x0A
during message 4 (5325), 0x03 and 0x388 together at the end (5657). Only
the door locks 0x7E1/0x7E2/0x7E4/0x7E8 and 0x1FF are missing (doors
slice). The player reaches (-4584, -1, 1431) by frame 4396, within 2–3
frames of event 8 starting (4393), with no game over.

## ST03 trigger box and event 8 scene 1 (police car)

**Area 1 set-up state machine (0x80100990, byte +4, every frame).**
- 0: the first pass (object set by flags 12 / 14 / 13 / 0 / 4 / 3);
  +4 = 255 for 12, 14, 13, 0; 2 for 4; 1 for 3 or none (none also starts
  scene 0). 0x80101918(1) after 14 / 13, (0) after 0 / 4 / 3 / none; not
  ported (reported).
- 1 (0x80100ADC, the trigger box): needs flag 3; flag 4 moves on to 2;
  with 0x605 clear, the player cell (raw x / z >> 9) minus box origin
  (0x80114F2C: -5, 11), unsigned, below the size (2, 2) - x -2560..-1536,
  z 5632..6656 - sets 0x605 and starts event 8 scene 1.
- 2 (0x80100B90): flag 0 moves on to 3; otherwise flag 0x604 is cleared
  and event 8 scene 2 starts (the Inspector talk).
- The byte restarting at 0 on each area-1 entry is CANDIDATE.

**Scene 1 (0x80102224, states via 0x80100034).**
- 0/1: 0x80042238; byte 7 = 10, state 2; player +8 = 3, +9 = 1,
  +0xBD |= 0x10, targets = the current pose, +0x448 = 48.
- 2: byte 7 counts down; then waits until player +0x16 equals
  0x80048E6C(player); then 0x80031BEC(pose, (-2240, 7000) from 0x801164DC,
  heading, 32): a non-zero delta adds to +0x442 with +0x448 = 13; zero
  gives +0x448 = 48, message 31, state 3.
- 3: when the window closes: 0x8001A008(270), 0x8001F1DC, 0x8001E6C0,
  camera 0x80116164, timeline 0x8011614C, target (-2496, -1, 5065) with
  heading 2240, police object *(0x8011CB3C): +9 = 1, +0x0A = 0, +0x56 =
  1536; byte 7 = 10, state 4.
- 4: while the record's byte 1 is 1, byte 7 counts down, then +0x442 += 32
  per frame; at the end record message 32, state 5. The record callback
  runs only in state 4.
- 5: when the window closes: state 7, +0x442 = 0, 0x8009395C = 1, flag 4,
  0x80103BA0, 0x80103AB0(1). 6 (Start path): the same without 0x8009395C.
  7: event +0 = 0, 0x80042314. Start in states 2..5 goes to 6 (from 2..3)
  or 5 (from 4..5) and places the target (-2496, -1, 5065).

**Oracle (VERIFIED_1TO1, `m3_01_st03_crash_control`, an oracle-only position
write just outside the box, then walking in):** 0x605 and scene 1 start on
entering; byte 7 counts 10 → 0; the player turns +32 per frame with
animation 13 from 1255 to 2129, then message 31; state 4 places the player
at (-2496, -1, 5065) heading 2240 and the police car (+9 = 1) drives in;
heading +32 per frame after the countdown; state 5 → 7 with heading 0,
flag 4 and a new object set; the player returns to state 1/0. The port's
scene 1 follows the same steps (host test). The police car's own handler
(its drive) belongs to the ST03 scene objects, not ported yet; the port's
floor check for state 2 uses its floor query (CANDIDATE mapping).

# 0.0.15.16 validation: subtype-0x1F pop-out Reaverbot

Build: `pass-20261023-181458-m2-01516` (+ rebuild). `Legends3DS.3dsx` is 1,059,388 B,
SHA-256 `8c021fc55c3689e2a59bdb2e2ab8349935dd6f7e1097dd91d2b7a21fe72371da`
(not committed). Host: 162/162 PASS, including the new `class20_1f_popout` and
L3CB v5. Native: PASS, 0 warnings.

**Azahar 2126.0 (Old 3DS).** Normal boot with `autoplay/popout_corridor.txt`;
trace evidence. This was build `pass-20261020-171602`, which is identical except
that L3CB v5/byte 0x0B was not in it yet.

| Step | Original (`docs/research_01516/POPOUT_1F.md`) | Port (Azahar) |
|---|---|---|
| Wait | action 0 until work80 < 0x4E201 | **match**: triggers at work80 289908 / 314964 / 319252 as Mega Man walks past |
| Lunge | 9 frames: forward 0x40 ×8 then 0x200, Z scale 0 → 0x1000 | **match**: frames 870→879, x 2560→2624, scale_z 4096 |
| Hand-off | +2 = 3, +3 = 0x40; subtype-3 init on the next frame | **match**: morph at frame 880/895/911, subtype 3, action 8 |
| Combat profile | stats record of subtype 3 | pop-outs 2 and 3: HP 64. Pop-out 1 (work +0F = 0x80): **not admitted**, holds |
| Airborne entrance (action 8, param 0x40) | needs record byte +0x0B | that run: **held** (NeedsCombatProfile; byte 0x0B was not cooked) |

The fix for the last row (L3CB v5 with byte +0x0B) was built afterwards, but it
is **not validated in Azahar**. The Mac's screen was locked, so the emulator
could not start. Host tests cover the cook, the L3CB round trip and the binding.

Not covered:
- the hit proxy while hidden;
- record 1585's context byte 0x80: admitted afterwards from the instructions
  (bit masked by 0x7F); host-tested only;
- whether the airborne entrance matches the original (no PCSX-Redux trace).

## Left-right mirror (gameplay camera)

The gameplay view drew the world mirrored. Vertices are (x, −y, z) and the
LookAt is a pure rotation, but flipping Y needs a reflection.
`test_frontend_handedness` shows the gap:
- At the tutorial start, the port's copy of the original 0x80037FB4 camera plus
  RTPS (OFX 160, H 384) puts a point 300 units toward −x at sx 248, on the right.
- The un-flipped LookAt put that point on the left.

The 3DS LookAt path now negates camera-space x (projection column 0). The cutscene
path (source scene view) already applies diag(1,−1,−1)·R·diag(1,−1,1) and is
unchanged.

**Azahar check** (same idle start, old vs new build): the top-screen image is
an exact horizontal mirror of the old one; the bright wall panels at x≈95/365
moved to ≈527/257.

**Bottom-screen HUD and text:** these are drawn in software to the bottom
framebuffer, so the flip can't reach them. They read normally in the new build.

**Steering:** D-pad Right sends the original turn mask 0x0800 and raises the yaw
(trace: 0→108). Facing −z, that turns toward −x, which is the original's
screen-right, so after the flip Right also turns right on screen. No input
change was needed.

## Doubled top screen and hidden Mega Man (map face test)

**Cause:** the port drew every map polygon from both sides. The original runs
NCLIP and drops back faces, and in Area 1 (mode 2) it draws far back faces
near-black. See `docs/research_01516/TERRAIN_FACES.md`.

**PCSX-Redux at the control start:**
- mode 2, threshold 522;
- the start wall's back faces are culled at t0 284/270;
- 17 farther back faces are drawn black.

**Host check:**
- `terrain_visibility`: min SZ 284 for the start wall, skipped;
- retail-trig cross-check: the same four culled faces as the original.

**Azahar (new build):**
- `# terrain_faces classified=1 drawn=259 black=108 skipped=72 unsupported=158`;
- the idle start now shows Mega Man from behind with a clean corridor, like the
  PS1 frame;
- the radio-call spot also renders cleanly.

## Tutorial route: type 16, defeated flags and the E0/07 trap room

See `docs/research_01516/TUTORIAL_ROUTE.md`.

**Host:**
- type 16 (0x620 pulse, 60-frame wait, message, 0x621);
- `source_gate_e007` (switch box, gate close/shake/0x61F→0x60A/reopen);
- real Area 1 data: the gate resolves to IDX 119, descriptor 21 → 19.

**Azahar (`autoplay/trap_room.txt`, normal boot):** messages 0 → 1 → 2 → 3 → 4
→ 6 → 7 at frames 25/660/1483/1958/2529/3681/3783. The switch raises 0x607.

**Not matched:** message 7 comes about 100 frames after message 6, because the
six subtype-0x19 Reaverbots the original wakes in this room are not ported. The
gate's close/reopen happened too fast to capture on screen; it is covered by
the unit test.

## Subtype 0x19 trap-room Reaverbots

**Host:** `class20_19` covers:
- the dormant drop-in on the wake flag;
- the idle-to-selector picks;
- hit sounds 0xA3/0xA0/0xA2;
- the stagger knockback hop;
- the lethal countdown into the 0xA1 defeat.

The cook test covers the new L3CB rows and L3DP profile 6.

**Real Area 1 map** (retail trig/angle tables, owner data): both records commit
1200/1200 frames with no holds. Drop-in 34 frames; they land at y −1; they
wander/alert, or chase/lunge with detection on. They stay in the room with the
gate descriptor closed.

**Azahar:** not run. The Mac screen was locked, so the emulator could not
start.

## Area 2 targets, doors and the action check

`autoplay/area2_targets.txt`, diagnostic spawn `2 -5300 -576 12032 1024`, Azahar trace:
- **Check:** A next to target 0x61 sets 0x60E. Trigger 14 opens message 8, then
  9 (a Yes/No question); Right+A answers No and it closes at frame 846.
- **First fix:** before the Buster fix, only the first shot of the run did
  damage, because every later shot reused a slot with a stale hit packet.
- **After the fix:**
  - 0x61 goes 100 → 52 → 4 and breaks apart (0x609);
  - 0x60 goes 76 → 28 and is destroyed (0x608) at frame 1590;
  - both doors then open and release (nothing left to draw after frame 1590).
- **Not checked:** the door IDX swap on screen, and triggers 12/13 (their
  regions were not entered).

## Area 2 box (E0/05)

`autoplay/area2_box.txt`, diagnostic spawn `2 -256 -385 10500 0`. The spawn
is inside the E0/00 entry gate, so the box group is selected. Azahar trace:
- The box spawns in slot 15 with 160 HP and draws one RNG sample per frame
  (status Ready).
- Mega Man walks the z 12544 corridor to (4929, 12538) and shoots. HP goes
  160 → 112 → 64 → … → −8, then 4 break frames and release.
- On the release frame, reward record 1 (word00 0x0302040C) is queued and
  0x620 is set. Trigger 16 pauses objects; message 11 opens 60 frames later
  (frame 982).
- Message 11 ends in a choice. Plain A restarts the text, Right+A closes it.
  Then 0x621 is set, objects resume (pause 2), and the reward expands
  into 14 pickups at the box (frame 1455).
- **Fix found by the run:** the reward was first drained inside the actor
  tick, which trigger 16's pause skips. It is now queued right after the
  session tick, like the original's in-callback 0x80047228 call.
- **Not checked:** the box's look (untextured diagnostic mesh), its sound
  and the break effect.

## E0 object textures (targets, doors, box)

- E0/05 and E0/08 meshes now use their original TPAGE/CLUT pages. Their raw
  material words (Area 2: 0x3D40002A target, 0x7D80003A door, 0x3DC00026
  box) are cooked into `actor_area_NN.l3mc` with the same upload order as
  the Reaverbots. All three resolve in Area 2.
- The draws go through the scene E0 material plan. +0x40 is the flat-packet
  modulation, and its 0x02000000 bit makes the door pulse and the box fade
  semi-transparent (ABR from the TPAGE).
- The box fade now also scales: 0x8011CB08 starts at 0x1000 and grows by
  counter × 20, and ScaleMatrix 0x8007ADE8 is applied each frame (host test:
  4096 → 0x1028 → 4236 → 4422, colour 0x2E202020).
- **Crash found and fixed:** with the extra semi-transparent passes, Area 2
  overflowed the 256 KiB citro3d command buffer when the Reaverbots spawned
  (svcBreak PANIC loop at about frame 27). The buffer is now 512 KiB; peak
  use was 52% on the box route.
- **Azahar:**
  - Box route: 7 E0 objects drawn, all textured, none held. The box fade
    reached scale 4422, and the 0x620 → message 11 → 0x621 → reward chain
    still works.
  - Targets route: target 0x61 still takes hits and breaks. Target 0x60 was
    not reached because the script's fixed timing went out of step with the
    dialogue (Mega Man stopped at z 12861), so this is not a render result.
- **Not checked:** how they look. The Mac screen was locked, so no
  screenshot could be taken. Shade, OT order and the break effect are still
  UNKNOWN or unported.

## Sound effects and Roll's voice

See `docs/research_01516/AUDIO.md`. Cooked locally:
- `sfx_st00.l3sb`: 297 requests, 66 samples, 968,798 bytes.
- `voice_st00.l3vb`: 46 clips, 121.6 s, 4,597,248 bytes.

Azahar box route, after copying the user's own `dspfirm.cdc` to `sdmc:/3ds/`
(without it `ndspInit` fails and both players stay off). The trace shows:
- **Roll's voice:** message 2's clips 0x15, 0x16 and 0x17 start on cue
  (frames 5/34/113).
- **Shooting the box:** each shot keys 0x9A, each box hit 0xA0, and the break
  0xA2.
- **Message 11:** it is unvoiced (0xA2), so it plays the typing blip 0x84 per
  glyph (235 requests).
- **Distant objects:** the target beeps 0x11C (2/18-frame rand reload) and the
  door pulses 0x11A are cut off beyond 5000 units, as in the original gate.
- **Reaverbots:** 0x9A/0x9B/0x9D/0x9E play near them.
- The box chain still ends with 0x621 set, with no panic.
- **Found and fixed during the run:**
  - The source audio queue was never drained, so it filled at 24 records and
    dropped the rest.
  - Without rand() the target beep fired every 2 frames.
- **Listening check (user, Azahar):** the effects and Roll's voice sound right.

## Area music (libsnd SEP)

See `docs/research_01516/MUSIC.md`. Build `pass-20261024-103151-m2-01516`:
- `Legends3DS.3dsx` 1,110,304 B, SHA-256 `924135aa…52f7` (not committed);
- host 166/166 PASS (new `source_music`); native PASS, 0 warnings.
- Cooked locally: `music_st00.l3mu`, 712,048 B (area row `FFFF 0 0 1 FFFF
  0`, requests 0/1, m_op01b SEP + VH, 14 VAGs (5 looped), reverb entry
  `02 40 40`).

Scratch render of the real bank, 3 minutes:
- 0 errors, up to 15 voices;
- the NRPN loop wraps about every 51 s;
- RMS ~1030 dry, ~1400 with reverb; ~570× realtime on the Mac.

**Azahar 2126.0 (Old 3DS).** Normal boot with `autoplay/music_area1.txt`
(40 s in Area 1), trace evidence:
- `source_music_open` ready; `source_music_request` area 1 → request 0x0,
  queued; frame 1: handler 1 dispatched, slot 0 = access 0, volume 127.
- The reverb is type 2, with output 16512 (0x4080) after the 30-pass wait.
- Key-ons at frames 300/600/1200: 80/246/638. The host render has 81/249/644
  at 10/20/40 s, so the sequencer keeps real time.
- 0 errors; 1 underrun (at start), no more afterwards.
- Frame interval ~33 ms; the slow frames are render-bound (render 33–39 ms).

Not covered:
- **Listening check (user, Azahar, merged build):** the area music plays.
  There is no side-by-side comparison with the PS1 yet.
- Area 1 → 2 continuity (same request, no restart) is host-tested only.
- The music stops while an area load blocks the main loop.
- No PS1 capture comparison (tempo, levels, reverb tail).

## Full-run attempt (stopped)

`autoplay/tutorial_full.txt` uses new event-driven autoplay steps: talk, wait
for flag, steer-until, fight, shoot-at, door. Normal boot, Azahar:
- Area 1 messages 0 → 1 → 2 → 3 → 4 → 6 → 7 → 9 → 10 → 11 in order.
- **Bug found and fixed:** the live class-0x20 list fed to trigger 6 skipped
  the subtype-0x19 Reaverbots (they keep +14/+18 in `source_origin`, not in a
  behaviour state). So 0x61F was set right after message 6 and the trap-room
  fight was skipped. Now all six drop in and are shot down (0xA2/0xA1, E0/00
  flags 8..13), and message 7 follows the fight.
- Both corridor doors and the Area 2 door were passed in one run (Area 2
  reached at frame ~5200).
- The run was stopped at the user's request before reaching 0x622. The door
  steps of the script are timing-sensitive, and the Area 2 floor needs a jump.


## Break and burst effects (0x80031618)

Build `pass-20261024-102424-m2-01516`: `Legends3DS.3dsx` 1,097,780 B, SHA-256
`bcd71b5a21063c8630396ec0441f24559c95b2d5921bdf249b6bb59e6dca692a` (not
committed). Host 165/165 PASS, native PASS with 0 warnings. Research:
`docs/research_01516/EFFECTS.md`.

Azahar 2126.0, scripted, with trace lines `# effect_spawn`, `# effect_tick`
and `# effect_draw`:

- **Targets** (`autoplay/area2_targets.txt`, spawn `2 -5300 -576 12032 1024`):
  - Target 0x61 breaks. At frame 1270, three 1/0x83/2 puffs spawn from
    0x801165A4 at (−5888, −624, 12032), (−5942, −624, 12066) and
    (−5997, −624, 12099), 0x40 apart along the heading.
  - At frame 1279, the 5/0x80/0x82 burst spawns from 0x8011669C at
    (−5973, −608, 12084).
  - Drawn textured: 3 sprites and 1 model, 8 passes, none held. The burst is
    released after 15 frames (the host test predicts 14 grow frames plus
    release); the puffs are released at frame 1298.
  - Target 0x60 again fell out of step with the script (state 2 at the end),
    as before.
- **Box** (`autoplay/area2_box.txt`, spawn `2 -256 -385 10500 0`):
  - At frame 963 the 5/0x80/0x42 effect spawns from 0x801141EC at
    (5120, −513, 12544), which is the box's y − 0x80.
  - The reward follows at frame 966.
  - The model is drawn textured (2 passes) and stays frozen while trigger 16
    pauses the world for message 11. It is released at frame 1541, after the
    answer.
  - The chain still ends with 0x621.
- **Textures:** the Area 2 actor cache now resolves all 7 words, including
  the burst page 0x3C80002B and the puff page 0x3D48002E (upload order
  CANDIDATE).
- **Visual check (user, Azahar, merged build):** the target, box and
  Reaverbot break effects look right. There is no PCSX-Redux frame comparison.
- **Not watched:** the E0/07 camera shake (host-tested only).
- **Azahar note:** another session's run replaced the shared SD folder halfway
  through one box attempt. The run above is a clean rerun.

## Renderer fidelity R1: terrain depth cue and back-face threshold

See `docs/research_01516/RENDER_FIDELITY_R1.md`.

**Host:** `test_terrain_visibility` covers:
- the player depth is 511 for the authored start camera (522 measured on PS1);
- a near face keeps 0x808080;
- a face about 6000 units ahead gets IR0 4049 and colour (1, 17, 17);
- bit 28 skips the cue on positive cells; negative cells are always cued;
- without an area record nothing changes.

Host 166/166, native PASS.

**Azahar:** Area 2 entry, spawn `2 -256 -1 6400 2048`, idle autoplay. Same
framing for the old build (`dc8b649`) and the new one:
- **Trace:** the threshold is 521 (was the fixed 522). 397 of 426 drawn faces
  are depth-cued, with fog on (far colour (0, 16, 16), distance 1280).
- **Image:** the corridor's far wall and upper panels fade to dark teal. The
  near walls, the floor and Mega Man are unchanged. The screenshots are kept
  locally (`3DSPORTS/renderer-shots/r1_area2_corridor_{before,after}.jpg`)
  because they contain game textures.
- **Cost:** frame interval 50.1 ms → about 50 ms, render 40.2 → about 41 ms,
  32 draw calls in both. Command buffer 38% of 512 KiB. Linear memory grows
  by 12 bytes per terrain vertex.
- **Not compared:** there is no PS1 reference image of this view, so
  closeness to retail is shown in code, not in images.

## Renderer R2: CLUT depth ramp and near cull

**Host:** `test_terrain_clut_ramp` covers:
- the otz rules for bits 31/27;
- the near cull at < 0xC1;
- the ramp step by area bytes, LOD and bit 28;
- the L3CR fit, round trip and rejection.

Host 167/167 (the pack test expects `--ramp-out`), native PASS. The pack now
cooks `area_00..04.l3cr` (Area 1/2: 12 words, 644 bytes).

**Azahar** (Area 2 west wing, spawn `2 -7000 -576 12300 3072`): 12 ramp words
loaded; 327 of 429 drawn faces ramped, 416 depth-cued. Frame interval 49.9 ms
and render 38.7 ms, the same as R1. 32 draw calls; command buffer 38% of
512 KiB.

**Not shown in an image:** both test spawns put the follow camera inside a
wall. The ramp only starts at otz 5120, where the fog is already about 94%
toward the far colour, so its visible effect in these rooms is small. There
is no PS1 reference image.

## Renderer R3: per-cell visibility

**Host:** `test_terrain_cell_visibility` checks:
- nine yaws covering both wedge walkers;
- the near window, the cells ahead, the cells behind and to the side, and
  the side cut;
- that the widened cut contains the PS1 cut;
- the mode-1 whole-area walk;
- that TR `0x1F800030` equals the camera translation.

Host 168/168, native PASS (the 3DS Makefile lists the new source).

**Azahar** (2126.0, idle autoplay `3000 0 0 0`; R2 build before, R3 after;
each instance run alone):

| View | drawn | black | skipped | unsupported | culled by cell | frame interval | render | draw calls / triangles |
|---|---|---|---|---|---|---|---|---|
| Area 1 start, before | 259 | 108 | 72 | 158 | – | 40.8 ms | 31.3 ms | 27 / 2032 |
| Area 1 start, after | 116 | 34 | 4 | 0 | 443 (53 IDX, 28 negative cells kept) | 33.1 ms | 25.9 ms | 17 / 1282 |
| Area 2 entry (`2 -256 -1 6400 2048`), before | 426 | 204 | 42 | 16 | – | 50.0 ms | 38.1 ms | 32 / 2588 |
| Area 2 entry, after | 314 | 141 | 39 | 16 | 178 (145 IDX kept) | 50.6 ms | 39.5 ms | 29 / 2238 |

- Black faces at the Area 1 start fall from 108 to 34. The original draws 17
  there; the rest is likely the missing Near LOD and the see-through-wall rule.
- The 158 unsupported faces (vertices behind the camera) at the Area 1 start
  are all in culled cells.
- Area 1 frame time improves by about 19%. Area 2 is unchanged within noise.
- **Caveats:**
  - Azahar timings are emulator timings, not Old 3DS ones.
  - No screenshot was taken, so holes at the screen edges are not checked by
    eye.


## Renderer R3: see-through walls
Rule of `0x80024F30`/`0x80025900`, plus a semi-transparent pass after the
actors (`docs/research_01516/RENDER_FIDELITY_R1.md`).

Host 167/167 (`test_terrain_visibility` extended), native PASS.

**Azahar** (Area 1 start, idle `3000 0 0 0`, trace only):
- `see_through=0` and `see_through_held=0`, as expected for this view (the
  start wall's back face is skipped);
- other face counts unchanged;
- command buffer 29%.

No case on screen: screen access was not granted, and a second launch at the
Area 2 wall spawn did not boot.

## Renderer R3 merged (cell visibility + see-through)
Both R3 branches merged into main. Host 168/168, native PASS
(`Legends3DS.3dsx` 1,134,568 bytes, sha256 `cc3a54d2…5da7`).

**Azahar 2126.0** (merged build, seen on screen this time):
- **Area 1 start**, idle: trace identical to the cell-visibility branch (drawn
  116, black 34, cell-culled 443, threshold 522, ramped 46, see_through 0).
  Screenshot: the corridor is complete to the screen edges, with no holes and
  no visible black faces. 30 FPS.
- **Area 1, slow spin** (`3000 0 0 0` then `2400 0 -60 0`): the camera sweeps
  through both corridor walls. No holes in the screenshot taken mid-turn. Every
  trace sample (every 120 frames) shows see_through=0.
- **Area 2 west wing** (`2 -7000 -576 12300 3072`): the spawn puts the camera
  inside the wall, so the wall's front face is behind the camera and
  see_through=0 is correct. The screen shows that wall's dark back face, which
  is a spawn artifact.

**Still open:** the see-through pass has not been seen to fire in game. It
needs a view where a wall sits between the camera and Mega Man, such as a
corridor corner while walking.

## Clear colour and the see-through corner walk
- **Clear colour:** `FUN_8001215c` sets both DRAWENVs (`0x800C0C5C`,
  `0x800C0CEC`) to isbg=1 with r0,g0,b0 = 0, and nothing else in the EXE
  writes those bytes. The 3DS cleared to navy `0x101820`; it now clears to
  black. In Azahar the space above the Area 1 walls is black, matching a user
  PS1 screenshot of the same corridor (SOURCE_BACKED; overlays not searched).
- **See-through corner walk** (tutorial route: radio, (2816,-5900),
  (2816,-9500), (1500,-10000), (-256,-10000); reaches (-209,-10020) at frame
  1440). The trace now logs every 10th frame whenever see-through faces exist.
  No frame logged any, so the rule did not fire around the first corridor
  corner. The follow camera stays inside the corridor there, and no front
  face comes between it and Mega Man. Where the original shows this effect
  is still UNKNOWN.
- **User check (2026-10-24):** after the explanation of the see-through rule,
  the user reported "yea it works" for the merged R3 build in Azahar
  (user-observed, no screenshot or location recorded).

## Renderer R4: per-IDX Near/Mid/Far LOD and the mode-2 off-screen test
- **Host:** 168/168. New coverage:
  - the LOD cuts and `idx_lod` (`test_terrain_cell_visibility`);
  - the off-screen test: a face to the side, the diagonal midpoint and the
    widened window (`test_terrain_visibility`).
- **Host replay, Area 1 start, PS1 window:** 17 black faces, with t0 values
  identical to the PCSX-Redux probe (5377×4, 5391×4, 6398, 6412, 6909×3,
  6922×3, 7419). 115 faces pass the screen test, matching the probe's 115
  hits per frame at `0x80025E70`.
- **Azahar 2126.0** (Area 1 start, idle):
  - trace: drawn 135, black 25, off-screen 28, LOD quads 74/84/6, cell-culled
    1169 (that count now includes the other-LOD copies of visible cells' quads);
  - 25 is exactly what the host predicts for the CANDIDATE 400-pixel window:
    the PS1's 17 plus 8 in the extra side strips;
  - screenshot: the near walls show the Near descriptors' finer stone and band
    detail, with no holes; 30 FPS.

## Renderer R5: GP0 packet translator (terrain)
- **Host:** 169/169 (new `test_psx_gpu`: OT order, bit-28 chaining, GP0
  decode, the 0x80026158 terrain packet, subdivision rule).
- **Azahar 2126.0** (Area 1 start, idle):
  - 113 positive faces drawn as POLY_FT4 packets in 3 runs, 0 unresolved
    CLUT/TPAGE;
  - 19 faces the original subdivides stay on the 3D path; black 25
    (unchanged); packet SXY vs the 3D camera at most 1.19 px.
- **Screenshots:**
  - with x/y from the GTE SXY, Mega Man, the walls and the depth were
    correct, but thin dark seams showed where packet faces meet the 3D-drawn
    subdivided faces;
  - the build now takes x/y from the 3D camera. Its trace is the same, but
    no screenshot could be taken (Azahar window capture failed), so the seam
    fix is **not verified by eye**.
- **User check (2026-10-24):** the R5 build's walls look clean in Azahar (no
  seams).

## Renderer R6: subdivider 0x8002466C, every positive face a packet
- **Host:** 169/169. New coverage: `test_terrain_visibility` (midpoints, UV
  centre rule, corner SXY, NCLIP rule) and `test_psx_gpu` (FT3 round trip).
- **Azahar 2126.0, Area 1 start, idle:**
  - 216 packets in 3 runs, 19 faces subdivided, 0 left on the 3D path,
    drawn 135, black 25;
  - SXY vs the 3D camera at most 3.35 px.
  - Screenshot: no seams, near walls show the affine texture bend, Mega Man
    aligned.
- **Spin:** up to 43 subdivided faces per frame, 0 unresolved, no 3D
  fallback. A vertex at the camera gives up to 50 px of SXY saturation.
  Faces with a vertex behind the camera (unsupported, up to 162 while the
  camera is inside a wall) stay 3D.

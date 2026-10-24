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


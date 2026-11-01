# Tutorial Reaverbots and the gameplay camera (branch reaverbot-camera-fix)

User report: the Reaverbots spawn, but the pop-out ones stay floating, and the
camera zoom may not be exact.

## 1. The pop-out Reaverbot stays floating: fixed

**Cause (VERIFIED_1TO1, from the port's own traces).** In every Azahar trace
since the pop-out was ported (for example
`hardware_01516_4002217165791.csv`), the three corridor pop-outs rewrite
themselves into subtype 3 and then log `actor_state1 ... status=16, action=8,
phase=0` on every sample. Status 16 is `NeedsAnimationConsumer`.
`tick_imported_actor_state1` holds the whole callback when the selected
animation request isn't in the actor's bank. The class 20/03 bank decoded only
requests `{0,1,2,3,4,6,21,22}`:
- action 8 (the airborne entrance with parameter 0x40) selects request 12, then
  13..16 on each landing;
- action 7 (Buster knockback) selects 7..11;
- phase 3 of both selects 17.

So the pop-out froze where it sprang up (y −129), and any class 20/03 hit by a
non-lethal shot would freeze in the same way.

**Fix.** `l3::kClass20_03AnimationRequests` = {0..4, 6, 7..17, 21, 22}, used by
the native binding. A probe of the real Area 1 EBD class 20/03 row decodes
every listed request: 7..17 have 3..18 records each, and 5/18/23 are absent.
Commit `42a6835`.

**Landing (SOURCE_BACKED_PARTIAL).** `popout_landing_sim.cpp` runs the morphed
state through the ported reaction reducer and contact solver on the real
`st00.l3map`, with the real `actor_reactions.l3r7` and combat byte 0x0B (= 8).
The result:
- falls from y −129 to the floor (y −1) in 5 frames;
- bounces four times, smaller each time (requests 12→16, sound 222 on each
  landing);
- recovers in phase 3 (request 17) for 18 frames, then flags |= 9 hands back
  to the action chooser.

No snapping and no forced floor: the drop comes from the original timer table
and the 0x1000 contact bit. It has not yet been compared with a psxrecomp trace
(ST00 isn't native in the oracle, and the oracle was in use).

**Hidden-pop-out hit proxy (VERIFIED_1TO1 against the ST00 disassembly).**
`0x80106410..0x80106470` registers `0x800341BC(+0x354 ← +0x180, 0x50, 0,
0x8000, work+1, +0x35C, 0x04880000|+0x350, 0x00800000, 0x80)` whenever flags
bit 8 is set (flags 0x2C sets it), then clears +0x35C. It is now registered,
so a shot is stopped by the hidden pop-out, with no damage because bit 2 is
clear. Commit `8a8f235`. The +0x180 point is computed right after the callback,
as for 0x19 (one-frame timing, CANDIDATE).

## 2. Other Reaverbots
- Area 2 class 20/03 (parameter 0x20) sitting at action 3 phase 5: not a stall.
  Phase 5 releases them through `0x80031824` (`work_00 = 0`), and the trace
  reprints the released record.
- Action 7 knockback was blocked by the same missing requests and is fixed by
  the bank change.
- Trap-room 0x19 (separate port, `test_class20_19`): no change.
- Not compared against the oracle: attack timing, chooser and defeat, frame by
  frame (UNKNOWN). That needs ST00 compiled native in psxrecomp.

## 3. Camera zoom
Checked against `0x80038894` (decompile plus instruction-level reads at
0x80038BD0..0x80038C00):

| Item | Original | Port | Label |
|---|---|---|---|
| target | player x/z, y = player y − [0x800A39B4] (0x96) | same | VERIFIED_1TO1 |
| yaw chase | 0x8003A218/0x8003A154, rate gap/4 (min 0x18, else 0x80), lag 0x200 | same | VERIFIED_1TO1 |
| distance | +10/−10 per frame, clamp 0x200..0x300, eye test only while player +0x44D == 1, delay 0x3C | same | VERIFIED_1TO1 |
| pitch | 0x30 | 0x30 | VERIFIED_1TO1 |
| +0x44D = 1 writers | action 2 `0x8003CF58`/`0x8003CFC0`, action 3 `0x8003D584`, action 8 `0x8003DCA4` | action 2 and 8 sites ported; action 3 unported (the whole action) | SOURCE_BACKED_PARTIAL |
| projection | H 384, OFY 120, 240 lines | vertical FOV 34.708° = 2·atan(120/384), 400×240 | VERIFIED_1TO1 vertically; wider horizontally by design |
| yaw/pitch offsets 0x800A3974/78 | added in the 0x80037FB4 call; written only by scene setter 0x80039CA0; 0 in the oracle | omitted | CANDIDATE (event scenes only) |

**Oracle check (VERIFIED_1TO1).** In psxrecomp at the Area 1 start, at rest the
original has distance 512, pitch 48, height 150 and target y −151. While
running, +0x44D = 1 and the distance grows 10 per update (512 → 622 → 672).
When Mega Man stops, +0x44D = 0 and the distance holds. The port's
`source_camera_eye_80037fb4` with the real trig tables reproduces the
original eye exactly at all four sampled frames:
(2822,−188,−4322), (2822,−188,−4642), (2822,−196,−4884), (2822,−200,−4962).
The pop-out was not traced in the oracle: the tutorial message boxes blocked
movement and the run was cut short.

So in normal follow, the distance, height and vertical framing are the
original's. In an open corridor both games grow the distance to 0x300; the
port's trace shows 768 there.

**Real difference (not fixed here).** The original has a second, closer camera
that the port lacks. Player action 14 (`0x8003EEC0`, the lock-on/aim stance;
table 0x80089FAC) calls `0x80039630(+0x108, 350)` every frame. That blends
over 8 frames to distance 400, target height 0xD2, and pitch = the aim pitch
`+0x108`. Leaving the stance blends back through `0x800394AC`/`0x800395E8` to
0x200 / 0x30 / 0x96. Action 14 isn't ported, so the port never zooms in while
aiming. This is the most likely cause of "the zoom isn't exact". Porting it
means porting action 14 itself (follow-up).

## Validation
- Host: 170/170; new `bank_covers_reaction_requests` (test_class20_03_reaction)
  and `hidden_proxy` (test_class20_1f_popout).
- Native: `tools/m1_mac_build.sh` PASS (both commits).
- Azahar: **not run.** `azahar -w` stalled before boot (empty Azahar log, game
  list only) while the Mac's current Space was a full-screen app; another
  session's run stalled the same way. Re-run
  `docs/validation_01516/autoplay/popout_corridor.txt` and look for
  `actor_state1 ... action=8` with phase 1→3 and status 0.
- No physical Old 3DS test.

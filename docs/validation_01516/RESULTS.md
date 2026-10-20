# 0.0.15.16 validation: subtype-0x1F pop-out Reaverbot

Build: `pass-20261020-185103-m2-01516`. `Legends3DS.3dsx` is 1,023,904 B,
SHA-256 `e2878cb96eedbf9766e3f75b18ad0aa05f2a768b507dd975c614dcccda91924f`
(not committed). Host: 157/157 PASS, including the new `class20_1f_popout` and
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

**Seen but not part of this fix:** in both builds the top screen looks doubled
and blurry at the idle start, and Mega Man isn't visible.

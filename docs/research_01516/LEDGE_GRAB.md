# Ledge grab and pull-up (0x80040818, action 13)

Status: SOURCE_BACKED_PARTIAL. Needed for the Area 2 raised ledge (wall at
z 8704, top about 192 above the floor), which a jump (~104) cannot clear.

## Trigger 0x80040818 (from decision 0x800400B8, actions 7/8)
Runs before the motion handler, so the new action's motion runs the same frame.
- airborne (+0x74 != 0), falling (+0x46 > 0), feet at least 0x30 above the
  floor (y <= 0x80048E6C - 0x30 - bounds.y1).
- q = ((+0x56 - +0x116) + 0x200) >> 10; wall = 0x8004976C -> 0x800497CC(q & 3).
- even q: dz = wall - z; probes at x + x0 and x + x1, z + (q&2 ? z1+1 : z0-1) + dz.
  odd q: dx = wall - x; probes at z + z0 and z + z1, x + (q&2 ? x1+1 : x0-1) + dx.
- |dist| < 0x41; both probes at y - 0x98 must give the same 0x80048EA4 floor
  (NULL bounds, dy 0) and y - 0x98 < top < y - 0x68.
- Then raw x/z += d (fixed +1C/+24 are not updated: source quirk, kept), y and
  fixed y = top + 0x98, heading = (q&3) << 10, +0x116 = +0x112 = 0,
  action 13, phase short = (q&3) << 8.
- 0x80039A38(&0x80097D9C) camera request: UNKNOWN, not ported.

## Wall finder 0x800497CC
Starts at -0x7FFF (q 0/1) or 0x7FFF (q 2/3). For every candidate cell (mode 0)
and group whose other-axis expanded span holds the position, every primitive
(flag 0x8000 filter as 0x8004B414) whose 0x8004A060 Y range holds y:
q0 max z1' with z0' < z; q1 max x1' with x0' < x; q2 min z0' with z < z1';
q3 min x0' with x < x1' (primes: expanded by the bounds).

## Action 13: decision 0x80040178, motion 0x8003EB08 (table 0x80010C34)
- decision, phase 1: floor - 32 - y1 < y -> 0x8003EE68 release, forward -42.
- 0: shot off, anim 29, sound 159, +44A = +B4 = +44D = 0.
- 1: anim done (+A7 == -1) -> anim 30.
- 2: hang. Back (+126) -> phase 6. Re-probe the top one unit past the face;
  |y - (top + 152)| >= 32 -> release; else y = top + 152. Forward (+124) ->
  +75 = +74 = 2, +76 = 0, +44 = +48 = 0, +46 = ((y - +2E) << 5) - 144, anim 31.
- 3: move (0, +46, 0) until +A6 == 1.
- 4: +44 = 80, +46 = -432, move.
- 5: +46 += 48, move; +74 == 0 -> anim 19, sound 147, action 8 phase 6;
  then +A7 == -1 -> release.
- 6: +44 = -42, move, release.
- release 0x8003EE68: action 7 phase 4, +75 = +74 = 1, shot on, anim 18.

+74 = 2 blocks the 0x8004B414 contact-state write-back, so the pull-up ends
through the release, not a landing.

Test: tests/test_runtime_contact.cpp `ledge_grab_and_pull_up` (authored block).

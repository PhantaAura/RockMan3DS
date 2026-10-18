# Collision Ownership Pass 0.0.11

This pass starts closing the player-collision blocker with exact USA executable behavior rather than assigning guessed floor/wall meanings.

## Exact response wrapper recovered

The player update calls USA `ROCK_NEO.EXE` `0x8004B414`.  That wrapper invokes the geometry solver at `0x8004B6AC` and then owns the final position/rollback mutation.

The portable bridge now reproduces the proved post-solver behavior:

- positive solver returns use masks `0x0300`, `0x0C00`, and `0x3000` to snap X, Z, and Y respectively from retail collision scratch `0x1F8000C0/0xC4/0xC2`;
- every snapped short coordinate also rewrites its authoritative 16.16 mirror;
- collision state is copied from scratch `0x1F8000F6` unless player work `+0x74` is already `2`;
- zero/negative results take the exact rollback branch selected through USA helper `0x8002FE74(work+1)`;
- the zero-group path restores X/Z, clears forward/strafe, and conditionally restores Y/clears vertical velocity when `+0x74` is nonzero;
- the nonzero-group path restores the entire saved short/fixed pose without inventing extra velocity changes.

`l3_pl00_snapshot_collision_pose()` preserves the exact rollback mirrors consumed by that wrapper before native fixed-point integration.

The geometry solver itself is **not** claimed complete yet.  This pass therefore exposes the response as an explicit runtime seam: a future exact solver can feed its signed result and scratch values into `RuntimeSession::apply_retail_contact_resolution()` without another approximate collision implementation.

## Jump state ownership recovered

The work-mutation portion of USA `0x8003DAC0..0x8003DB08` is now portable:

- launch only when signed `work+0xA7 == -1`;
- set `work+0x74 = 1`;
- increment `work+0x0A`;
- copy `work+0xAC -> +0xAD`;
- set vertical velocity `+0x46` to `-0x1C0`, or `-0x320` when `work+0x16B bit0` is set;
- re-enable the Buster through the already translated `0x800423C8` gate.

The jump-hold branch at `0x8003DB50..0x8003DB78` is also exact: when held input `+0x11C` intersects mask `+0x138`, vertical velocity receives `+0x30`.  The helper reports whether that result is still negative or has reached/passed zero, leaving animation/state side effects to the later action-state translation.

## Validation

- release host build: PASS with `-Wall -Wextra -Wpedantic -Werror`;
- CTest core suite: PASS;
- exact USA reference verifier: SUPPORTED, 205 DAT / 38 XA / 1 STR;
- fresh tutorial cook: 91 files;
- integrated tutorial runtime session probe: 5 areas mapped, 5 moved, save/dialogue/UI probe PASS.

This is a real collision-ownership checkpoint, **not** tutorial-complete 0.1.  Remaining collision work is the exact `0x8004B6AC` geometry solver/response loop and the rest of the player action-state transitions; combat, EBD behavior, cutscene/audio, and the hardware backend still remain after that.

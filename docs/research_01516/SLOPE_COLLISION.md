# Slope collision codes 8..11 (0x8004BDC8)

Status: **SOURCE_BACKED_PARTIAL**. Decompiled with GhidraMCP (ROCK_NEO_psx).
The classifier and the post-switch tail were read from the R3000
instructions.

## Why
The WCA route stalled at the Area 2 entry ramp, which is made of code-8
primitives. The movement solver rejected them as Unsupported. Mega Man hung
at a jump apex (y −104) and could not climb, so trigger 9 (y −0xC1 for
90 frames → 0x618) never fired.

## Source
- **Rectangle.** `0x8004B6AC` builds the raw rectangle (IDX + group x/z,
  IDX + primitive y). The family-0/1 classifier `0x8004E304` expands x/z by
  the player bounds in place and copies them to `0x1F8000F0/F8/F4/FC`. The
  dispatcher expands y (y0 − b.y1, y1 − b.y0). That matches the port's `Rect`.
- **Cases 8 / 9 / 10 / 11.** The top sits at the +Z / −Z / −X / +X side.
  - Guard: airborne, or the slope's blocked bit (8 / 4 / 1 / 2) clear, or
    more than 0x1F below the top.
  - Surface height: t = Y0 − (Y1 − Y0)·foot / span, h = Y1 − t, then
    surface = t + h·(distance from the top side) / span, clamped to ≥ Y0.
    The divisions truncate.
  - The rising face moves by span·(Y1 − y) / h when that is positive.
  - Room (`uVar7`) is the player's distance to that face before the move.
- **Tail `LAB_8004E1B8`.** Above the surface or under the slab: no contact.
  Grounded, within 0x20 and closer to the top than the room: snap onto the
  surface and clear u.y. Otherwise call `0x8004F44C`(top = surface,
  mode 0x43 | 0x20 for 8/9, 0x10 for 10/11). A nonzero result clears u;
  when not moving up, contact_state = 0 (`0x1F8000F6` = 0x100).
- **`0x8004F44C` modes.** Bit 0x10 uses `0x80050A24`, bit 0x20 `0x80050A90`
  and bit 0x40 `0x80050B68`: a nonzero delta and the direction not blocked,
  with no magnitude limit (Y: a nonzero correction).
- **After the switch.** When the guard fails and the player is inside the
  slab: onto its top, blocked = 0x20, u.y = 0.
  - CANDIDATE: u.x/u.z come from `0x1F800112/116`, which feed only the
    unported x5 fallback, and are taken as 0.

## Evidence
- `test_runtime_contact`: codes 8 and 9 climbed to the top grounded.
- Real Area 2 map (host replay, owner data): from (−256, −1, 7104), walking
  +Z at 24 units a frame reaches y −193 = −0xC1 grounded at z 7704, with no
  Unsupported status and 2 solver iterations per frame.
- **Not modelled:**
  - the floor query's grounded flag lags the slope surface by a few units
    while climbing;
  - codes 0x0C..0x0E (other slope/diagonal shapes) are still Unsupported.

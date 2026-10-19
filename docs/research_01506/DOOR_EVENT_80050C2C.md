# Player door event 0x80050C2C case 3 and event 0 (0.0.15.6)

Evidence: USA `ROCK_NEO.EXE` and the ST00 PROGBIN overlay (load 0x80100000),
decoded with `docs/research_01503/psx_mips_dis.py` (prints only instructions).
The owner-local Ghidra export `ROCK_NEO.EXE.2.c` was used to find the
functions; every claim below was re-read from the R3000 instructions. REA 6.7.0
cannot open PS-X EXE/raw MIPS, and Ghidra headless was not re-run this pass.

## Call site — VERIFIED (Ghidra export, instruction call at 0x8003C4EC body)
`0x8003C4EC` (player post-update) calls `0x80050C2C(player)` when
`0x8001DA58(0x200)` is clear. The return value is ignored.

## 0x800514FC event-primitive query — VERIFIED (0x800514FC..0x8005196C)
- Candidates from `0x8004B1E0(player xyz, 0)`, searched last to first;
  `0x80048CEC` returns the packed cell, `<= 0` is skipped.
- IDX record: xyz at +0/+2/+4; `raw6 & 0xFFF` selects descriptor +4, else +8.
- Groups: 16 bytes (x0,x1,z0,z1, primitive pointer, count); the list ends on a
  null pointer. Only groups whose **first** primitive code is `>= 0x100` are
  tested.
- Test point: the player xyz, or, when the first primitive's flags have bit
  0x8000, the point 64 ahead: `RotMatrixY(+0x56)` then `ApplyMatrixSV` on
  (0,0,-64), i.e. `x - (rsin*64>>12)`, `z - (rcos*64>>12)`.
- Shape test: table `0x8008AA24[(code & 0xF0) >> 4]`; families 0/1 are
  `0x8004E304`: rectangle expanded by the player bounds (+0x78 → scratch
  0x1F800124..2E), strict `x0 < qx < x1`, `z0 < qz < z1`. Its 1..4 classifier
  result is always nonzero inside.
- Per primitive: `lo = idx.y + p.y0 - b.y1`, `hi = idx.y + p.y1 - b.y0`, then
  `0x8004A060(rect, code & 0xFF)`. For low bytes outside 8..85 it takes the
  default tail `0x8004AC54..88`, which subtracts the bounds **again**. Hit when
  `lo < y < hi`.
- Result: `x = idx.x + (g.x0+g.x1)/2`, `y = idx.y + (p.y0+p.y1)/2`,
  `z = idx.z + (g.z0+g.z1)/2` (truncating halves), heading = +0x56,
  type = `code >> 8`, flags = primitive +6.

## 0x80050C2C prefix and case 3 — VERIFIED (0x80050C2C..0x80050CB4, 0x80050E0C..0x80050F64, 0x800513E0..0x800514A0)
- Jump table 0x80011034, case 3 → 0x80050E0C.
- `a1 = player` only if `(+1 & 0xF0) == 0`; that branch returns 0 when
  `+0xBD != 0`.
- Gate `0x80051970`: `a1 != 0`, `+0x184 == 0` (word), action `+9 < 2 or == 4`,
  `(+0x11E & +0x136) != 0`, `0x8004324C == 0`.
- Aux records (scratch 0x1F800050, 24 bytes, end on hw0 == -1): hw4..6 must
  equal the sample xyz. Facing: `((facing - hw7) + 0x200) & 0x400 == 0` — bit
  0x400 only, so both sides of the door pass. Quadrant
  `((facing + 0x200) / 0x400) & 3` with a truncating signed divide; side test
  0: `sample.z <= player.z`, 1: `sample.x <= player.x`, 2: `player.z <= sample.z`,
  3: `player.x <= sample.x`. Failing any of these moves to the next record.
- `0x800519E4`: if flag 0x7E0 or `0x7E0 | (hw0 & 0x1F)` is set the door is
  locked (returns 1); then, unless 0x7C0 or `0x7C0 | id` is set,
  `0x80063BC8(0,0,0,hw2)` shows a message.
- hw0 bit 15 clear: `0x8001DA8C(0x7B1)` (local door) and return.
- hw0 bit 15 set: 3558 ← sample (x,y), 355C ← (z, facing), 3560 ← hw8/hw9,
  3564 ← hw10/hw11, 3568 ← hw7, 356A ← hw11, 356E ← byte 6, 356F ← byte 7,
  3571 ← 10, 356D ← 0, 3570 ← 2; then `0x8002EE90(0x56,5)`,
  `0x8001DA8C(0x7B0)`, `0x8001F6C4()` (BGM check). **356C is not written.**

## What 0x7B0 starts — VERIFIED (ST00 overlay 0x80100598, table 0x8011ADF8)
The ST00 poll table is `{0x7B0→event 0, 0x604→1, 0x605→2, 0x606→3}`. Event 0
is `0x800485E4` (0x80089024[0]); phases `0x80048620`, `0x80048704`,
`0x80048A40` (table 0x8008A778).
- Phase 1 has 14 substates (table 0x80010D6C). In order: camera at 3558 and
  player target 64 units back along the door heading (3568); flag 0x7B1 (the overlay door object
  `0x80112568..` opens, holding 0x7B2 while it animates and swapping the cell's
  IDX descriptor); forced forward input `0x80048A9C` until event work +0x10
  reaches 37 (its incrementer is not traced); clear 0x7B1; `356C = 1`, 3571 = 10, 3570 = 2; wait for 2; set 3;
  wait for 0 (area loaded).
- Substate 8 (`0x80048898..0x80048974`): `+0x438 = 3560 + (rsin(356A)<<6>>12)`,
  `+0x43A = 3562 - 1`, `+0x43C = 3564 + (rcos(356A)<<6>>12)`, fixed copies at
  +0x428/42C/430, `+0x440 = 0`, `+0x442 = 356A`, `+0x444 = 0`, `+0x448 = 48`,
  player state 3 substate 1. State 3 substate 2 (`0x80041C64..0x80041D2C`)
  copies the current pose to the previous pose and +0x438.. to +0x14.. each frame.
- Substates 9–13 open the Area 2 door, walk in, close it, clear 0x7B0 and set
  0x7B3; phase 2 restores the player and clears 0x7B3; `0x8001D990` then sets
  flag `0x380 + event`.

## Area 1 door data (owner-local package, not committed)
- Door cell IDX 123 (−256,0,−5376), raw6 1: event group ±129, primitive
  y −33..33, code 0x301, flags 0x8000.
- Area 1 aux record 2: `[0x8000,0,0,0x200,−256,0,−5376,2048,−256,−1,6272,2048]`
  → stage 0, area 2, destination (−256,−1,6272), heading 2048.
- Area 2 arrival, substate 8: (−256, −2, 6208), heading 2048. That point is the
  vestibule inside door cell IDX 149 (−256,0,6400), in front of the closed Area 2
  door. Area 2 record 0 (`hw3 0x100`) leads back to (−256,−1,−5248) in Area 1.

## Native mapping
| Source | Native | Label |
|---|---|---|
| 0x800514FC (families 0/1, default interval) | `runtime_player_event_primitive_800514fc` | SOURCE_BACKED_PARTIAL (other families/low bytes 8..85 hold as Unsupported) |
| 0x80051970, 0x80050C2C prefix + case 3, 0x800519E4 | `evaluate_source_door_event_80050c2c` | VERIFIED against instructions; other event types not ported |
| 3558..3571 stores | `SourceDoorRequest` lanes | VERIFIED |
| Call after player contact, flag 0x200 | `RuntimeSession::update_source_door_event` | SOURCE_BACKED_PARTIAL (exact native frame order not traced) |
| Loader spawn 80015EE8 + substate 8 + state-3 copy | `commit_source_door_transition` | arithmetic VERIFIED; immediate scheduling CANDIDATE |
| Event 0 substates 0–7, 9–13, door objects, fades, camera, sound, BGM | not ported | UNKNOWN / unported |
| Net flags after event 0 (0x7B0 clear, 0x380 set) | applied at commit | SOURCE_BACKED_PARTIAL |
| 3DS A → psx Circle 0x2000 (profile 0 +0x134/+0x136) | `make_psx_held` | CANDIDATE button choice |

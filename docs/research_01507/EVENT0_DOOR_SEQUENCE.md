# Event 0 door sequence (0.0.15.7)

Evidence: USA `ROCK_NEO.EXE` and the ST00 PROGBIN overlay (load 0x80100000),
decoded with `docs/research_01503/psx_mips_dis.py` (instructions only). Jump
tables and the one byte table below were read as words from the same images;
no retail bytes are committed. Ghidra was not used: the export has no function
at `0x800485E4` (see `docs/research_01506/WEB_PRIOR_ART.md`).

Registers in event 0: `s0` = player 0x800B51B0, `s1` = event work 0x800C4C48,
`s2` = request lanes 0x800C3558.

## Start and counter — VERIFIED
- `0x8001D990`: if no event is active and `356C == 0`, it runs the stage poll
  (ST00 `0x80100598`), then runs the active event in the same call. When the
  event clears +0, it sets flag `0x380 + id` (0x8001DA38).
- `0x8001DDE4` (start) zeroes the work, including +0x10, and the cursor 0x800C4CEC.
- `0x8001F070`, called at the end of every phase-1 frame (0x80048A1C), adds 1 to
  +0x10 (0x800C4C58). Its cursor branch resets +0x10 only when a record matches;
  the cursor is null for event 0 (nothing in it calls `0x8001F20C`), so the
  counter is frames since phase 1 began. No other writer of 0x800C4C58 exists in
  the EXE. That the null-cursor read (address 2) never matches is CANDIDATE.

## Phase 0 `0x80048620` — VERIFIED
Substate 0: player +6 |= 3, `0x8002EE90(8,128)`, substate 1. Substate 1:
`0x80042238(player)`; on 1, 0x800BC7F0 = `0x8008A9D4[(3568 << 16) >> 26]`,
event +0xA4 = 0x8008A9F4, +12 = 0, phase 1. `0x80042238`: +0x449 = −1,
+0xBD |= 0x40; returns 1 if (+0 & 1) == 0, if state 1 action 0 (or action 14
with byte 0x800C1B93 ≠ 0) is not busy (`0x8004324C`), or if state is 3. Action
0x0D with +10 == 2 sets +10 = 6. Ready clears +0xB4 and stops sounds 22/23.

## Phase 1 `0x80048704` — jump table 0x80010D6C — VERIFIED
| Sub | Address | Action |
|---|---|---|
| 0 | 0x80048754 | camera `0x8001E6C0(3558,355A,355C)`; player state 3 sub 1; target x = 3558 + (rsin(3568)·64 >> 12), y = player +0x16, z = 355C + (rcos(3568)·64 >> 12); +0x440 = 0, +0x442 = 3568, +0x444 = 0, +0x448 = 48 |
| 1, 9 | 0x80048978 | substate++, set 0x7B1 |
| 2, 10 | 0x800487F8 / 0x80048994 | wait while 0x7B2; player +9++; (2 only) event +10 = 0 |
| 3, 11 | 0x800489B8 | forced walk `0x80048A9C` |
| 4, 12 | 0x800489C8 | clear 0x7B1 |
| 5 | 0x80048824 | wait while 0x7B2; 356C = 1, 3571 = 10, 3570 = 2 |
| 6 | 0x8004885C | wait for 356C == 2 |
| 7 | 0x80048878 | wait while 0x7B2; 356C = 3 |
| 8 | 0x80048898 | wait for 356C == 0; camera; arrival (0.0.15.6 note) |
| 13 | 0x800489E4 | wait while 0x7B2; clear 0x7B0, set 0x7B3, phase 2 |
Every phase-1 frame then calls `0x8001EC0C` (command interpreter, not ported)
and `0x8001F070`.

## Forced walk `0x80048A9C` — VERIFIED
0x800C0F96 = 0, then by event +10: 0 → +11 = byte `0x8008AA04[(s8)0x800C1B60]`
(stage 0: 108), player +0xBD |= 0x20, fall into 1; 1 → 0F96 = player +0x124,
done when +0x10 ≥ 37; 2 → +0xBD |= 0x20, fall into 3; 3 → 0F96 = +0x124, done
when +11 < +0x10. Done: substate++, +10++, +0xBD = (+0xBD & ~0x20) | 0x40,
0F96 = 0. Substate 2 sets +10 = 0 and nothing resets it, so the second walk
(after the load) runs states 2/3.

## Input `0x8003C07C` — VERIFIED (input part)
If (+0 & +6 & 1): +0xBD & 0x40 → +0x11C..+0x122 = 0; else & 0x20 → +0x11C =
0x800C0F96, +0x11E = 0F9A, +0x120 = 0F9C, +0x122 = 0F9E (event 0 writes only
0F96); otherwise the real pad. Profile 0 +0x124 = 0x0010 (forward).

## Player states — VERIFIED
- State table 0x80089F94: state 0 `0x8003C7B0`, 1 `0x8003C88C`, 3 `0x80041B8C`.
- State 3: sub 0 snapshots pose → targets (+0x448 = 255); sub 1 `0x800423A0`
  (+0x174/+0x177 = 0, +0x175 &= ~1, +0x104 = +0xAC, +0x100 = +0xA4), +0xBD =
  0x40, sub 2; sub 2 copies pose → previous, targets → pose (+0x14 8 B, +0x1C
  16 B, +0x54 8 B), +0xBD &= ~8, requests animation +0x448 via
  `0x80041DDC(p,anim,0,1)` when it differs or +0xA7 == −1; sub 3 `0x800423C8`
  (+0x175 |= 1), +8 word = 1, +0xBD = 8. Then `0x80051BAC` (not ported).
- State 1 (`0x8003C88C..C92C`) clears +0xBD bit 8 every frame.
- Phase 2 `0x80048A40`: `0x80042314` (0x8002D004 not ported; clears +0x449,
  +0x111, +0x71, +0x73, +0x188..197, +0x168/169; if +0 & 1: +0 = +6 = 3 and
  state 3 → +9 = 3, else +0xBD = 8), player +8 word = 0, 0x800A395C = 1,
  `0x80039820`, clear 0x7B3, event +0 = 0.
- State 0 `0x8003C7B0` re-initialises motion lanes and sets +8 word = 1.

## ST00 door object — state table 0x8011C928 — VERIFIED
| State | Address | Action |
|---|---|---|
| 0 | 0x801124B0 | +68 = `0x80048C60(pos)`; +44 = IDX word (0x8016C008 + cell·12); +48 = `0x8003AC24((*(+44 & 0xFFFFFF)) & ~0x1000)`; state 1 |
| 1 | 0x80112568 | 0x7B1 and player cell == +68 and lock flag 0x7E0 + (s8)+13 clear → state 2, +11 = 16, set 0x7B2, sound 0x105 |
| 2 | 0x801126DC | rotate by +11 << 8; +11--; at 0 → state 3, sound 0x185, clear 0x7B2, IDX word = +48 |
| 3 | 0x801127A0 | 0x7B0 set: if 0x7B1 clear → close (+11 = 16, state 4, set 0x7B2, sound 0x106). 0x7B0 clear: player leaves the cell → close and clear 0x7B1 |
| 4 | 0x801128CC | +11--; at 0 → state 5, sound 0x186, clear 0x7B2 |
| 5 | 0x80112968 | IDX word = +44, state 0 |
`0x8003AC24` scans the HED table at 0x8013A000 (tag, pointer) for the tag.

Owner-local data check (not committed): cell 123 (Area 1 door, −256,0,−5376)
descriptor word0 0x61900 → open 0x60900; cell 149 (Area 2 door, −256,0,6400)
0x61902 → 0x60902. Each descriptor's word0 equals its HED tag, so the native
lookup over cooked descriptors by word0 matches. Cell 155 (0xB1600) also has a
0x1000 pair but is not a door.

## Native mapping
| Source | Native | Label |
|---|---|---|
| Phases 0/1/2, 14 substates, counter | `RuntimeSession::update_source_door_event0` | VERIFIED per routine; frame order (player → event → doors) CANDIDATE |
| 0x80042238, 0x80041B8C, 0x8003C07C (input), 0x80048A9C, 0x80042314, 0x8003C7B0 | `source_player_*`, `source_event0_forced_walk_80048a9c` | VERIFIED, unported callees listed above |
| 0x8008AA04 | `source_event0_walk_limit_8008aa04` | VERIFIED |
| Door object states 0–5 | `source_door_object_step`, `source_door_object_init_801124b0` | VERIFIED; lock flag 0x7E0 + id not read (id is uncooked object data) |
| Door object spawn | lazy, in the player's cell when 0x7B1 is set and an area aux record lies in that cell | CANDIDATE (spawn list not cooked) |
| IDX word write 0x8016C008 + cell·12 | copy-on-write `door_map_` descriptor_index | CANDIDATE that 0x8016C000 is the live IDX the collision reads (scratch 0x1F80004C) |
| Substates 6–8 (load) | `commit_source_door_transition`, one counted frame each | CANDIDATE: the PS1 CD load may spend more frames, which would shorten the second walk |
| Camera 0x8001E6C0/0x8001EC0C, sounds, door leaf model and rotation | recorded or not ported | UNKNOWN / unported |

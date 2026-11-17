# Data's menu (ST03 class 0x11, event 0xFF) — research

Evidence: static disassembly of ST03.BIN (`0x80100000` = file + 0x800) and
ROCK_NEO.EXE, plus the PSXRecomp oracle from `m3_01_st03_crash_control`
(Data = NPC slot 5 at (-5196,-1,1269); player placed at (-5030,-1,1269),
heading 0x400, Circle). Labels: SOURCE_BACKED_PARTIAL unless noted.

## NPC behaviour 7 (generic NPC 0x80107B84, jump table 0x80116E34[7] = 0x80109CF8)
The talk start (0x8010A344) sets state_09 = 7 for class 17. Behaviour 7 is a
sub-state machine on +0x34B (table 0x80117040):

| +0x34B | handler | what |
|---|---|---|
| 0 | 0x80109D34 | +0x34C = heading, +0x34A = pose; player +0 &= ~2, player action +9 = 0x10, phase +0x0A = 0; camera 0x80039FAC((npc.x, npc.y-0x30, npc.z), 0x100, heading to player, camY+0x6C) (event 0xFF mode, 0x800A3959/0x800A3962 = 0x40); pose = idle request (0x8010A44C); +0x34E = \|turn to player\| >> 3 (0x80031BEC, limit 0x800); +0x34B = 1 |
| 1 | 0x80109E94 | when `0x8003A13C` (0x800A3962 sign bit set = camera settled) is false: start the talk `0x80063BC8(npc+0x14, 0xD, 0, +0x0C)`, +0x34B = 2; every frame heading += turn toward player (step +0x34E) |
| 2 | 0x80109F1C | heading += turn; while 0x800A87B0 (message busy) != 0: if main phase 0x800C1B10 == 6 -> +0x34B = 3; when it is 0: +0x348 &= ~4, +0x34B += 2 (-> 4), pose = idle, `0x80039E18` (leave event 0xFF, camera back) |
| 3 | 0x80109FD4 | when main phase == 3 again: clear windows (0x80053A30(0)), reopen message block 0xDC in slot 0 (0x80053A08(0, 0xDC)), +0x34B -= 1 (-> 2) |
| 4 | 0x8010A02C | turn back to +0x34C (0x80031D5C, step +0x34E); when done (or +0x0D & 0x80): player action/phase = 0, player +0 \|= 2, pose = +0x34A, state_09 = +0x0D & 7, +0x348 \|= 2 |

The port had `talk_unported` for this (source_st03_objects.cpp).

## Messages (ST03_00.BIN message bank: pointer table at file 0xAA800, 251 blocks)
Data's talk block is +0x0C; its scripts end by jumping to the menu block
**0xDC**: text "What do you want to do?" then `97 80 DF DE E4 FF` (MULTI,
rows Recharge Energy / Save / Tell me... / Cancel):

* Recharge Energy -> block 0xDF
* Save -> block 0xDE: `AC 08 06  AC 09 00  AC 0A 00  AC 0B 00  8A 01 00  95` —
  opcode 0xAC (handler 0x80055C80) writes byte `arg2` to the byte pointed to by
  `0x8008D0D4[arg1]`; indices 8..11 are `0x800C1B10..13`, so Save sets
  **main phase 6, sub 0** and the three following bytes to 0.
* Tell me... -> block 0xE4 (second menu: rows -> 0xE8 / 0xE7 / 0xE6 / 0xDC; text
  blocks 0xE5..0xE8, ending `BF 00 E4`)
* Cancel -> 0xFF (continue after the MULTI, then the block's END/CLOSE).

New opcodes seen here:
* **0xBF** (0x80057408) JUMP: block = arg2 (bank arg1 = 0 here), `BF 00 DC 00 00`
  returns to the menu.
* **0xB5** (0x80056280) 4 args {ff, e0, e1, e2}: A = `0x8005DA1C(0)` (shield
  ok), B = `0x8005DAEC(0)` (special weapon full): A=0,B=0 -> arg1 (0xFF: continue,
  all three lines); A=0,B=1 -> block e0 (life + shield); A=1,B=0 -> block e1
  (life + weapon); A=1,B=1 -> block e2 (life only). Sets the read pointer to the
  block start and timer = 5.
* **0xB2 / 0xB3 / 0xB4** (0 args): per-frame recharge steps, each returns
  "continue" (advances 1) only when done, else 0 (the window waits):
  * B2 `0x8005DA78(1)`: life (0x800B521E) += 2, to max (0x800B5260); done when it
    reached max.
  * B3 `0x8005DAEC(1)`: special weapon energy `player + 0x4FA + 8*weapon(0x800B5322)`
    += 0x10 up to the max from `0x80037604`; max 0x7FFF = nothing to charge (done).
  * B4 `0x8005DA1C(1)`: if byte 0x800BBD87 != 0: clear it, 0x800BBD85 = 3,
    0x800BBD81 = 6 and return 0 (done next call), else done.
  Texts: "Life Gauge fully charged." (b2) / "Special Weapon ... " (b3) /
  "Life Shield repaired." (b4); then NEXT and `BF 00 DC` back to the menu.

Oracle check: HP 40 -> 80 within 4 frames of confirming Recharge (the handler
runs more than once per frame in the oracle's turbo mode; rate unmeasured).

## Save screen = main phase 6 (0x800164B4 -> 0x800665FC)
Oracle screens: "Save Data" / "Save to which Memory card?" (Memory card 1, 2,
Cancel) -> "Memory card 1" / "Checking data..." (~25 s in the oracle) -> slot
list "1:No data!  00:00:00" x5 + Cancel (a used slot shows "Cardon Forest
00:15:37" = stage name + play time) -> "Do you want to save? Yes / No" ->
"Saving..." -> "Save complete" (NEXT). Phase 6 sub 1 -> main phase 3, sub 2.

Serializer `0x80065064` builds the block at `0x800B5DB0` (8 KiB, written to the
card file at block offset 0): header init `0x80064CB8` from table 0x80097A64
(title "MEGAMAN LEGENDS […" in Shift-JIS with the play time, icon, CLUT), then
fields copied from `0x800C1B10`+x, the player work `0x800B51B0`+x and block
copies (`0x800176DC`, count in words): `0x800B5624`->+0x1C80 (4),
`0x800B56A4`->+0x22C (8), `0x800BE378`->+0x380 (8), `0x800BD3B8`->+0x480
(0x70), `0x800BC9B8`->+0x1280 (0x50), `0x800C1B94`->+0x1D40 (15 bytes).
The first oracle save is in a scratch copy of the card (not committed).

Not ported / open: camera (event 0xFF), player talk pose, Data's own 3DS
save-slot UI, "Checking data", memory card 2, load/continue.

## Port status (2026-11-17)
- Behaviour 7 in `source_st03_objects.cpp` (`behaviour7_80109cf8`); message ops in
  `RuntimeSession::update_dialogue`: 0xAC (phase-6 write = Save), 0xB2 (life +2 per update),
  0xB3/0xB4 (done at once: no special weapons/shield yet, CANDIDATE), 0xB5 (both read
  "done" -> block arg4, life only, CANDIDATE), 0xBF jump, 0x99 with slot != 0 recorded as
  `side_window_block` (header window not drawn, CANDIDATE). Parser: 0xB5 has 4 args.
- Save writes `sdmc:/3ds/Legends3DS/save_01516.l3sv` (native L3SV, checkpoint id
  0x80000000 | stage << 8 | area) through `set_save_writer`; no card/slot/confirm screen
  (CANDIDATE). During an ST03 talk the pad is held off (stands in for player action 0x10).
- Host: `test_source_data_menu`, `test_source_st03_objects`. Azahar run 41 (Old 3DS mode,
  `diagnostic_stage_01516.txt` = `stage 3 area 1 -4566 -1 1402 1255`, route
  `autoplay/st03_data_save.txt`): talk block 9 -> menu 0xDC waits for the choice -> Right ->
  Save (block 0xDE) -> file written -> Data reopens 0xDC (twice); player stays at
  (-5010,1299); the written file loads (stage 3 area 1, HP 80/80, flag 3 set).
- Flags (oracle, `m3_01`, own instance, 2026-11-17; flag words 0x800BE378 (0x000..0x7FF) and
  0x800BE438 (0x600..0x9FF) diffed directly, the same regions compare_recomp_3ds.py arms):
  0x608 is clear before and set by the talk, as on the 3DS (block 9's own 0x9A).
  Recharge (HP 40 -> 80) and the whole Save (card 1, slot 1, "Save complete") change no flags.
  VERIFIED_1TO1 for these flags. After "Save complete" four more Cross presses left main phase at
  0x0600 within ~6 s (the return to phase 3 was not observed).

## Save block (slice 2a, 2026-11-17)
`build_source_save_block_80065064` (src/flow/source_save_block.cpp) builds the 8 KiB block
byte for byte as 0x80065064 + 0x80064CB8 + 0x800679B8/0x80067D10 + 0x80065384 lay it out; the
EXE header ranges (icon, CLUT, title, digit tables) come from frontend/option.l3ex.
`RuntimeSession::source_save_block_inputs` fills play time (frame counter, CANDIDATE), money,
stage/area, Turn, the player work with life/max life (+0x6E/+0xB0, 0x8005DA78) and the flags;
main.cpp adds the other OPTION bytes. Not modelled (written as 0, UNKNOWN): the rest of
0x800C1B10.., 0x800BD3B8 (0x8003B140's per-stage table), 0x800BC9B8; seeds use a local LCG.
Azahar run 42: `save_01516_slot1.blk` passes mml_retail_save_verify, its icon/CLUT bytes equal an
oracle-written card's, the title reads "MEGAMAN LEGENDS[1]00:01:42" in the same format.
Still open: the card/slot/confirm screen (phase 6, 0x800665FC).

## Save/slot screen 0x800665FC (research, not ported yet)
Work struct 0x80098910: +0 u16, +2 u16 mode arg (a0: 1 = save), +4 state (table 0x8008FD10:
0x80066664 init / 0x800666D8 / 0x80066714 run sub / 0x80066750 exit -> returns 1 so phase 6
goes to 3/2), +5 sub (table 0x8008FD20: 0x80066764, 0x8006689C, 0x80066D60, 0x80066E48,
0x80066F40, 0x80067174, 0x8006779C), +6 inner step, +7 "bad data seen", +8/+0xA pad
pressed/held (0x800C0C2A / 0x800C0C26), +0xC timer. The windows are message scripts in slots
0..2 opened with 0x8005382C from the message pointer table (save: 0x8008CAEC/F0/F4; load:
0x8008CB40/44/48), choices come back through 0x80098A58 (bit 30 | row).
* sub 0 (0x80066764): title, "Save to which Memory card?", Memory card 1 / 2 / Cancel. Row 0/1 ->
  gp+0x11E4 = card, sub 1; Cancel -> state 3.
* sub 1 (0x8006689C): "Memory card n" + "Checking data..." (0x8008CAF8+card, 0x8008CB00), 15-frame
  wait, then reads the first 0x180 bytes of the five files (names 0x8009 80AC[i], 0x80064EA4)
  into 0x800BAF38 + i*0x180 and patches the list strings (pointer tables 0x8008D168 / 0x8008D184):
  valid -> stage name 0x8008CE94 + 13*stage and the time glyphs from +0x101; empty -> "No data!"
  0x80097FAC and 0x80097FFC; broken -> 0x80097FC0, +7 = 1. +0x109 per record: 1 ok, 2 empty, 3 bad.
  Result codes from 0x80064EA4 go through the jump table 0x80011410 (6 cases).
* subs 2..6 (slot list, confirm, writing, complete, errors) still to trace.
Port plan: run these scripts on the existing MessageWindows engine (mml_message_window.cpp,
already used by OPTION and the Start sub-screen) with the 3DS save file as card 1 and card 2
absent; draw on the bottom screen like the sub-screen (CANDIDATE placement).

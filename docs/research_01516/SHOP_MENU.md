# Junk shop menu (ST04 area 1) — research, not ported

Source: the message bank of ST04_01.BIN (pointer table at file 0x4D000), static reading of
USA ROCK_NEO.EXE. Static only (screen locked: no oracle/Azahar check yet).

## Blocks
* 0x00 / 0x19 / 0x22: the counter. Flag tests 0x9C (0x524, 0x2F0), side windows 0x99 slots 2/3
  (blocks 1, 2/6), 0xAC 0x14/0x16 = 0 (setting-table bytes 20/22), rows Buy / Sell / Refill canteen /
  Cancel, MULTI type 0x90 -> blocks 0x05 / 0x16 / 0x21 / 0xFF (0x22 has no canteen row).
* 0x05: Items / Parts / Cancel (0xA7 header, 0xAC 0x0F..0x13 = 0,0,0,1,5), MULTI 0xA0 -> 0x0F /
  0x0A / 0x00. 0x16 (Sell): sets flag 0x2EE, then 0xBF -> 0x05 (same menu, sell mode).
* 0x0A..0x0E (parts) and 0x0F..0x13 (items): the list windows. 0xAC 0x0E/0x0D, 0xB8 (list setup,
  ids from the stage's table), rows of 0xB7 + 0x90, 0xB6, MULTI type 0x40 over the rows, 0xBC / 0xBB
  (price and stock lines), HALT.
* 0x01: the money window (0xBA with a 7-glyph field). 0x14 / 0x20: Yes / No with 0xB9 branches.
* 0x15 / 0x1E / 0x1F: "not enough money" (0x1F adds the free-refill line, 0xC4). 0x1B: "I'll sell you
  ... OK?" (0xC2, 0xC5). 0x1C: already full. 0x1D: refilled (0xC3, 0xC4).
* Part description windows 0x23.. ("Buster Parts", ATK/ENG/RPD lines) and 0xC1 name lines 0x4D...

## Opcodes the port lacks (exec table 0x8008A91C)
0x90 0x8005497C (34 insns), 0xB6 0x800543E0 (6), 0xB7 0x80056778 (358), 0xB8 0x80056D10 (232),
0xB9 0x800570B0 (29), 0xBA 0x80057124 (24), 0xBB 0x80057184 (38), 0xBC 0x8005721C (43),
0xBD 0x800572C8 (80), 0xC0 / 0xC5 0x800543B0 (6), 0xC1 0x800576C4 (32), 0xC2 0x80057744 (46),
0xC3 0x800577FC (74), 0xC4 0x80057924 (64); plus their draw handlers (table 0x8008AAC4).
They need the stage's shop tables, the zenny 0x800C1B2C, the canteen state and the item/part
ownership words the sub-screen already models (0x800BE420, 0x800B5600/04).

Plan: run the shop's blocks on MessageWindows (like the save screen), port the opcodes above with
host tests from the bank's real blocks, then wire the talk to the junk-shop owner once ST04 loads.

## Port status (2026-11-20)
Ported on MessageWindows: 0x90, 0xB6..0xBD, 0xC0..0xC5, 0x8D, 0x94, 0x9A/0x9B, decor 0 (as 8), the
number printer 0x8005BF10; prices/tables cooked into option.l3ex (0x8008FD3C.., 0x8008AF24..).
Host probe on the real ST04_01 bank (local only, not committed): the counter (block 0 -> 0x22),
Buy -> Items -> the one-row list (0x13) -> Yes -> 0xBD bought (zenny 1000 -> 950) -> back to the
Items/Parts menu, with no unported opcode on the way.
Open: the talk that opens block 0 (the shop NPC's behaviour in the ST04 overlay), the bottom-screen
host like the save screen, applying shop_bought/shop_sold to the player (0x23 life max, 0x24/0x25
canteen, 0x80061320 re-equip), Azahar once ST04 runs there, and the recomp flag compare.

# Message windows and the OPTION menu (2026-11-14)

Port: `include/legends3ds/mml_message_window.h`, `src/flow/mml_message_window.cpp`
(host test `test_mml_message_window`, probe `l3probe_option_menu`). Cooks:
`tools/cook_option_menu.py` (EXE ranges → `frontend/option.l3ex`) and
`l3cook_font_materials` (KAIFONT.BIN → `materials/font.l3mc`). Front-end hook:
`run_frontend_title` (state 6). All static-disassembly evidence unless noted.

## Slots (VERIFIED_1TO1 for the fields below)
Five 196-byte slots at 0x800BB6B8. 0x80053A30 clears them; 0x8005382C(slot,
data, index) opens one (refused while 0x80098A58 & 0x400000; opening any slot
but 4 also clears slot 4). Fields the port uses: +0 flags, +4 timer, +8/+10
window x/y, +16/+18 pen, +20 draw pointer, +56 scroll, +59 lines, +61 OT
entry (3), +62 colour, +63 speed, +68 index base, +72 page, +108 read
pointer, +112 default colour, +113 cursor, +114 rows, +115 row being drawn,
+116 cursor byte address, +120 glyph brightness, +121..123 remembered
cursors, +124 decor, +125 decor state, +126/+127 width/height in characters,
+136..+150 frame corners, +180 margin, +184/+186 pad held/pressed
(0x800C0C26/0x800C0C2A), +188 repeat counter, +194 index.

## 0x80053B40 per frame
For each open slot: copy the pad (unless 0x80098A58 & 0x40000, which a 0x97
in an earlier slot sets), then execute: glyph bytes (< 0x84) count the timer
down by the speed (+63, halved while Triangle is held), play sound 132
unless 0xA1 set 0x20000000; control bytes go through the exec table
0x8008A91C (op * 4; nonzero return keeps executing). With speed 0 the slot
runs until a handler sets 0x10000000. Then the draw pass walks +72..+108:
glyphs are SPRT 12x12 (code 0x64) at the pen, u = (g % 21) * 12,
v = (g / 21) * 12, CLUT 0x80097F30[colour], brightness +120, advance by
0x8008AE7C[g]; control bytes go through the draw table 0x8008AAC4. Then the
frame (flags 0x04000000) through 0x8008AAE4[decor]. Last, the font draw mode
GetTPage(0,0,960,256) at OT entry 31 (0x80054240). gp+4596 is 0x80098A58:
bit 31 any window, 0x08000000 >> slot open, bit 30 + low byte the last choice.

## Opcodes ported (exec / draw)
| op | exec | draw | meaning |
|----|------|------|---------|
| 0x86 | 0x8005457C | 0x8005459C | newline: pen x = window x, y += 12 |
| 0x89 | +2 | 0x80054694 | colour = arg |
| 0x8A | 0x800546B0 | +3 | speed = timer = u16 arg |
| 0x8C | 0x80054798 | +7 | window x, y (u16), width, height (chars) |
| 0x93 | 0x80054ADC | +3 | decor type, decor exec 0x8008AAC0[type] (2/4: 0x8005B85C) |
| 0x95 | 0x80054BAC | +1 | halt (returns 0, no advance) |
| 0x96 | 0x80054BB4 | +3 | selection row |
| 0x97 | 0x80054E54 | 0x80055304 | choice (below) |
| 0x99 | 0x80055344 | +3 | open message (slot s8, id) via 0x8008CACC |
| 0xA0 | +3 | 0x800555F4 | pen x += u16 |
| 0xA1 | 0x8005563C | +1 | no per-character sound (0x20000000) |
| 0xAD | 0x80055CC4 | 0x80055CFC | option row (below) |
| 0xAE | +3 | 0x800560D0 | brightness = setting[a] == b ? 128 : 64 |
| 0xAF | 0x80056128 | 0x80056148 | pen x = window x, colour = default, y += arg |
| 0xB0 | +2 | 0x80056180 | brightness = arg |
| 0xDB | +5 | 0x80058D64 | pen = (u16 x, u16 y), absolute |
| 0xED | +1 | 0x8005A858 | 12-px space (the cursor slot) |

0xAD row (args nav, -, setting, help slot, help id[2]): the draw handler
writes 0xED after the row; on the cursor row (and no 0xC00000) it reads the
pad: Up/Down → nav high/low nibble (held: after 13 updates, every second
update), Left/Right → setting 0/1 (sound 130 on change), Cross → toggle
(sound 130, Cross consumed) except on the last row; then opens the help
message (slot arg, id by the setting's value), writes the cursor glyph 0x60
and sets 0x800000 (one cursor row per pass); sound 129 when the cursor moved.

0x97 choice (type byte, one target per row): first run sets 0x8000 for type
& 0x80 (Triangle cancels), the cursor, timer 5. While rows are 0xAD rows
(0x4000) only the last row confirms; Cross confirms (sound 130), Triangle
(with 0x8000) or Start in main phase 4 (0x800C1B10) cancels to the last row
(sound 131). The choice goes to 0x80098A58 (bit 30 | row), the cursor byte
becomes 0xED, and target 255 continues after the command (OPTION: `8A 01 00
95`, halt). The front-end state 6 sees bit 30 and fades back to the menu.

Frame (decor 2, 0x8005ACA8 with primitives from 0x8005BCE4): four POLY_G4
bevel edges (greys 0x80097F78.., 0xF0 outer → 0x80), then two POLY_F4
semi-transparent backgrounds from 0x80097F6C[6 * 0x8008AF18[decor]]: one
with ABR 0, one with ABR 2 (OPTION: black, so a 50 % darkening).

## OPTION data (USA)
0x8008B69C: title window (0x99 01 25 → "Options" at y 32), menu window at
(28, 62) 22 x 9, rows Turn / Free Look / Sound / Map / Lock-On / Close,
help window in slot 2 at y 188 (ids 0x27..0x31, one per row and value).
Settings pointer table 0x8008D0D4: 2 → 0x800C1B64 Turn, 3 → 0x800C1B65
Free Look, 4 → 0x800C1B68 Sound, 5 → 0x800C1B69 Map, 6 → 0x800C1B66 Lock-On,
7 → 0x8009879C (Close, unused byte). 0x80013360 zeroes 64/65/68/69 at boot.

Readers: Turn 0x8003BF2C (control profile 0x800425FC; also 0x80060C00,
0x80063690), Free Look 0x80041F1C (axis pair 0x80041EF4 orientation),
Lock-On 0x8003F224 (both inside the look mode 0x8003EF04), Sound
0x8001A344.. (mono mix), Map 0x8005DE28 (map draw; Off skips it).
Port: the 3DS pad is the PS1 pad 1:1 (D-pad / circle pad = directions,
L/R = L1/R1); Turn installs control profile 0/1 (table 0x8008A04C, checked
against the EXE), so profile 0 (the default: 0x80013360 zeroes the byte,
and the oracle's New Game has 0x800C1B64..69 = 0) turns on L1/R1 and slides
(0x8003D124 strafe) on the D-pad, profile 1 the reverse
(`test_player_turn_profile`). Diagnostic routes turn with the profile's turn
buttons. Sound → NativeSfx mono; Map → bottom-screen map blank. Free Look
and Lock-On are stored but their reader (look mode) is not ported.

## Font (KAIFONT.BIN)
The USA message font is KAIFONT.BIN ("..\bg\char\kaifont.dat"): the oracle's
VRAM at (960, 256) 64x256 equals it word for word during play. FONT.BIN has
the same layout but other glyphs at the cursor (0x60) and the button icons
(0x4E/0x58/0x5A): the first Azahar check with it showed kana there.
Type-3 resource: 0x8000 bytes after the 0x800 header
= 16 tiles of 32x32 halfwords, two tiles across, covering VRAM x 960..1023,
y 256..511. The 16 CLUTs (x 960 + 16 * (c & 3), y 508 + (c >> 2)) are the
page's own last rows; the oracle's VRAM at (960, 508) 64x4 matched the
detiled file (read-only `vram_peek`, 2026-11-14). Pixels pack two 2-bit
planes; CLUTs 0..3 show the low plane, the row-510/511 CLUTs the high one.
The cook keeps colours 0..3 (OPTION uses 0 and 3).

## Azahar (Old 3DS mode, build pass-option-menu-01516)
`docs/validation_01516/autoplay/option_menu.txt` + `diagnostic_title.txt`:
state 6 at frame 239 (215 packets), Right on Turn, A on Free Look, Close
→ state 4 at frame 436, NEW GAME → `# option_settings,turn=1,free_look=1,
sound=0,map=0,lock_on=0`. A window capture on the menu showed the original
layout: "Options" title, rows and values (lit/dim), hand cursor, L1/R1 icons
and the help window ("Down arrow moves viewpoint up." for Free Look
Reverse), with the Servbot parade darkened behind the frame.

Turn/slide (build after 470fb92, spawn `1 2822 -1 -4833 0`, Turn 0): 3DS L
held (psx_held 0x400) turned in place (heading 0 -> 4084, x/z unchanged);
D-pad Left held (psx_held 0x80, action 2) slid sideways (x 2822 -> 4575,
heading fixed at 28, z -4833 -> -4917).

## In-game Start sub-screen (bottom screen, 2026-11-15)
Main phase 4. 0x8005EC34 draws the tiles, then page task 0x8008DBB0[page]
(byte 0x800A38F0): 0 0x8005EC80 opens message 0x32 (0x96 rows, 0xD6 help
ids 0xDD + row) and status layout 6; 1 0x8005ECE0 draws the status bar
0x80061E0C and dispatches the choice (0..3 -> pages 2..5, 4 -> 6 Options
0x80060B00, 5 Close -> 8 exit 0x80060DB8; Select -> 7, Start -> 8). The
Options page clears the windows and layouts, opens message 0x26, and on a
choice or Start (sound 131) reinstalls the control profile from 0x800C1B64
(0x80060BFC), then returns to page 0. Status frame: sprite list 0x8008E09C
(layout 6, handler 0x800640F0: mode 1 blink, 2 TIME colon, 3 LIFE box grows
with max HP). Status bar: cells at x 49 + 5i (full/half/empty u 0x70/0x78/
0x80, v 0xB8), zenny and h:mm digits via 0x80062028. OT: windows/status
use 0x80098934 + 112 + 4 * layer (front-end entry 28 + layer). The tile art
is 61 rows in the 64-row SPRT; the sub-screen never clears the frame, so the
previous frame fills the gap (the CPU raster keeps its buffer).
Port: SubScreenMenu (pages 0/1/6/8; 2..5 and 7 reopen the menu, CANDIDATE),
rasterized on the CPU into the bottom framebuffer. Azahar (Old 3DS mode):
Start opened it at frame 189, Options -> Turn Right -> Close -> Start exit,
`# subscreen,open=0,turn=1`; the game stayed paused (frame 189).

## Open
- Sounds 129/130/131 come from the title banks; 132 (per character) is
  suppressed by 0xA1 in all OPTION messages.
- Message-tick vs front-end-thread order is CANDIDATE (front end first).
- The voice calls (0x8001D494/0x8001D878, slot bit 0x40000) are not ported.
- Not oracle-compared frame by frame (the oracle was in use by other sessions).

## Sub-screen item pages (in progress, 2026-11-15)
- Pages 2..5 load their data with the loader queue (0x8001D440 cmd 6, file
  table 0x80082CD0 {sector, bytes, type}): page 2/3 entry 204 = SUB_KEY.BIN
  (SUB01.TIM + `..\MESS\STFF_FE.MSG` loaded to 0x801F2000, 248 messages),
  entry 205 = SUB_WPN.BIN (same size; weapon pages). Leaving loads entry 22 =
  EXIT_SUB.BIN (KOUKA00/KOUKA04/SH1100 effect pages back into VRAM).
- Normal Items 0x8005ED9C (jump table 0x80011264): 0 clear + load 204;
  1 open(1, 0x801F2000, 20) = "Normal Items" title, which opens 21 (the list)
  in slot 2; 2 wait for a pick (item byte 0x80098B30, 255 = cancel, sound
  131) else 0x80060E64 (exit -> sub 7) / 0x80060EC0 (L1/R1 pages); 3 confirm
  per item: 64 -> 80/88 (0x8005D95C), 65 -> 58/59/82 (table 0x8008DB08,
  0x80037604), 66 -> 84, 67 -> 86, else 90 (< 88) / 91; 4 on yes: 64 81 +
  0x8005D9C4, 65 83 + 0x80037594, 66 85 + 0x8005EBB4(1800), 67 87 +
  0x8005EBA4(900), sound code 1344 + n (0x8001DCD0 at sub 6); 5/6 wait for
  the window, back to 2; 7 wait loader -> page 0.
- List message 21: 0xAC 1A 2C (setting 26 = 0x800989D4 <- 0x2C), 0xDF
  (exec: list 0x800BE2F8 = 255 x128, scroll gp+4408 = 0; draw: owned item
  ids from the 64-bit field 0x800BE420, bit 0x80 >> (id & 7)), 20 x 0xDD
  rows (args left/right/up/down) + 0xE0 (item name) in two columns, 0xDE 03
  A4, 0xDC. Picks go through 0xE8 = the 0x97 handler (low byte = list id).
- Handlers still to port: 0xDD draw 0x80058F18 (scrolling list), 0xDE draw
  0x80056558, 0xDC exec 0x800592FC, 0xE0 0x800594CC/0x80059530, 0xD0
  0x80057AD0/0x80057B24, 0x9F 0x80055688, 0xA9 0x80055C1C, 0x84 0x80054450,
  decor 8 exec 0x8005B98C.
- Normal Items: ported (d615644). Special Items 0x8005F210: 0 clear + load
  204; 1 open(1, bank, 25) ("Special Items" -> 26 grid) + layout(0, 2);
  2 choice -> code byte 0x80098B2E, use dispatch table 0x80011284 (18 codes
  -> 0x8005F3D0/F608/FAE4/FBB4/FFBC: story events, NOT ported) else
  0x80060E64 exit (page 0) / L1/R1.
- Grid message 26: 18 x 0xD7 cells (exec 0x80058788: rows++, flags
  0x400800, +5; draw 0x800587F8: up/down/left/right bytes, repeat, no input
  while slot 0 is open, cursor glyph 0x60 unless 0x80098A58 & 0x20000),
  0xD6 03 84 help, 0xD9 choice (0x80058AEC: Cross -> 0x40000000 | cursor,
  Triangle -> 0x20000000 | rows-1; 0x20000 mirrors slot 0 open).
- Layout handler 1 0x8006430C (list 2 = 0x8008DE1C, records {u16 cell |
  mode << 8, s16 flag, s16 dx, s16 dy, u8 u, u8 v, u16 clut, u16 w, u16 h}):
  E1 0x0C (SUB01.TIM page), SPRT at (cell % 6 * 44 + 34 + dx, cell / 6 * 40
  + 60 + dy); n = flags(f) + flags(f+1) + flags(f+2) (0x8001DA58); modes
  (table 0x800113D8): 0 icon if f else empty box (u/v 0x80/0x40, CLUT
  0x3C48); 13 icon if f; 1 icon if n else box; 2 icon if n; 3 u + (n-1)*8;
  4 u + (n-1)*32 else box; 5/7 tens of byte 0x800C1B8C/8D (>= 10, needs f);
  6/8 ones; 9..12 icon if f and 0x800B531A / 531B & 2 / 531B & 1 / 531C.

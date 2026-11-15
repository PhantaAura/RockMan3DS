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

## Open
- Sounds 129/130/131 come from the title banks; 132 (per character) is
  suppressed by 0xA1 in all OPTION messages.
- Message-tick vs front-end-thread order is CANDIDATE (front end first).
- The voice calls (0x8001D494/0x8001D878, slot bit 0x40000) are not ported.
- Not oracle-compared frame by frame (the oracle was in use by other sessions).

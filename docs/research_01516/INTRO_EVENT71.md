# Pre-title intro: ST01 event 71 (scope, 2026-10-31)

Status: research only. Oracle boot sequence added 2026-10-31 (below) (static read of R3_ST01.BIN; extracted owner-locally
from `CDDATA/DAT/ST01.BIN`, not committed). Nothing ported yet.

## Handler 0x8010049C (11-entry table at 0x80100004)
Same machinery as ST00 event 1 (`OPENING_EVENT1.md`):

| Phase | Code | Effect |
|---|---|---|
| 0 | 0x801004D8 | `0x80042238(player)`; then `0x8001E6C0(0,0,0)` camera init |
| 1 | 0x80100504 | `0x8001B33C`, `0x8001D58C(0x2000001E)` (CD/XA command, id from the request table, CANDIDATE: narration/music stream) |
| 2 | 0x80100520 | wait `0x8001D878`; +0C = 5 |
| 3 | 0x8010053C | count down +0C |
| 4 | 0x80100564 | fade `0x80017C30(2)`; constructor `0x80100400` |
| 5 | 0x80100584 | busy 0 and (timeline record byte == 0xFF or `0x8001DE84` Start skip) → fade 10, phase 10; every frame: callback `0x8010045C`, camera `0x8001EC0C`, timeline `0x8001F070` |
| 10 | 0x801005D0 | same three calls; busy 0: `0x8002EE90(86,128)`, `0x8001D494(0,1,0)`, `0x80042314(player)`, `0x800A395C`=1, `0x80039820`, `0x800C1B92` &= 0x7F, event off, `0x80098AA9`=0, `0x80098AA8`++, `0x80012F78(0x80013420)` (title thread) |

`0x8001DE84` only honours Start when `0x800C1B6A` != 0 or flag 896+71 = 967
is set; the opening's oracle flags show 967 set after a New Game, so on a
first boot the intro cannot be skipped (CANDIDATE until traced).

## Constructor 0x80100400
`0x8001E6C0(0,0,0)`; camera directory `0x800BC7F0` = `0x80100F30`; event+A4 =
timeline `0x80100F7C`; one `0x8001EAE8` record at `0x80100F1C`
(`00 0A 47 20 … 5C 03 D4 FE`; family/subtype and the actor it spawns:
UNKNOWN until traced, CANDIDATE the airship, class init `0x8010080C`, path
`0x801006A8`).

Camera program `0x80100F30`: commands 01, 20, 12 (0,0,0), 13 (0,0,0),
10 (0,0,0), 11 (0,0,0x07D00000), FF (static shot; same interpreter as
event 1, all opcodes already ported).

Timeline `0x80100F7C`: one record, 3100 ticks (0x0C1C), index 0, callback
`0x801002E4`; then the 0xFF terminator (phase-5 end condition).

## Callback 0x801002E4 (background layer fade; not the crawl)
Drives the message/text window state at `0x800BBA90`:
- tick 0: `0x800BBA9C` = 130, `0x800BBA90` &= ~2, window coords
  `0x800BBB20` -= 56 and `0x800BBAE8` -= 16; event+24 actor action 1;
  event+0C = 3.
- tick 2200: `0x800BBA90` |= 2.
- after tick 2200, while event+06 == 0: every 3 ticks `0x800BBA9C`-- ;
  at -11 event+06++ (crawl finished).
The text itself and the window renderer belong to the message system;
which message/page it shows is not yet traced (oracle needed).

## Stage resources (ST01.BIN)
`R3_ST01.BIN` (7004 B code), `ST01_MAP.IDX/MDT`, `ST01.STG/HED`,
`ST01_00.EBD`, `EM0A00.TIM` (ST00/AR04), `OPENG0..3.TIM`, `ST01BG00..02.TIM`.
Sky/sea gradient: `0x80100920` / `0x80100A70` (POLY_FT4 lines, see
`FRONTEND_TITLE.md`).

## Port plan (one commit each, host test each)
1. Cook ST01: map/render/EBD/materials, constructor/camera/timeline profile
   (like L3P5/L3P6), XA stream for command 0x2000001E.
2. Boot path: intro before the title (stage 1 area 0), event 71 phases
   0..5/10 through the existing pipeline with an ST01 profile; hand-off to
   `run_frontend_title`, coordinating fade 1 with the main session.
3. Sky/sea gradient draw, the constructed actor (airship) and its path.
4. Text crawl: callback `0x801002E4` + the message window it drives.
5. Oracle per phase (frames, actor, crawl counter) and an Azahar run.

## Oracle: cold-boot attract sequence (no input, US disc, 60 Hz frames)
| Frames | Event | What is on screen |
|---|---|---|
| 1254-7508 | 71 (ST01), phases 1..5, then 10 | sky/sea backdrop, small airship crossing right to left, white text crawl ("In a world covered by endless water...") fading at top and bottom |
| 7508-9562 | 71 phase 10, then title | title screen "PRESS START BUTTON" |
| 9732-12719 | 10 (ST07), phases 0..2 | Bonne attract scene: the Gesellschaft over the sea, Servbots in the cockpit |
| 12719-14764 | 10 phase 2 (event off) | title again; a Servbot runs across it |
| 14838- | 0, then 71 | attract loop restarts with the narration |

Event 10's overlay is `r3_st07.bin` in `ST07.BIN` (load 0x80100000, 35904 B;
signature matched against oracle RAM at 0x80100000). ST07 also ships
`ST07_MAP.IDX/MDT`, `ST07.STG/HED`, `ST07_00/05/05B.BIN`. Event 10 is a
separate port (its own stage cook, constructor/timeline, Bonne/Servbot actors),
after event 71.

## Static read 2 (R3_ST01 full disassembly, 2026-10-31)
The narration is three layers; none uses the message system.

**Text crawl** (object update 0x80100918, scanline 0x80100A70). The text is
prerendered: OPENG0..3 (4-bit 256x256 pages) sit side by side in VRAM, tpage
21 + (v >> 8), CLUT 0x7C00. Each frame emits 176 one-line POLY_FT4 strips,
x 32..287, at y 32 + i:
- i 0..15: v = scroll + i, colour i*8 (grey), semi-transparent (fade in)
- i 0..143 at y 48 + i: v = scroll + 16 + i, colour 0x808080, semi-transparent
- i 0..15 at y 192 + i: v = scroll + 160 + i, colour (16 - i)*8 (fade out)
Strips with v < 0 or v >= 1024 are skipped. Init (0x801008F8): scroll = -190.
Scroll += 1 every 3 updates; the object releases (0x800319F4) at 1024.

**Background layers**: the EXE's 11-layer parallax renderer 0x80023144
(56-byte records at 0x800BBA90), initialised by 0x80022FD4 from the stage
descriptor table at 0x80100D5C (ST01 stage init 0x801000A8). BG\CHAR
ST01BG00..02 are its tiles. Callback 0x801002E4 shifts two layer offsets
(0x800BBB20 -= 56, 0x800BBAE8 -= 16), sets 0x800BBA9C = 130 at tick 0, and
from tick 2200 decrements it every 3 ticks to -11 (CANDIDATE: layer
brightness fade-out). Not ported anywhere in the repo yet.

**Airship**: class init 0x8010080C (0x8002F9C4 model, +344 = 0x00200020,
+348/+350 scale 32/16912, +86 = 1024) and action 0x801006A8: x (+20) -1 every
4 updates, y (+22) +1 every 5 updates until 220, then x continues to -289,
then a 30-update wait and 0x80031824.

Port order: crawl (self-contained) -> background layers -> airship -> boot
hook before the title (coordinate with the main session) -> event 10.

## Background layers ported (2026-10-31)
`source_bg_layers`: 0x80022FD4 init and the 0x80023144 velocity scroll
(+0x8002366C wrap) and tile draw; the camera-driven scroll (record+1 == 0)
holds and is reported. ST01 uses four velocity layers (all +1 = 1): layer 0
is the lighthouse (hidden at init, shown by the callback at 2200, x 130 ->
-11), layers 1/2 drift once the callback sets vx -16/-56, layer 3 is static.
Tiles: tile word + (+0x28 << 14) gives CLUTs (128..176, 511) over tpages
0x1C..0x1E; all eight intro materials resolve fully from ST01's TIMs.
SPRT tiles are drawn as the identical POLY_FT4. Widescreen: 40 px of
wrapped columns on each side (port presentation). Before the phase-4 fade-in
the screen stays black (oracle shot at phase 1: black + "now loading").
Not oracle-verified yet: layer values per tick (the oracle stalled at boot
on this run; static read only).

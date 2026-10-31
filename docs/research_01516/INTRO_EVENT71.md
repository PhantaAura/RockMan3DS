# Pre-title intro: ST01 event 71 (scope, 2026-10-31)

Status: research only (static read of R3_ST01.BIN; extracted owner-locally
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

## Callback 0x801002E4 (text crawl)
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

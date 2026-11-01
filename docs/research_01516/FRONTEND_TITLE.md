# Front end: intro, title, menu and New Game (research, 2026-10-25)

Status: research. Sources: ROCK_NEO.EXE disassembly, R3_ST01.BIN decompiled
with Ghidra headless (owner-local, not committed), and the psxrecomp oracle
(phase/event bytes and screenshots). Nothing ported yet.

## Flow observed in the oracle
| Screen | main phase / sub (0x800C1B10/11) | stage 0x800C1B60 | event 0x800C4C48 (act, id, phase) |
|---|---|---|---|
| Capcom logo (CAPCOM20.STR) | 0 / 0 | 0 | — |
| now loading | 1 / 0, 2 / 1, 3 / 0 | 1 | — |
| Intro "In a world covered by endless water…" | 3 / 2 | 1 (ST01) | 1, 71, 3 → 5 |
| Start during the intro | 3 / 2 | 1 | 71 → phase 10, then inactive |
| Title (logo, Servbots, PRESS START) | 3 / 2 | 1 | none |
| Menu NEW GAME / CONTINUE / OPTION | 3 / 2 | 0 (next stage) | none |
| No input: attract demo | 3 / 2 | 7 (ST07) | 1, 10, 0 → 2 |
| New Game: opening | 3 / 2 | 0 (ST00) | 1, 1, … 6 (Area 0 scene) → 7 (hand-off 0x80101274) |
| Control | 2 / 0 → 3 / 2 | 0 | — ; Mega Man at Area 1 (2822, -1, -4833) |

## ST01 (intro and title stage)
ST01.BIN uses the ST00 container. Resources: `R3_ST01.BIN` (code, 0x1B5C bytes
at 0x80100000), `ST01_MAP.IDX/MDT` (a small map), `ST01.STG/HED`,
`ST01_00.EBD`, `EM0A00.TIM` (from ST00/AR04), `OPENG0..3.TIM`,
`ST01BG00..02.TIM`.
- Header: word 0x14 then ten entry pointers (0x801004D8…0x801005D0).
- 0x80100098: stage area init (camera block 0x80100F1C/F30, actor tables
  0x80100D5C/D88/DA4, area record copy 0x800371A0).
- 0x8010049C: intro event (71) phases 0..10. Phase 4 builds the scripted
  scene (0x8001E6C0, 0x8001EAE8); 5 waits for the script or Start
  (0x8001DE84); at 10 it ends with `0x80012F78(0x80013420)` and bumps
  0x80098AA8 (front-end state).
- 0x80100920 / 0x80100A70: the sky/sea gradient, one POLY_FT4 line per call
  (16 + 144 + 16 lines, CLUT 0x7C00 row).
- 0x801006A8: the airship actor's path; 0x8010080C its class init.

## Front-end state machine (ROCK_NEO.EXE)
Thread 0x80013420 (started by 0x80012F78): resets 0x1F800001 = 2, stage
bytes, then loops: `state = *(s8*)0x80098AA8`; Start or Cross (0x800C0C2A &
0x4008) while state 2 plays sound 130, sets byte +1 from the save checks
(0x80098A72/70) and state 4; dispatch table 0x80080950; then 0x80031AA4 and
0x80012E98(1) (frame wait).

| State | Function | Role (CANDIDATE where marked) |
|---|---|---|
| 0 | 0x80013578 | attract: toggles +3, stage = 1 (intro) or 7 (demo), thread 0x800155A4 |
| 1 | 0x800135DC | title setup: 0x8002E7A8(1), 0x8003697C, 0x8002CFA4, area record 0x800808D0/A8, scratch 0x1F800058 = 0x800808A4; then 0x80015428, fade 0x80017C30(2), state 2 |
| 2 | 0x800136B8 | title, substates via 0x80010070: fade, 0x80053A30, timer +4 = 900, logo 0x80013FB8, PRESS START 0x80014394 (blinks by timer), then attract |
| 3 | 0x80013890 | state = 0 |
| 4 | 0x8001389C | menu (CANDIDATE: NEW GAME / CONTINUE / OPTION), draws 0x8001456C |
| 5 | 0x80013CA0 | CANDIDATE: CONTINUE (memory card, 0x80062368) |
| 6 | 0x80013E0C | CANDIDATE: OPTION; Start (0x800C0C2A & 8) → sound 0x83, state 4 |

Scene draw for the title: 0x800137D4 (0x800175D8, 0x80015428, E0 sweep
0x80016FD4, camera 0x80038370(0x800979DC, 0x800979EC), 0x800175FC, class-20
draw 0x80028DE0, OT link at 0x800A4D40/0x800A8D3C).

2D packets (POLY_FT4, code 0x2C, 9 words, rgb 0x808080):
- logo 0x80013FB8: TPAGE GetTPage(1,0,320,256), CLUT GetClut(0,0x1F0),
  xy (32,8)-(287,168), uv (0,0)-(255,160);
- PRESS START 0x80014394: TPAGE GetTPage(0,0,320,256), CLUT (0,0x1F1),
  xy (56,176)-(264,192), uv (0,160)-(208,176);
- menu strip 0x8001456C(sel): TPAGE (0,0,320,256), CLUT (sel ? 16 : 32, 0x1F1),
  xy (104,148)-(216,164), uv (0,200)-(112,216).

## Title assets (DEMO_DAT.BIN, not ST01)
The title graphics are in `DEMO_DAT.BIN` (the front-end package): `NTITLEU.TIM`
256x256 8 bpp at VRAM (320,256), CLUT row 496 (256 entries, the logo);
`NTITLE5U.TIM` 256x96 4 bpp at (320,416) with 9 palettes on row 497 (PRESS
START, copyright, menu text incl. EASY/NORMAL/HARD); `NTITLE3U.TIM` 192x160
8 bpp at (576,256), row 498 (not yet placed); `DEMO_DAT.EBD`; `ee_title.vab`.
`l3cook_frontend_materials DEMO_DAT.BIN OUT.l3mc` resolves the four words
fully (logo 0x7C000095, PRESS START 0x7C400015, menu 0x7C410015/0x7C420015).
PRESS START (0x80014394) draws two packets: the text, then the copyright
lines at (32,208)-(288,232), uv (0,176)-(255,200). The fourth menu row
(0x800C1B28 set) is the difficulty (EASY/NORMAL/HARD).

## Title backdrop (ported) and the Servbot parade (logic ported)
GP0 capture of an original menu frame (psxrecomp `gpu_frame_capture.py`)
and a RAM write trace on the packet buffer show 0x80013FB8 writes:
logo (OT list 0x800A4F50, linked at entry 32); the NTITLE3U wall as two
POLY_FT4 halves x 0-160 / 160-320 (uv mirrored), TPAGE GetTPage(1,2,576,256),
CLUT row 0x1F2 (entry 35); a POLY_G4 | 2 strip (0,144)-(320,160) black to
white with an E1 SetDrawMode(GetTPage(1,2,0,0)) after it (ABR 2, B-F)
(entry 34); a POLY_G4 gradient (0,162)-(320,240) black to (192,255,224)
(entry 35); then 0x800137D4 (scene draw). The menu adds a POLY_F4 | 2 box
(96,144)-(224,200) (216 with the difficulty row), black, after its rows,
blending with the logo's ABR 0. Ported; packets in `frontend_title.cpp`.

Servbots (logic VERIFIED_1TO1 against the oracle, drawing not yet ported):
0x80015428 (called with the front-end state) spawns 16 class-20 actors
through 0x80031418 (pool 0x8009F108, 0x3C0 bytes): work +0 = 1, +3 = index,
+2/+4/+0x0C cleared, then byte +0x0C = wave (+8). It waits 300 updates (+6);
then **0x8002EE90(2, 0x80)** sweeps the class-20 pool (bit 2), unlinking
(0x80052528) and releasing (0x80031824) every live slot (not a music call),
and steps the wave (wraps after 8). Phase 2 re-arms only in states 4/5/6
(menu), so the title itself shows one wave. Class callbacks: scratch
0x1F800058 = 0x800808A4 -> table 0x800808A0, entry 0 = 0x800153EC,
dispatching on work +8 through 0x80082104 (init 0x80014AC0, run 0x800150E0,
release 0x800153CC) with a1 = work + 0x348.
- Script reader 0x80014BCC: op 0 = wait (count - 1), ops 1..27 through table
  0x800100B4[op - 1] (26 jump 0x80015074, 27 end 0x80015068 = state 2).
  0x800150E0 has no state test after the reader: the end update still moves.
- The title setup's spawn (state 1) runs its callbacks from the next update;
  menu spawns (via 0x800137D4) in the same update (oracle, mechanism CANDIDATE).
  A reused slot keeps its previous position (+0x14/+0x1C are not cleared).
- Camera 0x80038370(a0 = 0x800979DC target (0,-224,512), a1 = 0x800979EC
  (0,-448,-512)): distance = SquareRoot0 of the XZ distance plus one Newton
  step (1024), yaw = ratan2(dx, dz) + 0x800, pitch argument -ratan2(dy, d)
  = 141, then 0x80037FB4. Oracle 0x800A3A10 = (0xF73, 0): VERIFIED.
  `tools/cook_title_servbots.py` evaluates it on the EXE tables (L3TS0002).
- Model: DEMO_DAT.EBD (load 0x8016C000) class 0x20/0, 7 parts, one word
  0x3D88002B (page (704,0), CLUT (128,246)); requests 0/1/2/9. The page
  resolves fully from DEMO_DAT.BIN (EM0000.TIM; CANDIDATE that the live VRAM
  holds it rather than ST01's EM0A00).
- Oracle comparison (`tools/l3probe_title_servbots`, pool snapshots
  slots 0-3 per frame, game update = 2 frames): title wave 0, 664 updates /
  958 slot-updates, 0 mismatches; menu waves 4-7, 1094 updates / 2805
  slot-updates, 2 mismatches, both a slot's position left from before the
  probe window.
## Port plan
1. Cook ST01 (map, EBD, TIMs) into the pack.
2. Port the front-end state machine (states 0..6, substates) as a host-tested
   module; New Game hands over to the existing ST00 boot.
3. Draw the title: ST01 scene (map + Servbot actors) and the three 2D packets
   through the GP0 translator.
4. Then the intro (event 71) and the opening (ST00 event 1, partly ported
   behind SOURCE_EVENT_BOOT).

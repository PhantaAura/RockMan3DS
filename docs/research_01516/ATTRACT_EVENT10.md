# Title attract scene: ST07 event 10 (scope, 2026-11-01)

Status: research only. Static read of `r3_st07.bin` (from `ST07.BIN`, load
0x80100000, 35904 B; extracted owner-locally, not committed) plus oracle runs
on the US disc. Nothing ported yet. Played after the title idles (title
result Attract, 0x80013420); the port currently skips it (`run_frontend_title`
returns Attract, main.cpp replays the narration).

## Dispatch
The EXE event table 0x80089024[id] gives the handlers: event 1 0x801010C4
(ST00), event 10 0x80100330 (ST07), event 71 0x8010049C (ST01). Event 10's
handler dispatches on event+4 through 0x80105B80: phase 0 0x8010036C,
phase 1 0x80100598, phase 2 0x801006D8.

Oracle: event 10 runs in stage 7 with the `ST07_00` container loaded (the
EBD at 0x8016C000 matches `ST07_00.EBD`, 54960 B). The **map area changes
with the shot segment**: 0x800C1B61 = event+8 (0 for the exterior shots,
then 1, 2, 3, 4). Map area 0 has no terrain (exterior: background layers
and the ship model); areas 1-4 cook fully with `l3cook_material_cache`
from INIT_DAT + ST07 + ST07_00 (7/5/6/7 materials, all resolved); areas
5/6 belong to other containers.

## Phase 0 (0x8010036C, sub-state event+5)
- sub 0: `0x8001E6C0(0,0,0)`; camera directory 0x800BC7F0 = 0x80105DD0;
  timeline event+0xA4 = 0x80105E1C; +0xA = 128, +0xB = 0, +0xC = 0;
  `0x8001EAE8(0x80105B8C)` (one spawn record); eight background-layer
  records from 0x80106124 (8-byte rows: +0 flags byte, +0x20 vx, +0x22 vy);
  0x800BBC26 = 240, 0x800BBC20 = 0x00F00000; 0x80108B0C = 0;
  CD command `0x8001D58C(0x20000005)`.
- sub 1: record callback; wait `0x8001D878` (CD done).
- sub 2: record callback; +0xB counts ticks: 10 -> fade `0x80017C30(2)`;
  30 -> `0x8001F1DC`, camera directory 0x80105E2C, timeline 0x801060D4,
  phase 1.
- Every tick: Start (0x800C0C2A & 8) sets +0xA |= 1 (skip request);
  camera `0x8001EC0C`; timeline `0x8001F070`.

## Main timeline 0x801060D4 (records {u16 event+8, u16 ticks, callback})
| Shot (e9:e8) | Ticks | Callback | Oracle frame (60 Hz) | Content |
|---|---|---|---|---|
| 0:0 | 240 | 0x801007E0 | 9803 | Gesellschaft exterior over the sea |
| 1:0 | 210 | 0x80100834 | 10281 | exterior from above, clouds |
| 2:0 | 150 | 0x8010086C | 10702 | |
| 0:1 | 90 | 0x80100944 | 11002 | |
| 0:2 | 150 | 0x801009B8 | 11181 | corridor, two Servbots walking |
| 0:3 | 180 | 0x80100B14 | 11476 | |
| 0:4 | 150 | 0x80100B98 | 11839 | |
| 1:4 | 120 | 0x80100C4C | 12141 | bridge: Tron, Teisel, Servbots |
| 2:4 | 90 | 0x80100CCC | 12381 | |
Total 1380 ticks (46 s); then e8 = 0xFF, phase 2 at frame 12566.
Camera program 0x80105E2C: one block per shot, selected by op 1 (e8 byte +
counter), each with 0x10 target, 0x11 yaw/pitch/distance, 0x12/0x13 pan
velocities and 0x20 shift. All opcodes are already ported (event 1).

## XA
`0x20000005` -> stream 245 (same file as the opening/narration), span LBA
91731..98747, **channel 5**: 7016 sectors, 46.77 s (matches the timeline).
`cook_xa_stream.py` needs this span added to `MEASURED_SPANS`.

## Stage resources (ST07_00.BIN + ST07.BIN)
`r3_st07.bin`, `ST07_MAP.IDX/MDT`, `ST07.STG/HED` (core); `ST07_00.EBD`;
object TIMs EM0000/EM0B00/EM2800/EM2B00, BS0B00/01 (Bonne characters);
SCR st07 d00xxx TIMs (terrain pages); BG CHAR ST02BG00/01 + ST07BG02
(background layers); DUMMY.MSG.

## What a port needs (vs. what exists)
1. Stage cook for ST07 area 00: map/render/init (the existing cookers are
   ST00-specific), terrain pages, background layers (the ported
   `source_bg_layers`, eight rows set by phase 0).
2. Event 10 handler, both timelines, the nine callbacks (actor spawns,
   actions) and the camera program through the existing interpreter.
3. Actors from ST07_00.EBD: the Gesellschaft, Servbots, Tron, Teisel
   (classes to be traced per callback).
4. XA channel 5 cook + playback (ticks locked to it, as in the narration).
5. Hook: `run_frontend_title` returns Attract -> play event 10 -> event 71
   -> title (oracle loop).

Shots captured from the oracle (owner-local, not committed) give a
per-shot visual reference for each step.

## Logic ported (2026-11-01)
`source_attract_event10`: the handler's three phases, the timeline consumer
0x8001F070 and the resumable camera interpreter 0x8001EC0C (pc at block
+0xB8 persists; each pass runs to the first failed wait, then velocities are
added), with the callbacks' background flags (0x801007E0 hides clouds at
count 222; 0x80100834 swaps to the top-down sea and strips; 0x8010086C hides
the strips) and area requests (0x80100944/9B8/B14/B98 -> 1/2/3/4,
0x80100CD4). Actor spawns/releases are reported, not ported. Background:
every area uses table 0x80108B4C (eight velocity layers, hidden), phase 0
writes the rows at 0x80106124 and layer 7 y = 240. Fade 20 (natural end)
is not a ported fade type yet.
Cooked check: phase 2 at tick 1414 (oracle ~1417); shots 1:0/2:0/0:1 at
273/483/633 (oracle ~274/485/635; the constant 1-2 tick lead is the CD
wait, taken as one tick). The 170-word camera program runs without an
unsupported command.

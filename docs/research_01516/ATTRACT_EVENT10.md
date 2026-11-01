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

## Player (2026-11-01)
`run_attract_event10` (main.cpp): plays when the title times out, alternating
with the narration as in the oracle (event 10 first). ST00's terrain is
released for the scene and rebuilt after. Each shot's area request
rebuilds the ST07 terrain of that area (materials and CLUT ramps from
attract/) and re-initialises the background layers (stage init). Frames:
background entries 35..33, the terrain through the camera program's view
(the face camera, cell visibility and 0x1F800006 far value of shift 0, plus
the scene-view matrices for directly drawn batches), then entries 32..30
(the semi-transparent cloud strips sit in front). XA 0x20000005 (channel 5,
46.77 s) starts the tick after its command and drives the tick clock. Black
until the phase-0 fade-in. Start skips (fade 10). Not ported: actors, fade
20 (natural end cuts), the areas' fog/depth cue (no area record).

## Cast (2026-11-01)
Oracle function trace (PSX_FNTRACE_ALL) per shot: shot 0:0 runs only the
handler and 0x80102AEC/0x80102C04 (a0 = 0x8009F108, slot 0 of the pool
hook 0x1F800058 dispatches: table[+2] -> per-class table[+3/+8]); the
corridor adds 0x8010205C/0x801022A4 (many objects); the bridge adds
0x80102D84, 0x80102E80.., 0x8010384C, 0x80103A00.
ST07_00.EBD rows: 0x20/0x00 (Servbot, page 0x3D88002B as on the title),
0x20/0x0B (the Gesellschaft, page 0x3D000026), 0x20/0x28, 0x20/0x2B,
0x40/0x0B. All class-0x20 pages resolve from INIT_DAT + ST07 + ST07_00.
**The Gesellschaft** (ported): record 0x80105B8C = family 0x20, type 0x0B,
variant 0 (0x801066D0[0] = 0x80102B28) at (0, 0, 0): init sets the drawn
flag, scale +0x158 = (512, 512, 512), animation request 0; each tick saves
the previous position and advances the animation (0x80032488). Lifetime:
the exterior area (the area-1 callback releases the actors).

## Servbots (2026-11-01)
Class dispatch: hook 0x1F800058 = 0x801054D4[class] -> per-class table
[+4] (Servbot 0x801052DC[4] = 0x8010205C; ship 0x80105338[10] =
0x80102AEC). Records 0x80105B8C.. (29, contiguous; spawn 0x8001E968 copies
rec[12..17] to +0x14/+0x1C and rec[18..19] to the heading +0x56; 0x8001EAE8
copies rec[8..11] to +0xC..+0xF). Callback spawn lists (first tick, after
releasing the previous event+0x24 slots): 0x80100944 records 1-3,
0x801009B8 4-6, 0x80100B14 7-11, 0x80100B98 12-14, 0x80100C4C 15-28 (11
Servbots + classes 0x28, 0x2B, 0x40/0x0B: the bridge cast, not ported yet).
Servbot init 0x8010212C: scale 1024 if +0xE == 1 else 512; floor query
0x80048E6C (y += floor). Behaviours 0x8010663C[id]: 0x80102238 animate +
shadow; 0x801021DC animate; 0x801022B8 (id 1), 0x8010241C (id 4) scripted
steps on event+0x10; 0x8010239C (id 2) forward 128; 0x80102500 (id 5)
forward 256 from tick 76; 0x80102590 (id 10) stand/walk/request 19 at 125;
0x80102678 (id 24) forward 64. Requests used: 0-7, 9, 10, 0x10-0x14 (all in
the EBD's bank). Not ported: shadows 0x8002C8CC; the floor query runs
against the requested area once it is loaded (CANDIDATE timing).

## Bridge cast (2026-11-01)
Per-class tables [+4 = 10]: class 0x28 -> 0x80102D84, class 0x2B ->
0x80102EA8 (family 0x20, hook 0x1F800058); class 0x40/0x0B -> hook
0x1F80005C = 0x80105584[0x0B] = 0x80105478[10] = 0x8010384C.
**0x28 / 0x2B** (ported): init drawn flag, model, scale 512, floor query,
animation request 19 / 0; then animate each update (shadow radius 48 not
ported). Placed by record 26 (904, -1, 340) and 27 (1124, -1, 390).
**0x40/0x0B** (not ported): a class-0x40 actor with its own timer states
(4/2/10/90/50) and the face UV routines 0x8002D404 / 0x8002D320 (as the
opening's Mega Man face), record 28 at (1024, -1, 240).

## Bridge cast height (2026-11-01)
Seen in Azahar: Bon (class 0x2B) drawn huge and low in the close-up 2:4.
Oracle in that shot: camera target (1024, -414, -95), pitch 209, distance
-273, shift 2 (the port's camera program gives the same); Tron (class 0x40)
at (1024, -106, 240) against record y -1, so the floor query adds -105 on
the bridge. Scale is not the cause (0x80033638 ScaleMatrix(+0x158 <<
shift) = the port's column scale with the shifted projection).
The port's 0x80048E6C query now runs for these classes with null bounds
(sample_actor_floor_80048e6c: work+0x78 = 0) and returns -1 in area 3, but
the cooked ST07 map has no contact list for the bridge, corridor and
area-1 cells (runtime_map_cell_contact_list == null), so those stay at the
record height. Next: decode ST07's contact lists in the map cooker.

## ST07 collision fixed (2026-11-01)
ST07's descriptors keep their contact list at +4 only (+8 = 0), and the
IDX file has raw6 = 0, so 0x8005172C picked the empty +8 list. Oracle: at
runtime every non-null ST07 IDX row has raw6 = 1, the stage init's seed
from the descriptor's render header (map_initialization, as ST00's, whose
seeds are 0). The pack now cooks attract/st07.l3init and the player
applies it. Floor query with null bounds then gives -105 on the bridge
(Tron y -1 -> -106, the oracle's value), -1 in the corridor and area 3,
-257 in area 1.

## Cast identities (oracle frames, 2026-11-01)
Late in shot 2:4 the original zooms onto the class-0x40 character in the
centre, talking (face routines): Teisel. Left edge: class 0x28 (Tron,
brown hat); right: class 0x2B (Bon). Azahar with the seeds applied shows
Tron and Bon at the oracle's framing; the centre is empty until class
0x40/0x0B (Teisel) is ported.

## Teisel ported (2026-11-01)
Class 0x40/0x0B (0x8010384C), record 28: init drawn flag, model, scale 512,
floor query, request 0, face set 0x80103A24(expression 0); each update: in
shot e8 4 the expression changes (e9 1: tick 10 -> 2, 90 -> 0; e9 2: tick
1 -> 2, 50 -> 1), 0x80032488, face step 0x80103A6C. Face: eyes script
0x80106944[expr] (all 0x80106914: frame 2 held), mouth 0x80106950[expr]
(0x80106918 closed, 0x8010691C frame 2, 0x80106920 the 32-frame talking
loop 2/4/3, 127 = loop); index advances unless the next byte is 126 (hold)
or 127 (back to 0); a new frame copies parts 17 -> 16 / 19 -> 18 with
0x8002D404(frame, 42, 64, 6). Drawn through the class-0x20 packet path
(ImportedActorPoseMatrices widened to the 24 class-0x40 slots; class 0x20
keeps 13). Colour/ramp follow 0x80028F90: the actor point is projected raw
(not shifted by 0x1F800074) with translation 0x1F800030; oracle +0x15E =
0x4210 (work 0x800C3580). Shifting it made him dark/red in the close-ups.

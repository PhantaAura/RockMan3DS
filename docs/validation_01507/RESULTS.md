# 0.0.15.7 — event 0 door sequence: Mega Man walks through the doors

## What changed
- **Event 0** (`0x800485E4`) runs in `RuntimeSession::tick` from the door
  request: phase 0 (`0x80042238` hand-over), phase 1's 14 substates (jump table
  0x80010D6C) and phase 2 (`0x80042314` release). The counter +0x10 counts
  phase-1 frames (`0x8001F070`).
- **Player:** state 3 `0x80041B8C` pins him to the event targets, the
  `0x8003C07C` input selection feeds the forced pad 0x800C0F96 from the walk
  `0x80048A9C`, state 0 `0x8003C7B0` re-initialises him after phase 2, and the
  state-1 update now clears +0xBD bit 8 (`0x8003C90C..C92C`).
- **ST00 door object** (state table 0x8011C928): opens on 0x7B1 in its cell,
  holds 0x7B2 for 16 frames, swaps the cell's IDX descriptor to the open
  variant (`0x8003AC24`, word0 & ~0x1000) and back after closing. Native: a
  copy-on-write map; the registry keeps the shared original.
- **Load:** the transition is offered only while substate 6 waits (356C = 1).
  The commit runs the loader spawn, substates 6–8 and the arrival; substates
  9–13 open the Area 2 door, walk him through and close it.
- **3DS:** unchanged commit path; it now sees the offer after the Area 1 walk.

## Results
| Check | Result |
|---|---|
| `l3door_replay ST00.mmlpkg PACK` fall phase | PASS (unchanged): 994 writes, 0 floorless/airborne frames, 24 slab contacts, stops at (−325,−1,−5409) |
| `l3door_replay` door phase | PASS: Circle on replay frame 30 requests Area 2. Area 1 door object opens (0x7B2 held 16 frames), the walk takes him from z −5440 to −4864, the door closes and the load is offered 56 frames later. Arrival (−256,−2,6208), heading 2048, counter 59. Area 2 door opens, the second walk ends at (−256,−1,7104), beyond the door cell (6144..6655), the door closes. Then 0x7B0/0x7B1/0x7B2/0x7B3 clear, 0x380 set, floor in all 10 idle frames |
| Return trip (scratch probe, not committed) | Facing −z in Area 2, Up stops at z 6433 against the re-closed door. Circle runs event 0 back: arrival (−256,−2,−5184), walk ends at (−256,−1,−6191), floor present |
| `source_door_event` (extended) | PASS: walk-limit table; `0x80042238` branches incl. action 14 hold and +10 = 6; state 3 pin/animation/return; input select; both walks and their limits; release and state 0; door object states 0–5, the 7B0-clear close, no-pair cells; session sequence (offer only at substate 6, counter 40/44/112, flags) and the descriptor swap 0 → 1 → 0 with a 16-frame hold |
| Host suite | 148/148 PASS (scratch build) |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; host suite 148/148 in the pipeline; 941,628 B, SHA-256 `a4c5942f733661a99ff3c63ab51632ac465f62021a262072084003fee4c61fd5` |
| Interactive Azahar | NOT RUN this pass; card: `AZAHAR_TEST.md` |
| Physical Old 3DS | Not run |

## Evidence labels
- **VERIFIED** (instructions, `docs/research_01507/EVENT0_DOOR_SEQUENCE.md`):
  event-0 phases and substates, counter writer, forced walk, the 0x8008AA04
  limit (stage 0: 108), state 3/state 0/release stores, input selection, door
  object states 0–5 and its descriptor rule; owner-local data check of cells
  123 and 149.
- **CANDIDATE:**
  - door objects created lazily in the player's door cell (spawn list not cooked);
  - 0x8016C000 being the live IDX table the collision reads;
  - frame order player → event → door objects, and the event starting in the
    requesting frame;
  - one counted frame per load substate (6–8); a longer PS1 load shortens the
    second walk;
  - the A → Circle mapping.
- **UNKNOWN / unported:** camera `0x8001E6C0`/`0x8001EC0C`, door leaf model and
  rotation, door/event sounds (recorded only), BGM check, fades,
  `0x8002D004`, `0x80051BAC`, `0x800321C8`, the object lock flag 0x7E0 + id.
  The 3DS terrain still draws the closed slab while the door is open.
  `PHYSICS_1TO1=false`.

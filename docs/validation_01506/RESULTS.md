# 0.0.15.6 — first Area 1 door opens into Area 2

## What changed
- **Query:** `runtime_player_event_primitive_800514fc` ports `0x800514FC`. It
  covers shape families 0/1 and the default `0x8004A060` interval; other shapes
  hold as Unsupported.
- **Door event:** `evaluate_source_door_event_80050c2c` ports gate `0x80051970`,
  case 3 and the `0x800519E4` lock check. It returns the `0x800C3558..71` lanes,
  sound 0x56 and flag 0x7B0 (or 0x7B1 for a local door).
- **Session:** after player contact, while flag 0x200 is clear. The commit runs
  `apply_source_handoff_spawn_80015ee8` (argument6 = 1), then
  `apply_source_door_arrival_80048898`. Player +8..+B become 1,0,0,0, flag 0x7B0
  is cleared and 0x380 is set.
- **3DS:** A sends psx Circle 0x2000; a door request is committed through the
  same staging path as the ST00 handoff (`door_transition_before/after` in the
  hardware trace).

## Results
| Check | Result |
|---|---|
| `l3door_replay ST00.mmlpkg PACK` fall phase | PASS: 994 writes, 0 floorless/airborne frames, 24 slab contacts, stops at (−325,−1,−5409) |
| `l3door_replay` door phase | PASS: Up from (−334,−1,−5750) heading 2048 stops at z −5409. Circle on replay frame 30 gives TRANSITION_REQUESTED to stage 0, area 2, destination (−256,−1,6272). Commit lands in Area 2 at (−256,−2,6208), heading 2048, matching the source arithmetic. Flags: 0x7B0 set then cleared, 0x380 set. He settles at y −1 with a floor in all 10 idle frames |
| `l3door_replay … --no-init` | FAIL, as in 0.0.15.5: floor lost at frame 9. The door phase never reaches the slab (NO_EVENT) |
| Return trip (scratch probe) | Holding Up in Area 2 stops at z 6335 against the closed door. Circle there requests Area 1 record 0 and lands at (−256,−2,−5184), heading 0 |
| New test `source_door_event` | PASS: query boundaries and double Y subtraction, front point, Unsupported/NeedsTrig holds, gate fields, facing bit 0x400 on both sides, signed quadrant, locks and message, local door, request lanes (356C unwritten), arrival arithmetic, session commit and the flag 0x200 suppression |
| Host suite | 148/148 PASS (scratch build and the m1 pipeline) |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 933,492 B, SHA-256 `f92fc434871ec8d6450aa23b5a7d11b0c6807637f1c58b32e3c90ec079af70f4` |
| Interactive Azahar | NOT RUN this pass; card: `AZAHAR_TEST.md` |
| Physical Old 3DS | Not run |

## Evidence labels
- **VERIFIED:** gate, case-3 matching, lock check, request lanes and the
  substate-8 arithmetic, from instructions
  (`docs/research_01506/DOOR_EVENT_80050C2C.md`).
- **SOURCE_BACKED_PARTIAL:**
  - query subset;
  - call order after native contact;
  - net event-0 flags.
- **CANDIDATE:**
  - the immediate commit replacing event 0;
  - the A → Circle mapping.
- **UNKNOWN / unported:** event-0 substates 0–7 and 9–13, door objects, fades,
  camera, door sound and BGM check. `PHYSICS_1TO1=false`.

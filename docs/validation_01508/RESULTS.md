# 0.0.15.8 — door terrain patch

## What changed
- `RuntimeSession::idx_descriptor_generation()` moves on every effective IDX
  descriptor write (door object `0x8003AC24` swap / state-5 restore) and when a
  transition drops a live swap.
- 3DS: on a generation change, `l3::TerrainDescriptorPatch` plans against
  `session.active_map()`: baked vertices of a swapped identity collapse in
  place (saved bytes come back on restore) and the live descriptor is drawn
  from a small overlay (borrowed textures). No area rebuild. Only descriptors
  with word0 bit 0x1000 are tracked (CANDIDATE: the door object is the only
  ported IDX writer). Diagnostic line `IDX swap patch …`.
- The full-map copy-on-write is kept (collision reads `RuntimeMap` directly).

## Results
| Check | Result |
|---|---|
| `terrain_descriptor_patch` (new) | PASS: span merging, alias bake, hide → byte-exact restore, overlay list, swap from "no descriptor", short live map |
| `source_door_event` (extended) | PASS: generation moves on open, on close, and over the Area 2 door |
| `l3door_replay` | PASS (fall and door phases unchanged); terrain patch beside the door phase: 4 generation changes, 2 hides, 2 restores, 1 overlay identity at a time, 85 tracked identities per area, door IDX 149 closed 33 → open 30, Mid primitives 10 → 10 |
| Owner-local descriptor check | Open and closed door descriptors share render streams; identical primitives at every LOD and bank (`docs/research_01508/DOOR_TERRAIN_DESCRIPTORS.md`) |
| Host suite | 149/149 PASS (scratch build and pipeline) |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 957,008 B, SHA-256 `b10bcfddd480f45c880f65e8d8c0be8bafc000d6f3c9f5d2f801f18eba8ae1fd` |
| Interactive Azahar | NOT RUN; card: `AZAHAR_TEST.md` |
| Physical Old 3DS | Not run |

## Evidence labels
- VERIFIED (owner-local data): door descriptor pairs, shared streams, identical expansion.
- CANDIDATE: only word0-bit-0x1000 descriptors swap; GPU writes after frame acquisition are safe (previous frame complete).
- UNKNOWN: door leaf draw and rotation `0x801126DC`; how the original shows the doorway open. The doors still look closed. `PHYSICS_1TO1=false`.

# Flight Sprint 0.0.12

This checkpoint follows the project's batch -> QA -> bug-fix -> rerun -> package workflow.  Its goal is not to manufacture a version number before a trip; it is to make the existing native core useful on real 3DS hardware as soon as a devkitPro build is available, while continuing exact collision ownership instead of replacing it with guessed gameplay.

## Runtime asset readers

The runtime now round-trips its pointer-free player formats rather than relying only on cooker-side structures:

- `L3MD` player model loader;
- `L3PC` player composition/attachment loader;
- `L3AN` animation loader;
- `L3SK` skeleton loader.

Each loader validates header/ranges/reserved fields and CRC before exposing data.  Synthetic cook -> load tests cover all four formats.  `l3player_runtime_probe` then validates the same path against an owner-side real tutorial cook: five components, 18 attachments, 316 visual primitives (116 triangles / 200 quads), 64 animation slots, a 31-frame frame-table action selected for the probe, 1,148 posed vertex references, and sane first-pose bounds.

## Collision progress

Two additional exact USA executable behaviors are now portable.

### `0x8004B1E0` contact-candidate gather

`runtime_map_collect_contact_candidates_8004b1e0()` reproduces the proved four-cell gather ordering used by the geometry solver: current cell, X neighbor, Z neighbor, diagonal.  Neighbor direction uses the original low-nine-bit threshold, bounds follow the retail unsigned comparison behavior, positive packed references are de-duplicated by packed value, negative/special cells remain distinct, and zero is opt-in exactly at the bridge boundary.  Tests cover sample ordering, bounds/dedup behavior, negative references and optional zero retention.

### `0x8004BC2C..0x8004BD90` no-contact result tail

`l3_collision_no_contact_result_8004bc2c()` reproduces the solver tail's exact dominant-axis result code and movement-sign masks.  Strict MIPS comparisons are preserved, including tie ordering: X loses a tie to Z and the horizontal winner loses a tie to Y.  Tests cover the zero case, all six dominant directions, sign masks and ties.

This does **not** claim the entire `0x8004B6AC` iterative geometry/contact-response loop.  The large response routine at `0x8004BDC8` and the remaining primitive dispatch/correction path are still open, so no floor/wall/ceiling semantics are invented in 0.0.12.

## ST00 render/runtime probe

`l3flight_render_probe` loads only cooked `st00.l3map` and `st00.l3render`, expands every tutorial area from runtime descriptor indices, selects fixed Mid LOD, triangulates the records and verifies:

| area | positive cells | Mid-LOD primitives |
|---:|---:|---:|
| 0 | 40 | 1,752 |
| 1 | 130 | 854 |
| 2 | 202 | 674 |
| 3 | 24 | 120 |
| 4 | 2,496 | 12,112 |
| **total** | **2,892** | **15,512** |

The expanded output is 93,072 triangle vertices after quad triangulation.  This matches the previous structural ST00 count and proves the pointer-free map/render bridge reaches a backend-ready stream without original PS1 pointers.

## First real 3DS backend source

`platform/3ds/` now contains a libctru/Citro3D application source and PICA200 vertex shader.  When built with a current devkitPro installation it is designed to:

- load the owner's cooked tutorial data externally from `sdmc:/3ds/Legends3DS/data/legends3ds/tutorial/`;
- validate and load ST00 map/render/flow/reference/message/EBD/player data;
- render real ST00 geometry to the top screen with depth buffering;
- load, pose and render the real cooked Mega Man model/animation;
- drive the portable `RuntimeSession` movement/turn path using D-pad/Circle Pad input;
- expose Buster/lock-on integration probes;
- cycle all five ST00 sub-areas with SELECT;
- show a bottom-screen hardware/debug status panel.

The probe deliberately uses untextured fixed Mid-LOD terrain and a last-valid-area guard.  Those are development safety choices, not final gameplay behavior.  The full collision solver, enemy/EBD execution, final materials/textures, cutscene choreography, XA playback and tutorial progression still separate this probe from keeper 0.1.

The active environment does not contain devkitARM/libctru/Citro3D.  Therefore no `.3dsx` is claimed in this checkpoint.  The backend source was syntax-checked against a local API stub and the exact selected portable source set was host-linked with dead-section elimination to catch missing dependencies; actual devkitPro compilation remains the next hardware acceptance gate.

## SD assembler

`tools/build_flight_sd.py` assembles the development SD layout from an owner-side cooked pack and an optional compiled `.3dsx`.  It copies the cooked `legends3ds/tutorial` directory under `/3ds/Legends3DS/data/` and never copies the original CHD/BIN/CUE/ROM.

## Batch QA

`tools/flight_qa.sh <CDDATA/DAT>` performs a fresh strict Release build, CTest, a fresh 91-file owner-side tutorial cook, player/runtime/render probes, source asset-safety scan and 3DS backend source check.  The validation for this checkpoint is recorded in `validation_012.json`.

## Still required for 0.1

0.0.12 is a hardware-probe checkpoint.  Keeper 0.1 still requires the remaining exact collision/grounding/action-state ownership, exact Buster/lock-on owning routines, per-frame tutorial EBD/enemy behavior, event/camera choreography, AUDIO-to-XA routing/decoding/playback, opening/tutorial/ending state flow, final title/save/bottom-screen integration, a real devkitPro build, and Old 3DS hardware parity testing.

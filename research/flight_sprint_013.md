# Flight Sprint 0.0.13 — Player-Facing Bottom Map Bridge

0.0.13 continues directly from the 0.0.12 Citro3D hardware-probe source. Its purpose is to make the first real 3DS probe feel less like a telemetry harness without pretending unfinished gameplay ownership is complete.

## Imported-map minimap model

A new backend-neutral `BottomMapModel` is built directly from the cooked pointer-free `L3MP` area grid. Positive packed cells are represented as normal map geometry, negative/special cells remain distinct, and zero cells stay empty. Player position is resolved through the same recovered ST00 world-to-grid addressing already used by runtime map lookup. No door, secret, traversal, room-name, or objective semantics are invented.

The model also supports deterministic down-sampling for the 320x240 lower display or host probes. This keeps the map layer reusable when the temporary console presentation is later replaced with a skinned Citro2D/Citro3D bottom-screen renderer.

## 3DS gameplay-shell presentation

The 3DS probe now consumes the map model instead of filling the lower screen with XYZ, yaw, frame, projectile-count, and terrain-vertex telemetry. The lower display presents:

- the current imported ST00 area as a compact minimap;
- Mega Man's current grid position;
- HP;
- Buster readiness;
- lock-on state;
- concise hardware controls.

`SELECT` still cycles the five tutorial sub-areas because this remains a hardware QA checkpoint. The top-screen renderer is unchanged: real cooked ST00 geometry, real cooked Mega Man model/pose, fixed Mid LOD, untextured terrain, and the temporary last-valid-area guard remain explicit probe limitations.

## Validation

The strict host Release build and core test suite pass with warnings treated as errors. New unit coverage verifies positive/negative/zero map-cell classification, player-grid tracking, fixed-grid sampling, and invalid-area behavior.

The validation environment still does not contain devkitARM/libctru/Citro3D, so no `.3dsx` is claimed. A real devkitPro compile and Old 3DS hardware run remain acceptance gates.

## Still required for 0.1

0.0.13 does not change the remaining fidelity blockers: full exact collision/grounding/action-state ownership, exact Buster/lock-on owning routines, per-frame tutorial EBD/enemy execution, event/camera choreography, AUDIO-to-XA routing/decoding/playback, opening/tutorial/ending state flow, final materials/textures, final save/title integration, and hardware parity testing.

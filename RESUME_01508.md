# Resume 0.0.15.8 (M2 — door terrain patch)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.7.

## Completed
- `RuntimeSession::idx_descriptor_generation()`: bumped by every effective
  `write_idx_descriptor` (door object `0x8003AC24` swap and restore) and when
  a transition drops a live swap.
- 3DS renderer (`platform/3ds/source/main.cpp`): when the generation moves it
  re-plans against `session.active_map()`. Baked vertices of a swapped IDX
  identity are collapsed in place (saved for the restore) and the live
  descriptor is drawn from a small overlay; no area rebuild, textures borrowed.
  Logic in `l3::TerrainDescriptorPatch` (`src/render/terrain_descriptor_patch.cpp`),
  tracked only for descriptors with word0 bit 0x1000 (85 per area on the replay).
- Finding (`docs/research_01508/DOOR_TERRAIN_DESCRIPTORS.md`): ST00's Area 1/2
  door descriptors (32→28, 33→30) share the same render streams; open and
  closed draw identical primitives. Only the collision list changes. So the
  patch runs but **the doors still look closed**.
- Tests: new `terrain_descriptor_patch`; `source_door_event` checks the
  generation; `tools/l3door_replay` runs the patch bookkeeping beside the door
  phase (2 hides, 2 restores). Host 149/149; native build PASS.
- Copy-on-write map kept (collision reads `RuntimeMap` directly; an override
  table would touch every collision reader). Not measured on Old 3DS.

## Next
- Owner: run `docs/validation_01508/AZAHAR_TEST.md`; send `boot_01508.log` and
  `hardware_01508_*.csv` (look for `IDX swap patch` lines).
- Find how the original makes the doorway look open: the door object's draw /
  model and the state-2 rotation `0x801126DC` (UNKNOWN), or a terrain record
  field the native decoder ignores.
- Cook the ST00 area object list so door objects come from their real spawns
  (now CANDIDATE lazy spawn) and read the lock flag 0x7E0 + id.
- Camera owners `0x8001E6C0`/`0x8001EC0C`, door sounds, BGM check, fades.
- If Old 3DS memory gets tight: replace the full-map copy with a descriptor
  override read by `runtime_map_cell_idx`.
- `RuntimeSession::reset_player` clears door objects but not `door_map_`
  (pre-existing; harmless today).
- Still open: 32 s / 44 s camera spots; pack section 35 into `ST00.mmlpkg`.

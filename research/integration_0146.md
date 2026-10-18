# 0.0.14.6 Integration Notes

## Lineage

`0.0.14.3 (trusted Old-3DS boot) -> selective 0.0.14.5 source-backed additions -> Push-2 bridge modules -> 0.0.14.6 staged renderer candidate`

Parent archive hashes are recorded in `state/CURRENT_CHECKPOINT.json`.

## Promoted into the actual runtime

These are not merely copied reports; they are compiled into the 0146 source path:

- 0145 PICA200 8x8 Morton + ABGR RGBA8 conversion (`ctr_texture_layout`).
- 0145 per-area Mega Man material-cache cooker.
- 0145 PL00M00 S-record -> M-pose sequencing and exact `0x80041DDC` request-state mutation.
- 0145 ordinary-movement bridge requesting `0x0C` before the existing turn/motion path.
- 0146 dual shader backend with a physically separate 0143 fallback and UV renderer.
- 0146 staged terrain/material/player/alpha/animation feature gates.
- 0146 SD boot-stage logging and owned linear/texture allocation counters.
- Push-2 renderer recovery and runtime diagnostics C modules are linked into the 3DS probe.

## Compiled but not silently activated

The rest of Push-2 (`combat_v2`, `world_v2`, event VM, cutscene bridge, save layers, loader guard, bottom UI, etc.) is built as the separate `mml3ds_push2` host library. This keeps the source available and regression-tested without letting reconstructed/unverified behavior replace Classic runtime semantics prematurely.

## Frozen fallback boundary

The 0143 shader is byte-identical to the trusted 0143 source (`fd242f...73bb`). The renderer fallback uses the original 3-float terrain layout, one VBO loader, fixed color attribute, original immediate-mode player attribute shape, untextured texture environment, and no alpha/material upload path.

The binary as a whole is not claimed byte-identical to 0143; core animation request research was intentionally carried forward. The claim is specifically a **true 0143 renderer fallback**.

## Owner-data validation performed for this checkpoint

Fresh local cook from the owner extraction:

- 101 files.
- Terrain Mid-LOD: 15,512 primitives total; 14,328 statically resolved; 1,184 unresolved.
- Unresolved material cases remain the known dynamic CLUT family; unrelated palettes are not substituted.
- Mega Man: 2 material words, resolved in all five tutorial areas.
- PL00M00 animation: 55 structurally mapped requests, 833 S records, 10 branches, 45 repeats.
- Required ordinary movement request `0x0C`: structural PASS (S action 12 -> M action 13).

Owner-cooked payloads were deleted/excluded before source packaging.

## Validation boundary

Host C/C++ and bridge regression tests pass. The current execution environment has no devkitARM/libctru/Citro3D, therefore:

- no `.3dsx` was produced here;
- no 0146 Old-3DS result is claimed;
- texture orientation/channel/alpha correctness remains hardware pending;
- movement/physics is not labeled 1:1;
- tutorial milestone 0.14 is not complete.

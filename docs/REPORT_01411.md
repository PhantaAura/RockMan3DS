# 0.0.14.11 — cumulative content visibility pass

This continues `work/Legends3DS_0.0.14.10_TerrainPlacement` in `work/Legends3DS_0.0.14.11_Visibility`. There is no Git repository, branch or commit. The frozen parent archive SHA-256 is `3f3edf130aca56edfc12453d3bb83282f9366e57ecb063d95d5f1d95fc45a2cc`. Synced sources and previous checkpoints were preserved. The historical supplied foundation is 0.0.14.6c and its 0143 safe renderer.

## Implemented shared fixes

- Cook and retain the default negative STG descriptor and referenced special HD2 descriptors, even when HED/IDX never references them. Optional HD2 participates in cache identity. Existing packages remain readable.
- Bind both negative descriptor families to the shared native area enumeration. Preserve signed halfword origins, packed flat height and rotation; retain the positive authored IDX path unchanged.
- Carry evidence-backed negative material/UV overrides in bounded optional package section 32 (`L3NM`). Unknown bindings get an untextured batch; they cannot borrow a material cache entry or unrelated palette.
- Compose material pages from ordered VRAM writes, including partial image uploads and later overwrites. Palette upload staging is treated separately from stored palette dimensions.
- Admit empty structural areas without rejecting a populated package; select the next populated diagnostic area through the shared bounded helper. This is not retail progression.
- Add a bounded diagnostic entry/heading search using the existing frontend camera. It repairs the empty inspection view; it is explicitly CANDIDATE, not a recovered retail spawn or camera rule.
- Keep allocation admission and cleanup shared. Release inspection storage before VBO creation, release optional bindings after session shutdown, track batch ownership before fallible uploads, and unwind partial GPU initialization. Invalid optional material bindings do not prevent safe boot.
- Update cooking, package validation, boot-sidecar identity checks, cache signatures and inspection probes. Code-only changes continue to reuse verified cooked data.

## Measured package result

The same runtime loader and expansion helpers checked all 37 packages containing both map and render sections: 203 areas, with 31 geometry identities in the full content graph. All 55,817 default and 2,183 special negative cells now bind, producing 58,000 additional Mid primitives. Positive results remain identical: 24,075 unique instances, 11,256 grid aliases and 137,451 Mid primitives. All 37 unload checks pass.

Only 109 default negative primitives in the five baseline areas have proven material bindings. The other 55,708 default primitives and all 2,183 special primitives retain UNKNOWN texture state. These aggregate structural checks include the supplied full data profile and do not establish the playable early-game boundary. No later story content is exposed here.

## Builds and checks

Inherited baseline: 15/15 CTest suites passed before editing. First combined build compiled but passed 19/20 suites; the quarter-turn fixture expectation was corrected against the recovered transform. The complete installer then passed 20/20 suites and validated all 205 packages before detecting a genuine palette-upload decoding problem. Both failed attempts and diagnostics are retained in `work/pass-20261002-visibility`.

Native toolchain: devkitARM r68-1, GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2; `-march=armv6k -mtune=mpcore`. A native integration smoke build compiled. Final installer, binary identity, emulator exercise and captures are recorded in `validation_01411/final-results.json` with six original captures.

Exact reproducible commands, run from this source tree:

```sh
cmake -S . -B /absolute/host-build -DCMAKE_BUILD_TYPE=Release
cmake --build /absolute/host-build --parallel 4
ctest --test-dir /absolute/host-build --output-on-failure
python3 -B -m unittest discover -s tests -p 'test_*.py'
./build_mml3ds.sh /absolute/owner-data/extracted --out /absolute/fresh-install --cache /absolute/separate-cache
/absolute/host-build/l3area_visibility_audit /absolute/package-a.mmlpkg /absolute/package-b.mmlpkg
```

The audit accepts every structurally supported map/render package; no room-specific fix is required. `validation_01411` preserves aggregate research and current build results. Full local logs remain in the pass directory; source releases exclude raw/cooked owner payloads and binaries.

## Memory and performance limits

Across the actual host package audit, maximum decoded estimates were 389,996 bytes and load-peak estimates 743,084 bytes, admitted against 4 MiB decoded / 8 MiB load-peak caps. These host `sizeof` estimates are not measured Old 3DS RAM. Native terrain admission is 34,952 primitive records, conservatively six 20-byte vertices per primitive, within the 4 MiB terrain VBO policy. Inspection geometry is bounded and load-only. Existing player rendering still has per-frame temporary work; whole-engine zero-allocation behavior is not claimed.

Azahar 2126.0 is tested with New 3DS disabled, CPU 100% and native resolution. Its homebrew Dev1 region can be 96 MiB, so passing there does not establish the retail Old 3DS memory budget, timing or GPU behavior. Sparse timing samples include synchronization and logging; they are not worst-case frame-pacing acceptance. Physical Old 3DS / XL remains HARDWARE_PENDING.

## Remaining visible-content blockers

Imported object records still have no proven complete initializer-to-model/material/animation presentation chain. The cooked EBD directory currently retains rows and offsets but omits the pointed actor resource payloads. The native draw path submits terrain, the player and projectiles. This is a concrete missing cooker/binding/runtime path, not proof that owner files are absent. See `OBJECT_VISIBILITY_BLOCKERS_01411.md` and its machine-readable evidence; no invented placeholder enemies were added.

The contact solver is not connected to normal native movement. Rendered floors therefore do not prove floor snapping, slopes, landing, walls, grounding or gravity. `PHYSICS_1TO1=false`. Original gameplay camera/entry, natural progression, event semantics, cutscene/audio playback, enemy combat and retail save semantics remain incomplete. SELECT cycles diagnostic areas and does not constitute tutorial progression. Near LOD/subdivision, alpha and dynamic palette state still need retail comparisons.

This checkpoint is a native technical prototype with broader shared content binding. It is not the full tutorial, a faithful playable remaster, or an accepted physical Old 3DS port.

## Checkpoint contents

The new complete code-only ZIP is named separately from all inputs and recovery archives. `CURRENT_CHECKPOINT.json` records its absolute path, adjacent SHA-256 file, local compiled binary/SD layout and screenshot gallery. `CHANGED_FILES_01411.json` lists cumulative changes relative to the frozen parent. `SOURCE_MANIFEST.sha256` covers the final source files.

Included: cumulative UTF-8 source, project/build files, tests, documentation and aggregate evidence. Excluded: reproducible caches/build outputs, raw ROM/CHD/BIN/CUE/EXE, retail saves, textures/models/audio/video and all cooked retail payloads. ZIP CRC and manifest identities are checked independently. Local SD data is owner-only and is not included in the code-only ZIP.

## Final exercised result

The final one-command installer passed all 20 CTest suites, all 205 package validations and nine repeatable native-versus-native replay scenarios. The compiled binary is 430,564 bytes, with SHA-256 `36502c9ce5af6d2be319aed709cd7069a12ad51273196724eec99a6ad10ed02a`. All 129 native snapshot identities match this source. The generalized cache reused 340 entries with zero misses on the retry; the changed tutorial recipe cooked 101 outputs. Failed attempts were preserved.

Azahar 2126.0, with New 3DS disabled, exercised the launch screen, all five baseline areas, movement, turning, firing, SELECT, safe0143 and one structurally distinct package. The broader package skipped its empty area 0 and rendered 492 primitive records in area 1. Its texture and gameplay bindings remain UNKNOWN. This establishes generalized native loading, not faithful broader gameplay.

Six original captures are preserved in the gallery listed in CURRENT_CHECKPOINT.json. Its captures.json records the original paths, settings, build and hashes. Area 0 and the post-controls view show the player and scenery; area 4 is obstructed. Terrain and player presentation defects remain. The firing and movement checks are backed by telemetry; the post-controls capture is not a retail combat comparison. No desktop render is presented as 3DS evidence. Desktop host tools and tests pass; no new desktop visual build was exercised.

Sparse five-area emulator samples recorded owned linear allocations of 16,320–363,360 bytes, owned textures of 1,048,576–3,670,016 bytes, and live malloc allocations of 632,464–1,293,104 bytes. Sampled frames ranged from 8,686 to 927,157 microseconds, including loading and synchronization. Loading spikes remain. These observations do not establish Old 3DS performance acceptance. The emulator reserved a 96 MiB application region, 32 MiB of linear memory and a 256 KiB stack. The physical budget and native graceful unload remain pending. Complete local traces stay outside the ZIP; released timing telemetry omits initialization coordinates.

# 0.0.14.16 — World Comes Alive continuation

This is a cumulative native technical prototype. **World Comes Alive is not finished**: the original encounter, combat, event/message and normal transition chain remain incomplete. PHYSICS_1TO1=false.

## Source and toolchain

Editable source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.16_WorldComesAliveIntegration`. Parent is the verified 0.0.14.15 Source ZIP, SHA-256 `494eaaa7fb0951397932e0991b436e5c83d313ee69de4aaa39f485ecb4e764cd`. No Git repository, branch or commit was supplied. Original archives, parent tree and synced sources were not changed.

Actual build uses devkitARM r68-1, GCC16.1.0, libctru2.7.0-1 and Citro3D1.7.1-2; ARMv6K/mpcore hard-float, 256KiB stack. It produced `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive/install-r1/build/Legends3DS_01416_WorldComesAlive.3dsx`: 553828 bytes, SHA-256 `3e32b79cfa506033777392cc0c944e2b370fc59b0122962e773f6a3fd59eef46`. Build warnings/errors: 0/0. All 154 native source snapshot hashes match current source.

## Research before implementation

Pinned primary sources corroborated the original stage overlays, separate map/EBD/message banks and table-driven object updates. The public decomp reference SHA1 matches the supplied executable exactly. Its incomplete implementation and viewer approximations served as navigation; no external code or assets were copied. See [research sources](research_01416/PRIMARY_RESEARCH.json) and [architecture](research_01416/ENGINE_ARCHITECTURE.md).

Original-instruction probes proved an IDX mutation region in 1120 synthetic cases. It changes both flags and descriptors, writing decimal0/100/200. It does not explain the earlier live gameplay value1, so no guessed map flag patch was applied. The current isolated PCSX-Redux observer has not reached the needed gameplay initialization. Memory cards are disabled; no original RAM, registers, pad ownership or save data were modified.

## Source changes and reuse

Reused the complete cumulative runtime, 0143 renderer, independently controlled renderer gates, ordered VRAM/CLUT resolver, actor mesh/pose decoders, area reload transactions and original animation caches. The 0143 shader is unchanged, SHA-256 `fd242f3023a04cb03ba80ee20ea716daff2f9921f54502f5eeefa1d3a2b373bb`.

Imported actor face UV/material identities now drive optional area GPU caches through the native draw path. The original first class03 material resolves to supplied EM0300 data; no unrelated palette or UV clamp is used. `actor_render_01416.cfg` containing `ACTOR_TEXTURES=1` enables this independent, default-OFF diagnostic. Original shade/alpha remain UNKNOWN; modulation is white and unlit. Missing, malformed or unresolved actor texture data retains gray rendering. Area rebuild/unload/shutdown release actor pages.

The exact original integer heading calculation, including wrapped arithmetic and zero-vector behavior, now feeds allocation-free imported-actor evidence queries with original distancework80. An optional, owner-local 80-byte L3AH sidecar supplies its table. Sparse native traces record results; they do not turn actors or implement AI/lock-on. The original outer callback also clears its hit packet after dispatch/animation/proxy submission; full callback sequencing remains unbound. See [changed files](CHANGED_FILES_01416.json) and [evidence ledger](../state/EVIDENCE_LEDGER.json).

## Tests and running builds

Inherited 36/36 and integrated 38/38 CTest suites pass Release with warnings-as-errors. Source heading: 1848 original-code synthetic-table cases plus462 local owner-table cases pass; malformed header/CRC/size/null and missing/range guards pass. Actor material and source/tool suites pass. Exact commands and outputs are in [build result](validation_01416/build-result.json), [host tests](validation_01416/host-tests.log) and [source angle evidence](research_01416/SOURCE_ANGLE.md).

Reproduce from source with:

```sh
cmake -S . -B /tmp/mml3ds-host-new -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/mml3ds-host-new --parallel 4
ctest --test-dir /tmp/mml3ds-host-new --output-on-failure
python3 tools/build_hardware_test.py --pack '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive/pack-runtime' --out /tmp/mml3ds-native-new
```

Choose fresh output directories. The build script verifies and reuses cooked data without recooking unchanged retail assets. The new owner-local pack preserves102 outputs and adds five actor caches and one angle sidecar; all108 output hashes validate.

| Target | Result and limits |
| --- | --- |
| Physical Old3DS/XL | HARDWARE_PENDING; no hardware test claimed. |
| Azahar2126.0 Oldmodel | Current compiled build boots and renders player/terrain/companionUI. Synthetic HID replay binds3 original class03 actors, emits54 heading/distance rows, uploads2 actor pages in area1 and1 in area2, and exercises movement plus a developer area transition. |
| Malformed optional actor cache | Actual native CRC rejection logged; runtime still reaches gameplay and actor pages remain absent. |
| Trusted0143 path | Featuremask0 boots the unchanged0143 shader with zero textures, despite actor texture request and absent optional angle file. Inherited stage label still logs requestedstage8; actual shader/feature behavior is recorded separately. |
| Host/desktop | Portable core and tools tested; no standalone desktop game acceptance claimed. |

## Memory, performance and captures

56 sparse enhanced HLE samples show malloc live1886680..1893640B, tracked texture3932160..4194304B and owned linear71640..82560B. Submission:15..18 draws and1710..2184 triangles. Largest sampled frame2.210676s includes diagnostic loading; update2..60084us, render9261..17008us. These are limited emulator observations, not physical FPS, worst-case pacing, stack peaks or complete memory measurements. All five EBD payloads567232B still remain resident.

Actor cache limits:1MiB decoded elements/64 entries,4MiB input buffer,512KiB RGBA+tiled upload temporaries and shared8MiB GPU cap. These are distinct bounds, not one memory total. No hot-loop allocation was added by the heading query.

Genuine original captures, device/build/scene labels and paths are in [screenshot inventory](SCREENSHOTS_01416.json). They are outside the source ZIP. The first capture shows the player and companion layout; after the diagnostic area transition, terrain occludes the camera/player. Original enemy texture fidelity and shading are not visually accepted. No desktop render or fabricated image is used.

## Remaining blockers and handoff

Resolve the actual gameplay IDX1 writer, domain and timing; shared map initialization must precede collision and geometry loading. Continue full source actor callback, detection/RNG/contact ordering, lock-on/Buster hit delivery, HP/reaction/defeat and proven event/message/normal-transition execution. Camera/terrain occlusion, dynamic palettes/shading/alpha, original top-screen HUD, audio/cutscenes/expressions, complete slopes/steps/ledges and true area streaming remain incomplete. Generic projectile/HP defaults are not retail evidence.

[Readiness matrix](../state/CURRENT_READINESS.json) keeps every subsystem and unknown explicit. [Resume note](RESUME_01416.md) gives the next concrete step. Source release: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.16_Source.zip` with SHA-256 sidecar. Code-only exclusions: all raw/cooked retail payloads, ROM/CHD/BIN/CUE/EXE, saves, audio/video, models/textures, compiled outputs, emulators, movies and screenshots. Reproducible __pycache__, .git and build_3ds are omitted. Complete inherited source, project/build scripts and relevant authored evidence are retained. No usage reset was used.

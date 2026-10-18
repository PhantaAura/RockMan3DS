# 0146d-r1 report — 2026-09-30

Latest status: a provisional Old3DS hardware-test technical prototype, not a faithful tutorial or full-game port. The d candidate was exercised through generated Azahar HID input; r1 fixes made afterward are built but not runtime-tested. Hardware acceptance remains pending. Work stops at a recovery checkpoint because a fresh usage check showed6% remaining in the current window.

## Source and toolchain
Exact source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.6d_SceneTest`. Continued supplied Integration Source and c-r2 changes. No Git repository, branch or commit exists. Original archives, synced sources and earlier ZIPs remain unchanged. Toolchain: devkitARM r68-1 / GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2, Picasso2.7.2-3, 3dstools1.3.1-3, 3dslink0.6.3-1. Native ARMv6K/mpcore target; mono top400x240/bottom console, no New3DS CPU-boost dependency.

## Reuse and changes
Kept RuntimeSession/map sharing, local cooked tutorial loader, player composition/animation, Citro3D frontend, existing projectile path, diagnostics and no-cook builder. Code changed: main.cpp, hardware_test.cpp, platform/3ds/Makefile, build_hardware_test.py, material_cache.cpp, CMakeLists.txt, test_ps1_geometry.cpp; added ps1_geometry.h and ps1_texture_modulation.h. Documentation/build/audit records are also retained.

Current r1 keeps the PS1 packet-order quad correction on the PBD player, r2 UV origin, camera X correction, D-pad turn mapping and valid0x8000 black texel handling. It corrects neutral texture modulation to /128 with TEV RGB scale2 and handles raw-texture color bypass; synthetic tests cover neutral, saturation and raw behavior. [PS1 color semantics](https://psx-spx.consoledev.net/ps1/gpu/rendering-attributes/#color-attribute-parameter-for-all-rendering-commands-except-raw-texture).

The earlier d terrain strip change was rolled back: an audit of authorized cooked ST00 data found mixed order and incomplete geometry. Among3,622 strict convex records,558 pass strip only,1,026 pass fan only,2,038 pass neither. Restoring the supplied fan is containment, not a terrain accuracy claim. See terrain_topology_audit.md/.txt/.cpp. No geometry was invented or asset files modified.

## Builds and checks
Both d and r1 actual native Make builds PASS; r1 CMake Release build and CTest6/6 PASS (core, map_sharing, ps1_geometry including palette/modulation checks, push2, integrity, pipeline_python). No native warning/error lines were found. No cooker/extractor executed.100 cooked files and exact manifest inventory verified before and after SD copy. Hash validation is not proof of original-game fidelity or rights.

R1 binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/validated-d-r1/build/Legends3DS_0146d_r1_SceneTest.3dsx`; 381036 bytes; SHA256 `6ab74dcdda0240159921da99125b457c2560220c1b140dd8103b28514eaf03c4`. All native snapshot hashes match current native source. One initial edit command used the wrong working directory; it was corrected before native snapshot, and the snapshot was explicitly checked to contain the terrain rollback. Logs/results in docs/validation_0146d_r1. Rebuild: `python3 tools/build_hardware_test.py --pack /absolute/existing/tutorial --out /absolute/fresh/output`.

## Runtime evidence
Azahar2126.0, Old-model mode, layout2 /96MiB app region. A generated46-second CTM replay exercised the d native HID path: A launch, Circle Pad forward, L/R turns, Y projectile creation, SELECT to area1 and forward movement there.49 CSV samples show changing positions/headings, max one projectile and actual area transition. Emulator log recorded LoadedMovie/Playbackfinished, with zero HID type mismatch messages. This is synthetic emulator replay evidence, not manual controls or physical hardware. Original CSV/boot log and sanitized replay log in docs/runtime_evidence_0146d; full original emulator log remains privately in work/pass-20260930/runtime-evidence-d.

D fidelity FAIL: fragmented terrain and dark/scrambled player textures remain visible. R1 runtime/palette/visual improvements are PENDING; no success claim. Desktop portable library/tools/tests PASS, but no desktop graphical game exists. Original Old3DS user test has not been reported.

The direct3DSX Azahar loader hardcodes Dev1 /96MiB despite Old-model selection. No supported64MiB direct-launch setting was found; see OLD_MODEL_MEMORY.md. Emulator execution cannot prove standard64MiB compatibility or physical pacing.

## Performance and memory observations — tested d candidate only
Area0:11 draws,4,020–4,021 triangles, max live malloc934,968B, owned linear210,240B and textures1,835,008B; median sparse frame-before-log16,711us. Area1:13 draws,2,224 triangles, max live malloc1,202,360B, owned linear102,480B and textures2,883,584B; median20,703us. Area1 transition sampled748,161us. Sparse samples include synchronization and exclude their current logging write; these emulator figures are not hardware FPS, peak memory or worst-case pacing. Full-area rendering, player allocations/sorts and largest area remain risks. No r1 runtime measurements.

## Genuine captures
Preserved originals, Azahar2126.0 / build0.0.14.6d / scripted input:
- `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/screenshots/01-Azahar2126-0146d-replay.png`: area0, animated visible player and both-screen diagnostics, displayed frame528/position(-512,0,-3265); terrain/textures defective.
- `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/screenshots/02-Azahar2126-0146d-replay.png`: area1 after movement, replay finished; visible player/terrain and bottom diagnostics. Console text is partially captured.
Initial d title was visibly observed but no original title image file was saved. No r1 or hardware captures, and no screenshot proving a projectile is available. CSV proves projectile creation; filename is not used as visual proof. Earlier c captures remain separately identified in the historical report.

## Bugs and unported features
Missing3 terrain material words affect1,184 primitives. ST00 coordinate/order/height/LOD and original palette ownership remain unresolved. Player facing, original animation and texture placement equivalence are unverified; PS1 semitransparency blend modes are absent. Rectangle-only collision, missing reverse/jump/enemy targets/tutorial events/natural transitions/cutscenes/audio output/touch/native save UI. SELECT is diagnostic area cycling. No faithful original-run reference provided. Full-game importer is blocked under the current restrictions until permitted source/loose asset inputs exist. Only the already authorized cooked tutorial pack was used locally; no commercial EXE/CHD/BIN extraction or circumvention. Commercial assets are excluded from the source ZIP; local SD folder must not be redistributed.

## Handoff and next step
Local r1 SD folder: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0146d_r1_LocalHardwareTest_20260930/sd_out`. Guide: HARDWARE_TEST_0146d_r1.md. Complete source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0146d_r1_SceneTest_Source_20260930_002.zip`. Generated output/caches, commercial game-data pack, raw binaries and private emulator configuration are excluded; source/docs/tests retained. Earlier ZIPs preserved.

Azahar is CLOSED. Eight model/CPU/SD test settings and all six original application folders were restored and verified. No temporary profile remains active. Next: temporarily isolate r1 for generated native-controls-capture-generated.ctm, capture title/movement/shot, inspect CSV/log, quit and restore; then physical Old3DS test with CSV/photos. Current milestone remains provisional until hardware acceptance and rendering defects are resolved.

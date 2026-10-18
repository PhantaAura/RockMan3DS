> Historical report for the original d candidate before runtime testing. Current source is d-r1; terrain strip was rolled back after the cooked geometry audit. See RESUME_0146d_r1_20260930_002.md and forthcoming r1 report.

# 0.0.14.6d scene-test report — 2026-09-30

## Assessment
A native Old 3DS technical prototype and provisional hardware-test build, not a faithful full tutorial or full-game port. The new executable and host checks pass. This version has not yet been exercised in gameplay. Hardware acceptance is pending the user's original Old 3DS test. Do not equate compilation with successful porting.

## Source identity and preservation
Exact project: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.6d_SceneTest`. Continued from `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.6c_HardwareTest`; its 113 checked source/header/shader files matched the preserved compiled r2 snapshot before this pass. The supplied origin was `/Users/emeraldhunter/Documents/Legends3DS_0.0.14.6a_Integration_Source.zip`, with prior b/c changes retained. No Git repository exists here: branch and commit are unavailable. The new source copy is isolated; original archives, previous checkpoints and synced sources were preserved. See `0146d_BASELINE.json` and the source manifest.

## Toolchain
Actual native build: devkitARM r68-1, GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2, Picasso 2.7.2-3, 3dstools 1.3.1-3, 3dslink 0.6.3-1, under `/opt/devkitpro`. Make targets ARMv6K/mpcore hard float with standard 3dsx specs. No New 3DS CPU boost, extra-core, C-stick, ZL/ZR or MVD requirement. Mono 400x240 top screen and bottom console; stereo disabled. Native stack and command buffer remain 256 KiB each.

## Reused systems and changes
Reused RuntimeSession/map sharing, material pages, player composition/animation, terrain loader, input profile, projectile path, UI diagnostics and no-cook builder. Changes this pass:
- `platform/3ds/source/main.cpp`: enhanced terrain/player quad topology uses packet-strip diagonal; camera trails movement after turning; D-pad left/right emits the installed profile's turn masks; build/HUD/log identity updated. The 0143 renderer fallback keeps its previous quad split and player rotation.
- `include/legends3ds/render/ps1_geometry.h`: small shared quad-index definition.
- `src/texture/material_cache.cpp`: only the complete zero PS1 word is discarded; valid 0x8000 black is retained with STP alpha. This does not add full PS1 blending.
- `tests/test_ps1_geometry.cpp`, `CMakeLists.txt`: synthetic square/trapezoid intersection/union and UV coverage checks, old fan negative control, palette zero/black/red/white tests. Release assertions remain enabled.
- `platform/3ds/source/hardware_test.cpp`, `platform/3ds/Makefile`, `tools/build_hardware_test.py`: versioned executable/title/CSV, retaining existing SD contract.
- README and current audit/porting map/build/handoff documents.

The geometry fix follows [PS1 GPU polygon research](https://psx-spx.consoledev.net/ps1/gpu/render-polygon-commands/#notes). Palette handling follows [texture-word semantics](https://psx-spx.consoledev.net/ps1/gpu/video-memory-vram/#texture-bitmaps). The source preserves packet vertex order; using a perimeter fan caused overlapping coverage and missing regions on a synthetic strip-ordered square. The camera fix follows the supplied runtime's forward vector (-sin,-cos); model facing is a separate unresolved issue. The prior r2 bottom-origin V correction is preserved.

## Build and regression results
Command: `python3 tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/full-build/tutorial/legends3ds/tutorial --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/validated-d`.
CMake Release build PASS; CTest 6/6 PASS (core, map_sharing, ps1_geometry, push2, integrity, pipeline_python). Native Make PASS with no warning/error lines in native-build.log. Host tools were compiled; no extraction or cooking was invoked. Cooked pack validated 100 file sizes/hashes and exact inventory, before and after SD copy. These checks prove structure/integrity, not original-game accuracy or rights. New executable is 380,828 bytes, SHA-256 `6349eafdea17edf35476aba4bd29d1a5c2a2ba185638605cec8cb2b0e4ed771b`. Logs and results are in `docs/validation_0146d` and `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/validated-d`.

## Runtime evidence and limits
- Old 3DS hardware: user has a console; no test loaded/reported yet.
- Azahar 2126.0: preserved r2 title was opened this pass in temporarily selected Old-model mode. Automated keyboard taps did not start the game. A synthetic CTM replay validated, but was not started because the application list selected a different homebrew (program-ID zero collision). Closing that dialog left automation unable to read Azahar's window. User was asked to close it before settings restore and a safely isolated d test. No d gameplay or replay success is claimed.
- Prior c-r1 emulator gameplay captures show the player and broken terrain/textures; sampled input did not establish movement. Prior r2 title capture proves title launch only. Both are historical evidence, not evidence for this d build.
- Desktop: portable core/tools/tests work. There is no supplied desktop graphical frontend, so no desktop gameplay capture or regression claim.

Generated controller replays in `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/replay-diagnostics` contain input records only, not game data. They remain runtime-untested and must be labeled synthetic replay if used later. Physical controls still require hardware testing.

## Performance and memory
No new d runtime measurements are available. Prior c-r1 Old-model emulator run reported layout 2 and a 96 MiB app region, not the usual 64 MiB application region. It recorded approximately 0.89 MiB live malloc, 205 KiB owned linear allocation, 1.75 MiB for seven RGBA8 texture pages, 28.3 MiB linear free and 5.27 MiB VRAM free at area 0. Area-0 submissions were 11 draws / 4,020 triangles; sampled frame-before-log timing ranged roughly 13–83 ms. These are historical emulator observations, not physical limits, peak memory or an achieved frame-rate promise. Full-area rendering, allocations/sorts each player frame and the 12,112-primitive largest area remain risks. No culling/overdraw/loading optimization or stereo work was added.

## Captures retained
All following paths are genuine previous Azahar 2126.0 captures, separately identified:
- `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/emulator-test-20260926/screenshots/01-launch-Azahar2126-0146c.png`: c-r1 gameplay, area 0; filename misleadingly says launch. Player and texture/terrain defects visible.
- `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/emulator-test-20260926/screenshots/02-movement-projectile-Azahar2126-0146c.png`: c-r1 later gameplay; filename is not proof of movement/projectile input.
- `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/emulator-test-20260926/screenshots/03-title-Azahar2126-0146c-r2.png`: c-r2 diagnostic title, top black/bottom text.
No new d gameplay screenshots or hardware captures exist yet. Do not fabricate or re-label previous captures as d evidence.

## Missing and unported features
1,184 terrain primitives across areas 0/1 have unresolved materials (three material words); area height/LOD and original palette ownership are unverified. Player model facing, animation equivalence and texture modulation above neutral/raw-texture handling remain unresolved. Semi-transparency blend flags/modes are not implemented. Collision currently limits movement to the area rectangle, not complete walls/floors. Reverse/jump, native enemy targets/damage, event progression, natural transitions, cutscenes, real audio output, touch UI and SD save UI remain incomplete. SELECT cycles diagnostic areas. There is no full original-run reference/golden trace to establish faithfulness.

## Assets and importer
Only the existing cooked tutorial pack was used, locally, with the user's express authorization. No commercial EXE, CHD or BIN payload was examined or extracted this pass; no circumvention and no new extraction/cooking occurred. Supplied game ZIP metadata exposes disc images and split/nested archive-looking members, with unknown nested contents and no visible loose licensed asset inputs. Full-game import remains blocked under the current restrictions until permitted loose asset/source inputs are supplied. The no-cook builder accepts complete manifest-verified cooked packs only. Commercial assets/pack are excluded from source ZIP; SD folders must stay local.

## Handoff
Local SD root: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20260930/validated-d/sd_out`. Hardware instructions: `docs/HARDWARE_TEST_0146d.md`. Complete source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.6d_SceneTest_Source_20260930_001.zip`. Verified source membership, CRC and file bytes. Local handoff: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0146d_LocalHardwareTest_20260930`. Original archives and recovery ZIP are preserved. Generated output/caches and local commercial game data are excluded. Next concrete step: close unresponsive Azahar, restore/test settings safely, isolate this native build for replay, then validate on original Old 3DS using the checklist. A faithful scene result and full-game port are not yet verified.

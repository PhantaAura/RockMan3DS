# 0.0.14.12 visual reference and bottom UI checkpoint

Continues cumulative0.0.14.11 at `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.12_VisualParity`. No Git repository, branch or commit. Source-only checkpoint: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.12_VisualParity_Source_20261003_003.zip`. No original inputs were overwritten; synced `sources/` remains read-only.

The native UV renderer now applies the original baseline player local scale1/8, corrects the Y rotation sign and shares a measured ordinary follow-camera with host inspection. Original PS1 GTE H384 at320x240 gives34.708-degree vertical FOV. Native400x240 preserves that vertical framing and adds scenery horizontally, as requested. This does not establish full1:1 camera or rendered output.

Original player components/materials/terrain/generalized loader/VRAM composition are reused. Player pose and material storage are retained after startup; the bottom map borrows live cells. New native bottom UI uses the mockup's three-column layout, selectable tabs, actual HP/Buster/Lock and a bounded map pan/zoom model. Inventory/loadout remain unavailable, with no fabricated stats or artwork. Original top-screen HP/Buster HUD is absent: its page/CLUT/resource binding is UNKNOWN. The source top HUD must be recovered before its1:1 requirement is satisfied. Buster triangles remain an explicit CANDIDATE proxy; no guessed original effect/model replaces them.

## Build and test evidence

Inherited20/20 CTest suites passed before edits. Final Release host build passes23/23 suites including inherited importer/package/malformed input/save/provenance/replay/VRAM tests and new view/UI/player storage checks. Focused view/UI/player checks pass AddressSanitizer+UBSan (modified units instrumented; inherited linked library is not fully instrumented). Player output equivalence passes128 synthetic poses,98 actor cases and761 supplied pose frames across55actions, with zero allocations after pose startup. Original bounded source camera/transform checks total346 cases. These checks are not retail replay parity.

`install-r2` failed because a projection fixture expected a point outside the newly narrowed reference frustum to remain visible. It was corrected to exercise an in-frustum point and explicitly reject the former point under reference FOV while accepting it under90degrees. Failure log is preserved as `validation_01412/failed-r2-host-tests.log`. A native declaration-order error was diagnosed and fixed with a forward declaration. No failing tests were disabled.

Final actual toolchain: devkitARM r68-1/GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2, ARMv6K/mpcore. Native build455340bytes SHA256`d4c5d88ab40424553b93c2d88534f74dbb0f843ec138f1f5da3c952989cd3416`. Binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/install-r3/build/Legends3DS_01412_VisualParity.3dsx`. Local-only SD: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/install-r3/sd_out`. Native source snapshot hashes all match final code. Safe0143 shader/draw bodies retained; current safe renderer rerun pending (parent had emulator exercise).

Commands and logs are recorded in `validation_01412/build-result.json`. Reproduce without cooking/extraction:

```sh
python3 -B tools/build_hardware_test.py --pack /absolute/existing/tutorial --out /absolute/fresh-output
```

Independent host:

```sh
cmake -S . -B /absolute/host-output -DCMAKE_BUILD_TYPE=Release
cmake --build /absolute/host-output --parallel 4
ctest --test-dir /absolute/host-output --output-on-failure
```

## Emulator and reference results

Azahar2126.0, `is_new_3ds=false`, CPU100%, resolution1: final binary boots title/ready, terrain and bottom UI render; synthetic Circle Pad/turn/fire/SELECT controls exercised. Camera/player can remain inside scenery at diagnostic entries or after movement. This failure is preserved in captures and prevents visual/playable acceptance. Native touch gestures have not yet been successfully exercised in emulator; host/raster/gesture tests pass. No physical Old3DS testing occurred. A desktop host tool build passed; there is no current desktop gameplay build evidence.

PCSX-Redux f78f8ef8/dev290 interpreter with supplied BIN/CUE and bundled OpenBIOS supplies genuine original320x240 references before first enemy contact. H384/center160,120 stable303vsyncs; original idle height113-115pixels, live baseline scale512/Q12 and camera offsets recovered. These are reference measurements, not equal native pixel output. ARM64 dynarec crashes; directCHD read is invalid. See `PS1_REFERENCE_01412.md`; user saves untouched.

Original captures and emulator metadata: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/SCREENSHOTS.md`. They are owner-local evidence outside sourceZIP. Captures must retain their device/build/scene labels; desktop host renderings are not Old3DS evidence.

## Memory/performance and remaining work

Actual area0 boot logs report linear-owned59520B, texture-owned2883584B and11 textures; area1 reports71640B and3670016B. Per-frame CSV records update/render/bottom_ui, draw calls, triangles, heap/linear/VRAM. Emulator timings do not prove hardware budgets. New pose/map paths avoid hot-loop allocation; overall runtime allocation behavior is not proven zero. Stack remains256KiB; total Old3DS main RAM/headroom and loading spikes require physical measurement. Narrow ordinary camera exposes earlier spawn/contact weaknesses rather than solving them.

Current result is a native technical prototype, not a full tutorial or complete remaster. PHYSICS_1TO1=false. Spawn/contact/grounding/walls, dynamic/scripted camera, original HUD, objects/enemies, event/progression, natural transitions/cutscenes/audio/save fidelity and visual material timing remain incomplete. See readiness for each subsystem. Next concrete step: recover original area spawn/ground contact and camera obstruction/state behavior using the paused reference; then bind original HUD resources with page/CLUT evidence. Physical testing and actual native touch/pan/zoom validation remain pending.

## Release exclusions

The cumulative sourceZIP includes source/build scripts/project files/text documentation/evidence ledgers/tests and a SHA256 manifest. It excludes reproducible caches/generated binaries, ROM/CHD/BIN/CUE/EXE, retail save data, Capcom textures/models/audio/video and every cooked payload. Local SD output contains owner data and must not be redistributed. Hash identity is not proof of ownership or legal permission. No usage reset redeemed.

## Current capture and timing inventory

Original title and area1 native window captures are retained with hashes/Old-model configuration in `validation_01412/capture-manifest.json` and `emulator-launch.json`; the wall-obstructed scene is intentional failure evidence. A playback-completion dialog is excluded from gameplay proof. Actual sparse nativeCSV/boot logs and per-area median/max counters are saved alongside `emulated-performance.json`. Touch checks could not exercise the native screen through this session's window binding, so native touch status stays pending. Focused sanitizer compile/run commands and return0 results are in `focused-sanitizer-commands.json`.

# 0.0.14.7 cumulative area-runtime checkpoint — 2026-10-01

This is a buildable technical prototype, not a faithful full tutorial or completed port. A generalized envelope now feeds geometry/contact data into the same native runtime rather than remaining host-only. Current native execution, renderer appearance and Old3DS acceptance are pending: the Mac was locked when Azahar was requested.

## Exact source and history

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.7_AreaRuntime`. Parent is local0.0.14.6d/r1, descended from supplied6c work. Parent ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0146d_r1_SceneTest_Source_20260930_002.zip`. No Git repository/branch/commit. Synced sources, old working trees and earlier ZIPs were not replaced. The current metadata was corrected from stale inherited6b fields; historical metadata is separately preserved. The frozen0143 shader is unchanged.

## What was reused and changed

Reused neutral MML2 envelopes, L3MP/L3RG readers, RuntimeSession and source-backed player helpers, contact queries, terrain expansion, existing Citro3D/0143/UV paths, player composition/animation/reference assets, and authorized local tutorial sidecars. This remains the Mega Man Legends port/reimplementation; no new replacement game content was authored.

Added a streaming bounded area loader. It CRC-validates all sections once, preflights decoded map/render structure and nested allocation ranges, rejects corrupted/overlapping/amplifying records, and decodes required sections2/3. Other payloads stay descriptors; no invented object/EBD/message behavior is executed. Packages are move-only and unload releases capacity. The native Makefile includes this new loader. The frontend tries areas/boot.mmlpkg, then routes the moved map through session.active_map and the loaded render records through the existing renderer. Package rejection uses the loose-data path.

An isolated compatibility adapter compares the package map/render bytes to the existing local pair before binding tutorial materials/flow/messages. Different packages inherit no unrelated tutorial palettes/events. Empty/over-budget native terrain areas are rejected before committing. Expanded geometry is counted without allocations; each CPU/linear vertex buffer has a4MiB admission cap using the existing six-vertex quad expansion. This retains the current topology rather than claiming it faithful.

Moved boot map/messages rather than retaining copies. Added unregister_location/release_inactive_locations with weak lifetime diagnostics. Removed inactive map ownership is proven by host tests; active player/flags/save state is preserved. Current native boot still retains one shared map/render package and all its local map areas; complete cross-package progression streaming is not finished. Outgoing GPU resources and CPU material caches are released before next-area cache decoding. Added before/after heap/linear/VRAM journal samples, separate from allocation estimates. Existing per-frame allocations remain a risk.

Added a cooked-only wrapper and integrated it into the no-extraction build workflow. The verified map/render pair is wrapped in a109672-byte MML2 package with input SHA provenance. No extractor/cooker/raw commercial input was invoked. Public source ZIP has no cooked payload; local SD handoff must not be redistributed. The browser/ROM installer is documented as a future local-data workflow, not implemented.

## Validation

Inherited parent Release build/CTest:6/6 PASS. Final0147 Release build/CTest:8/8 PASS (core, map_sharing, ps1_geometry, area_package, push2, integrity, pipeline_python, cooked_area_python). Python wrapper has14 synthetic cases. New loader tests cover two synthetic identities through the existing session/contact/render helpers, malformed sections, corrupt skipped payloads, budgets, unload/failure preservation, and1000-cell/shared-stream amplification. Warnings-as-errors applies to portable targets.

ASan/UBSan Debug build and targeted map_sharing/area_package tests:2/2 PASS, no sanitizer reports. Actual native build:PASS; no warning/error lines in final native log.117 native source/header/shader/Makefile snapshot hashes equal this source tree.

Toolchain verified locally: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2. ARMv6K/mpcore target. No New3DS speed-up requirement introduced. Standalone compiled3DSX: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001/validated-0147-final/build/Legends3DS_0147_AreaRuntime.3dsx`;403236 bytes; SHA-256`82328aa1b9934ca21335712b020f7f81cce1f2fabac8566b4786f29e84fdd34d`.

Exact commands/results are in docs/validation_0147/build-result.json, baseline-result.json and sanitizer-result.json; original logs preserved there. Top-level invocation:

```sh
python3 tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/tutorial-pack/legends3ds/tutorial --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001/validated-0147-final
```

No desktop graphical game exists; desktop validation is portable libraries/tools/tests. Current emulator/hardware:NOT EXERCISED. CUA reported that the Mac is locked and automatic unlock failed. No current captures exist; earlier6d captures remain in historical reports and are not labeled0147.

## Package proof and measurements

Host probe of the existing authorized wrapped pair:PASS; all five map areas bind contact queries and expand1752/854/674/120/12112 Mid/Primary primitives, total15512. This is tutorial data plus two synthetic identities, not broader early-game gameplay proof. The host allocation estimates are184804 decoded bytes and353452 loading-peak bytes; sizeof values are host-specific and include conservative per-allocation allowances. They are not live RAM measurements or ARM/hardware peaks. Native admission policy is file8MiB, section/loose4MiB, decoded estimate4MiB, loading peak estimate8MiB, <=64sections, <=256map areas. No current frame-time, loading-time, VRAM or hardware memory measurement is available. See the older6d report for separately identified historical emulator observations.

## Blockers and unfinished behavior

Terrain order/height/UV fidelity and three material words affecting1184 primitives remain unresolved. R1 player color/material/facing/animation corrections are retained but current GPU appearance is untested. No semitransparency equivalence. Full floor/wall/slope/ledge/step solver, reverse/jump/recovery, retail camera, lock-on, damage/enemy behavior, EBD operations, natural transitions, cutscenes, expressions, audio playback and save UI remain incomplete. PHYSICS_1TO1=false; no retail golden trajectories. Object-list and EBD/message/material bindings require an explicit evidence-backed neutral contract. Do not execute retained directory offsets as scripts or assign enemy roles by guess.

All37 requested readiness rows are in state/CURRENT_READINESS.json using only allowed evidence labels; no percentages. Evidence ledger and changed-file list are included. No broad importer or executable research rerun occurred; inherited205-package/4115-node/3916-edge figures remain structural historical results.

## Handoff

Complete cumulative code-only ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.7_Source_20261001_001.zip`. Its external.zip.sha256 file records the final ZIP digest. Local-only verified SD folder: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0147_LocalHardwareTest_20261001/sd_out`. The copied SD file inventory/hashes match the validated output exactly. Exclusions are listed in SOURCE_ZIP_EXCLUSIONS_0147.md. Resume note names exact paths, changes, commands/results, blockers and next step.

Next: when the Mac is unlocked, run0147 in isolated Old-model Azahar and verify PACKAGE HUD, controls, CSV area events, title/gameplay captures and safe fallback. Physical Old3DS profiling remains required. Then continue the engine pass with proven generalized material/object/message/event bindings and the unresolved scene-fidelity work; do not restart or claim full retail parity.

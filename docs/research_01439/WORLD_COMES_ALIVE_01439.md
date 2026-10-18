# World Comes Alive 0.0.14.39 — cutscene prop interaction

The user's door-lift and anchored-shard feedback identified missing runtime behavior. This pass ports the original prop controllers and connects their transforms to the existing imported meshes. It preserves the cumulative engine and 0143 fallback. World Comes Alive is still a technical prototype, not a completed tutorial or finished port.

## Source and toolchain

Exact project: `work/Legends3DS_0.0.14.39_WorldComesAlive` under this project mirror. There is no Git repository, branch or commit. Baseline is the preserved `Legends3DS_0.0.14.38_Source_r2.zip`, SHA-256 `43720d9ddd2ebf93d99eb0e1febb23bba837b07928f34c32dbb6c018ea42c519`.

Fresh local toolchain observation: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2. Actual ARMv6K/mpcore hard-float target compiled. No New 3DS-only dependency was introduced. Physical Old 3DS has not been tested.

## Implemented and reused

- Original door initial pose, bounded lift curve consumer, linked drop/reset branches, and complete typed RNG/child transaction preflight. Changing lift stays held when genuine receipts are absent. No lift speed or random values were invented.
- Original shard attachment states1/2/3, including bone3 rotation/scale/offset, averaged bone positions, integer/GTE saturation and opaque temporary-lane invalidation. The auxiliary release branch retains its unusual original foreign-global addresses and holds without their owner.
- SceneE0 and RuntimeSession bind current event/class40/E0 ownership, actual known sequence frame from the resident EBD, and optional validated384-byte L3PL curve profile. No hot-loop package decoding or allocations.
- Gated E0 rendering partitions retained texture STP and maps original ABR modes. Bit25 no longer means unconditional invisibility. Existing textures, CLUT identities, meshes and integer pose banks are reused; no replacement artwork or borrowed palettes.

The source top-screen HUD and bottom-screen UI were not redesigned in this pass. The safe renderer is preserved and remains independently selectable. GPU blend quantization, original ordering/depth and live CLUT override are still unverified.

## Verification

Inherited host suite:134/134 PASS before integration. Final suite:136/136 PASS in5.98 seconds. Native-r1 initially ran135/135; the lift test target was added afterward without changing any native inputs. All279 source-input SHA-256 values match the actually compiled snapshot.

Original-instruction comparisons:202 lift/drop cases,2759 public guards or3143 with the private curve profile;180 attachment cases and6156 guards;256 material packet cases plus16 ordering cases,768 texel classifications and four ABR plans. These are explicitly authored contexts executed through unchanged original instructions, not live full-game parity. See individual proof ledgers.

Actual native build:PASS,858900-byte `.3dsx`, SHA-256 `83a6f0252044f69db5f0a53544f123f290bf465ac6187fbab1b80a9b46e660e4`. Native build log has no warnings. Private SD layout is `work/pass-20261014-worldalive-01439/native-r1/sd_out`. The executable needs the existing owner-local cooked data; it is not a standalone retail-data distribution.

Old-model Azahar2126.0 uses `is_new_3ds=false`, CPU100%. Execution logs admit L3PL, show independent door Y writes/drop down to−50 and initial bone-owned Y−53, and submit15 E0 instances/1 semitransparent instance/16passes. A RuntimeSession test separately verifies shard XYZ/matrix/modulation reach its draw view. Correction r1: both the original locked run and unlocked rerun reached primary shard attachment states1/2/3, sampled at1800/1920/2520. The previous contrary interpretation was incorrect. Auxiliary slot14 still holds before attachment/release and stays at its original anchor. Per-slot attribution of the visible duplicate remains an inference.

macOS reported a locked screen, blocking CUA capture/input. Unlock was requested once. No new screenshot or visible-pixel acceptance is claimed. Existing historical captures are not relabeled as .39. The exact isolated test process was closed by SIGKILL after SIGTERM did not exit and the GUI remained locked. No original data was touched. Exact fallback drawing source matches the .38 baseline; no new fallback pixel acceptance is claimed.

The strict first RNG hold remains at frame2609: actor800C3580 and scratch9336 match; native lower80 allocation0 differs recordedE0000000. No pool values, positions or random samples were forced to advance it.

ARM static frames: attachment304B; lift456B plus40B receipt lambda; material plan36B; load-only curve loader672B. These are function frames, not measured total stack peaks. Emulator samples retain owned linear59520B, textures4980736B and malloc-live2190896B; they are not physical or peak memory proof. No new texture recook or residency expansion was needed. Timing after a held scene cannot establish gameplay performance on Old 3DS.

## Exact commands

From the project mirror root:
```sh
cmake -S work/Legends3DS_0.0.14.39_WorldComesAlive -B work/pass-20261014-worldalive-01439/host -DCMAKE_BUILD_TYPE=Release
cmake --build work/pass-20261014-worldalive-01439/host --parallel 4
ctest --test-dir work/pass-20261014-worldalive-01439/host --output-on-failure
python3 -B work/Legends3DS_0.0.14.39_WorldComesAlive/tools/build_hardware_test.py --pack work/pass-20261014-worldalive-01439/pack-runtime-r1 --out work/pass-20261014-worldalive-01439/native-r1 --host-build work/pass-20261014-worldalive-01439/host
work/pass-20261014-worldalive-01439/host/test_source_prop_lift work/Legends3DS_0.0.14.39_WorldComesAlive/tests/fixtures/source_prop_lift_01439/callback-authored.tsv work/pass-20261014-worldalive-01439/pack-runtime-r1/source_prop_lift.l3pl
work/pass-20261014-worldalive-01439/host/test_scene_e0_material work/pass-20261014-worldalive-01439/e0-material-private-r1/oracle-r1/source-e0-material.tsv
```
Choose a new native output directory when rerunning rather than overwriting evidence. Owner-data paths are local only. Raw proofs and engine setup remain private; REA's previously unsupported PS-X carrier/absent engine was not retried. Universal Modder's real-code/oracle workflow was applied with the existing original-instruction interpreter.

## Remaining work and checkpoint

Next: establish genuine RNG/effect inputs for the door and competing lower callbacks; retry the actual opening/grab at matched phases; bind the auxiliary release globals; complete the original script-to-gameplay handoff. Full enemy combat, triggers/messages/natural area transitions, broader normal progression and physical Old 3DS validation remain incomplete. PHYSICS_1TO1=false.

`state/CURRENT_READINESS.json` uses evidence labels only. `docs/CHANGED_FILES_01439.json` lists cumulative changes. Final code-only archive: `deliverables/Legends3DS_0.0.14.39_Source_r1.zip`, with adjacent SHA-256 file. The ZIP includes source, build scripts, authored tests and documentation. It excludes all owner retail binaries, models/textures/audio/video, cooked packages/control profiles, DSP, saves and generated build output. Reproducible caches are excluded. Original inputs and earlier checkpoints are preserved.

## Unlocked capture follow-up —14October2026

The same858900-byte binary was rerun unchanged in Old-model Azahar2126.0, CPU100%, with the private SD data. Two genuine source-scene captures and one0143fallback capture are saved. The fallback boot log reaches first-frame/runtime-ready with no textures resident and renderer gates0. Both capture-test PIDs exited after GUI quit. No emulator update or rebuild was performed.

These images show imported geometry/player/materials, scripted approach/grab and the bottom-screen prototype. They do not establish original PS1 pixel parity, physical performance or a completed tutorial. The auxiliary anchored representation and changing-lift RNG dependency remain unresolved. Trace review identifies the primary shard motion in BOTH earlier and current runs; the previous unreached-grab claim is explicitly corrected in the evidence ledger.

Capture manifest: docs/validation_01439/capture-validation-r1.json. Trace correction: docs/validation_01439/capture-trace-review-r1.json. Source archive revision r1 changes evidence/documentation only and preserves the original ZIP.

- Scripted source opening and shard interaction; primary attachment plus anchored auxiliary representation: [_14.10.26_16.08.45.489.png](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01439/azahar-01439-props-r2/captures/_14.10.26_16.08.45.489.png)

- Scripted source opening and shard interaction; primary attachment plus anchored auxiliary representation: [_14.10.26_16.12.08.757.png](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01439/azahar-01439-props-r2/captures/_14.10.26_16.12.08.757.png)

- 0143 fallback boot; advanced rendering disabled: [_14.10.26_16.15.45.323.png](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01439/azahar-01439-safe-r1/captures/_14.10.26_16.15.45.323.png)

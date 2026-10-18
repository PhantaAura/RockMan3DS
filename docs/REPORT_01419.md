# 0.0.14.19 World Comes Alive continuation

Imported actor detector, contact, pose, stats and sound producers now connect to the native runtime. This is a cumulative technical prototype. **World Comes Alive is unfinished; PHYSICS_1TO1=false.**

Source: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.19_WorldComesAlive
Parent: 0.0.14.18 archive SHA256 5d4cf0183855fa22d7eec0ca2d22b09cfa57acf2a6aba1f4fb8af0e719af7776. Archive-derived source; no Git repository, branch or commit.

Toolchain: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2; ARMv6K/mpcore hardfloat. The0143 fallback is preserved; enhanced captures are not a fresh fallback acceptance test.

Changes: bounded original detector math/occupancy composition; action3 reducer/contact preparation; fixed24x20-byte transactional sound queue; strict sparse L3CB combat fields and L3DP/L3NR/L3DB metadata; fresh selector lifecycle; area scratch from cooked flow byte38; genuine imported request3; stale snapshot/class/cue guards including signed cue bits; cooker provenance and optional reusable host cache. Existing importer, EBD meshes/poses, collision map, renderer, session, bottom UI and atomic-save systems reused. Exact list: CHANGED_FILES_01419.json.

Inherited46/46 tests passed before edits. Final58/58 host suites pass, portable warnings-as-errors. Actual Old3DS build passed with zero warnings/errors;178 compiled source hashes match current code. Earlier request/fixture/cue failures and fixes remain in integrated-tests-r1/r3 and integrated-native-r1 logs. Final output is integrated-native-r2.

Exact successful command from source:

```sh
python3 -B tools/build_hardware_test.py --pack ../pass-20261006-worldalive-01419/pack-runtime-r1 --host-build ../pass-20261006-worldalive-01419/integrated-host-r1 --out ../pass-20261006-worldalive-01419/integrated-native-r2
```

Build result: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261006-worldalive-01419/integrated-native-r2/build-result.json
Binary: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261006-worldalive-01419/integrated-native-r2/build/Legends3DS_01419_WorldComesAlive.3dsx
Bytes:605004
SHA256:00575a2b63f86585cebfa46fc65c5c99ecc9d6f7df2187050ad6436916739cf0
SD layout:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261006-worldalive-01419/integrated-native-r2/sd_out — owner-local game data; do not redistribute.

Owner-context checks passed: detector1548 authored cases(1478 exact/70 held),90 materialized owner-stage cases(82 exact/8 held); action3 contact1152 authored(832 admitted/320 held),24 owner-stage exact,12 original pointed-sound cases; combat2 owner+10 unsigned-boundary cases14076 instructions. New transaction test256 allocation-free ticks, plus640 contact preparations and1200 metadata lookups. These are original-instruction comparisons under documented premises, not live retail parity.

Azahar2126.0 Old-model100%CPU1x boots the current binary and completes2185-frame input playback. Terrain/player and bottom companion render; area1 actor stats/profile and eight animation requests are bound. Detector reaches phase1 then holds at preparation status6 SegmentUnsupported on sampled unsupported collision geometry. Full enemy movement/combat encounter was not exercised. Sound producers are wired but sampled queue count is0; no audio playback claim. Diagnostic area/source-group replacement is not normal tutorial progression.

Genuine captures: SCREENSHOTS_01419.md. Replay-end camera obstruction and player color/material defects remain. Original top PS1 HUD is unbound. No new title-screen implementation/capture; physical Old3DS unavailable to this executor.

Emulated telemetry: peak sampled malloc_live=1937384, owned linear=71640, owned texture=4194304 bytes; peak draw calls=18, triangles=2184. These are owned subsets and emulator observations, not total physical-device use. Memory layout2 reports96MiB application region, heap reservation66482176, linear33554432, stack262144. Real-device validation is required. GUI replay included turbo and paused states; no hardware FPS/frame-time guarantee. Full ranges and original log path are in validation_01419/emulator-observation-r2.json.

Remaining blockers: unsupported original segment geometry; shared BIOS RNG seed/full consumer cadence; native action9 motion/work36E/player-heading ownership; action4 child consumer; full enemy hit/reaction/defeat, lock-on/Buster integration; unknown event operations; normal event→message→transition→unload/load progression; cutscene/audio/expression completeness; broader early-game proof; original HUD/camera/material fidelity; physical Old3DS acceptance. Unknowns stay unknown.

Primary external research is documented in research_01419/EXTERNAL_RESEARCH_01419.md. Public decompilation and DashGL references corroborate structures but do not replace original-instruction evidence. Action9 boundary research has78 authored cases/6804 instructions and no native implementation.

Final cumulative code ZIP:Legends3DS_0.0.14.19_Source.zip with standalone SHA256 sidecar. Source/docs/authored fixtures only. Raw/cooked game assets, ROM/CHD/EXE, retail models/textures/audio/video/saves, generated binaries/caches and emulator profiles/captures are excluded. Original archive and both separately named recovery ZIPs remain intact. No usage reset used.

# 0.0.14.20 — World Comes Alive continuation

This is a validated partial checkpoint. The sampled detector geometry blocker is fixed, and source-backed pursuit action9 is connected to the actual native actor loop. World Comes Alive is not finished; the first sampled actor now holds at missing shared RNG input. No completed combat encounter, normal tutorial progression or physical Old3DS test is claimed. PHYSICS_1TO1=false.

## Baseline and toolchain

Exact source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.20_WorldComesAlive`. No Git repository, branch or commit; this is an archive-derived cumulative tree. Parent: `Legends3DS_0.0.14.19_Source.zip`, SHA256 `6016361e371067b963a05bfa67a11fa5eb71de8c06a60a3aa3fea189dc6e581e`. Parent archive CRC and source manifest were verified before copying. Synced `sources/` remains read-only and untouched.

Actual native toolchain: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2; ARMv6K/mpcore/hardfloat. Original0143 fallback renderer and independently controlled enhanced gates remain available and compiled. No engine replacement, remade retail model/animation or asset recook was performed.

## Implementation and reuse

The existing map, imported actor resource/request banks, actor state transaction, contact wrappers, source angle tables, audio producer queue, area loader, safe renderer and bottom UI are reused.

`actor_detection_segment.cpp` now applies the proved rectangle/diagonal/diamond footprint rules and plain-Y branches. Distant triangular groups no longer hold before membership testing. Unsupported geometry still reports the first cell/group/primitive/code; family7 interior and slope-specific Y remain unproved. The preparation status now accurately distinguishes missing helpers from missing profiles.

New `class20_03_action9.cpp` and `actor_action9_contact.cpp` implement the proved pursuit subset. Native `main.cpp` supplies the original player-to-actor angle, separate actor-to-player turn angle and raw unsigned player heading. `class20_03_behavior.cpp` and `actor_runtime.cpp` connect turn32, forward52, wall/floor results, full four-lane rollback, source animation cue, original action boundaries and imported pose playback. Detection flags are staged before choosing movement. Structural/stale/consumer failures retain state, contact, pose and audio. Cue176 stores the post-wall/pre-floor position; it does not reuse action2's pre-motion payload.

Action4 and its32-slot child allocator were traced, not implemented. Combat byteC, pool cleanup/lifecycle and resident subtype callbacks remain explicit requirements. Read-only original-reference tools measure the selected installed OpenBIOS service; no private enemy PRNG or guessed retail seed/cadence was added.

Complete changed-file inventory: `docs/CHANGED_FILES_01420.json`. Evidence labels and limits are in `state/EVIDENCE_LEDGER.json` and the34-system `state/CURRENT_READINESS.json`.

## Commands and results

Run from the exact source above:
```sh
cmake -S . -B ../pass-20261007-worldalive-01420/host
cmake --build ../pass-20261007-worldalive-01420/host -j4
ctest --test-dir ../pass-20261007-worldalive-01420/host --output-on-failure
python3 -B tools/build_hardware_test.py --pack ../pass-20261006-worldalive-01419/pack-runtime-r1 --host-build ../pass-20261007-worldalive-01420/host --out /path/to/fresh-output
```

Inherited baseline58/58PASS; final63/63PASS, including18 RNG trace/admission/corruption cases. Host core/tests use warnings-as-errors. The first integrated run failed two tests because new authored fixtures had not yet been copied; publishing those fixtures fixed the failures. Nothing was disabled. Original-instruction proofs include11113 exact supported detector queries,577 explicit holds and294/294 local owner-stage queries;2496 pursuit reducer cases;216 supported authored contact cases plus72 held and24/24 owner-stage contacts. New runtime tests cover24 stale/payload mutations,12 dependency holds, sound ordering, pose commit, rollback and384 allocation-free ticks. Separate detector1536 and pursuit640-per-function allocation checks pass. These authored comparisons are not live retail parity.

Final native build r2PASS: 611684 bytes, SHA256 `1a95de3b4b32af67f31a31fe81e79a675e41e0ca9bd4b42f13a94d9d9955839a`. Zero native warnings/errors; all182 compiled source hashes match current source. Build output: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01420/integrated-native-r2/build/Legends3DS_01420_WorldComesAlive.3dsx`. SD layout: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01420/integrated-native-r2/sd_out`. These local outputs contain owner-local game data and are excluded from the source ZIP. Existing pack113 outputs were verified and reused unchanged; manifestSHA `ecbb7eaa322e481168ac85e291a000ad3daa514e2686b6b14e23e8dd5e0cb306`.

Desktop validation covers the portable host core/tools/tests. No desktop graphical game run or complete desktop gameplay claim is made.

## Native and original-reference observations

Azahar2126.0, isolated Old-model profile,100%CPU and1x resolution: final r2 recorded input test reached sampled gameplay frame600 and loaded the imported actor group. All three detector preparations areReady; source16 reachesNeedsRandomSample, and source17/18 remainaction0phase2. HP64/profile/eight request banks are bound. Pursuit is connected and host-exercised but not reached in this native encounter. No normal progression or full AI/combat acceptance.

The Mac was initially locked; after unlock, a genuine capture was saved from the final build. The image shows obstructing terrain in front of the player and the bottom companion UI. This is evidence of an unresolved camera/placement problem, not a successful player close-up: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01420/azahar-01420-r2/captures/Azahar2126_OldModel_01420_r2_area1_continued.png`. Capture details and all original attempts are in `docs/SCREENSHOTS_01420.md`. No hardware screenshots exist.

The original PS1 reference used the verified local PCSX-Redux Reference app, interpreter mode, selected installed OpenBIOS and both memory cards disabled. Its read-only observer measured guarded startup state0, but no enemy RNG sample is yet accepted. The first process crashed after a status request; the C++ cause remainsUNKNOWN. A fresh file-only observer runs at `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01420/rng-reference/live-r3/observer.lua` with HTTP disabled. The user reached original tutorial gameplay after unlock. The file-only trace then contains20 original RNGenter/return pairs but stops at tutorial_callback_identity; no class03 sample is accepted. The trace remains a failed evidence artifact with unknown encounter ownership. A corrected admission observer is prepared under live-r4 for the next capture; it has not been installed in the active reference. Original saves were not accessed or modified.

## Performance, memory and remaining work

Frozen final r2 logs:48 sparse samples,37 distinct gameplay counters through600. Sampled maxima:18 draw calls,2184 triangles,1937384B live malloc,4194304B owned textures,71640B owned geometry/linear storage. The transition-associated frame sample reached2280021us. Reservations:100663296B application region,66478080B heap,33554432B linear,262144B stack. These are emulated reservations and distinct diagnostic pools, not a hardware memory certification; do not sum them blindly. Sparse synchronized samples do not establish FPS or worst-case frame pacing. See `docs/validation_01420/native-observation-r2.json`.

Known blockers: shared RNG chronology/sample ownership; action4/10 attack-child behavior and lifecycle; packet delivery/finalization, damage/reaction/defeat; family7/slopes and unresolved player contacts; normal trigger/event/message/transition/unload/load progression; original camera/top HUD/material fidelity; audio playback/cutscenes/save semantics and physical device testing. Area1 player contact isUnsupported in two frozen samples andNoData in one. Bottom UI remains provisional. The runtime is a technical prototype with player movement and imported content, not a completed faithful playable tutorial or full port.

## Deliverables and exclusions

New complete cumulative code-only ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.20_Source_r1.zip`; adjacent `.sha256` gives its checksum. The independently named recovery ZIP is retained. Release verification checks ZIP CRC, every source-manifest hash, expected files and prohibited extensions. Source, authored scalar fixtures, project/build scripts, documentation and evidence metadata are included. Raw/cooked retail payloads, ROM/CHD/BIN/CUE/EXE, textures/models/audio/video, retail saves, `.3dsx`/ELF/SMDH, emulator profiles, original captures/movies, reproducible caches and generated build output are excluded and preserved locally where needed. No earlier archive is overwritten and no usage reset was used.

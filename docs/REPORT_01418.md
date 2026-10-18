# 0.0.14.18 — World Comes Alive continuation

This cumulative checkpoint connects imported actor pose requests, retained source behavior fields and bounded original map contacts to the native runtime. It also fixes the player contact wrapper incorrectly undoing successful movement. **World Comes Alive is unfinished. This is a technical prototype with partial playable movement, not a completed faithful tutorial or complete enemy/combat/progression port. PHYSICS_1TO1=false.**

## Source and toolchain

Exact project: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.18_WorldComesAlive`. No Git repository, active branch or commit exists for this archive-derived source. Continue from `Legends3DS_0.0.14.17_Source_r1.zip`, SHA-256 `b12241d9e667ebd54c7ab824f9b6158fcb8e22cbf7cb3e0e102ce0900dc93cd7`; 875 inherited manifest entries were verified before editing. Parent source and earlier archives are preserved. The original 0.0.14.3 renderer foundation, feature gates, generalized package/map/session architecture and genuine owner-local meshes, materials and pose streams are reused.

Native tools: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1 and Citro3D1.7.1-2. Compiler version was checked this pass; package-version records were inherited from the installed toolchain audit. Target flags are `-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft`, with a 262144-byte stack reservation. No New-3DS CPU dependency was introduced. The unchanged safe shader SHA-256 is `fd242f3023a04cb03ba80ee20ea716daff2f9921f54502f5eeefa1d3a2b373bb`.

## What changed

- One immutable animation bank per original EBD resource row now holds genuine request IDs0/1/2/4/6/21/22, decoded from each request's own original pointer/pose table. No request0 substitution or rebuilt stand-in animation is used. Banks share across actors; resets and ticks allocate no memory. Unknown requests hold state and pose.
- Imported actor instances retain the source-used raw/fixed coordinate, angle, parameter, context, prior-work and contact fields. Source-used state1 fields and prepared action2 movement/heading/contact/pose commit together after validation. Missing bank/input, stale movement preparation, unsupported class or nonzero hit packet preserves the active transaction. Legacy request0 and new bank playback cannot both own an actor.
- The class20/03 source reducer covers scalar initializer/callback ordering, state1 action0/1 branches, prepared action2, signed timer/selector tails and bounded hit branches. External BIOS RNG, admitted detection geometry, effects, proxies, sound and attack/defeat ownership stay explicit. It is not complete native AI.
- Original flag predicate `0x8001DA58` is recovered and bound to `RuntimeSession::story_flag` for semantic IDs0..2047, including ID0. It uses the imported object's tail field. IDs outside that domain remain unknown. Flag meaning, creation and timing are not inferred; action3 after wake-up remains unported.
- New typed actor wall/floor contact queries use at most four candidate cells, bounded convergence and source-proved rectangle/packed-cell handling. Forward action2 preparation uses cooked original trig data, fixed-point source rounding and actual map queries. Unknown intersecting record families, slopes, deep overlap, invalid ranges and exhausted bounds report unsupported/failure.
- Original source comparisons exposed a real player bug: contact result0 must preserve work. `player_collision.c` now returns unchanged on zero; negative-result rollback remains. The inherited actor slope scratch contract returned the wrong sign/order, so actor slopes8..11 now report Unsupported. The shared player slope scratch contract still needs repair.

Main changes: `actor_animation_bank`, `actor_contact`, `actor_runtime`, `actor_sequence`, `class20_03_behavior`, shared player collision, native `main.cpp`/hardware diagnostics, CMake/Makefile, regression fixtures and source research probes. The complete cumulative changed-file inventory is [CHANGED_FILES_01418.json](CHANGED_FILES_01418.json). No original assets or original executable bytes were added to the source tree.

## Native wiring and observed behavior

Default-off owner diagnostic `actor_state_01418.cfg` with `ACTOR_STATE_FIELDS=1` enables one shared seven-request bank and supported state1 processing. Its zero hit packet is explicitly an authored diagnostic premise; BIOS RNG and admitted detection are never invented. The original request0 animation gate is retained separately. Enhanced rendering is required for this diagnostic; the independently selectable 0143 fallback remains preserved and compiled. Current fallback runtime testing was not performed; historical 01416 fallback evidence is not current acceptance.

The exact current binary executed in **Azahar2126.0 configured as Old model, CPU100%, internal resolution1x**, using an authored ground/jump input movie. Native boot logs reach runtime-ready and record994 original map initialization writes, package/map/render/reference/flow/message/EBD bindings and genuine player resources. One seven-request actor bank initializes when conditional actors spawn and is released on diagnostic area change. Three temporary class03 action2 queries return wall0/floor-1 with actual map/trig data; active actors are explicitly unchanged by these probes.

All123 sampled active actor state rows return `NeedsDetection` and retain action0/phase0/request0/unknownHP. This is evidence of the remaining integration blocker, not moving enemy AI. The logs record13 distinct player positions, source action IDs0/2/4/7/8 and grounded values-1/0/1. Diagnostic SELECT changes area1→2 and reloads resources; this is not normal trigger/dialogue/event progression. Projectile presentation remains an inherited candidate proxy. No current player/enemy visibility or visual-fidelity acceptance is claimed.

Physical Old3DS/XL: **HARDWARE_PENDING**, no current build tested. Desktop: host core, import/package tools, owner probes and tests build and pass; no desktop gameplay-renderer acceptance was performed. The port target remains native Old3DS, not PS1 emulation.

## Builds and tests

| Check | Actual result | Limit |
|---|---|---|
| Inherited host suite before edits | 42/42 PASS | Host tests only |
| Current complete host suite | 46/46 PASS | Includes final2309-case fixture; warnings treated as errors where configured |
| Source callback/state transaction comparison | 2309 cases ×65 state fields;289285 original instructions | Authored external-helper premises; not live retail parity |
| Original flag predicate/caller comparison | 8200 helper +4096 no-hook caller cases;297064 instructions | Authored flag/work contexts; no event/flag timing proof |
| Actor contact comparison | 176 wrappers;2190 supported walls,642 corrections;2268 floors;18 actual materialized stage queries | 546 unsupported cases classified; 4284566 original instructions; excluded slope/unknown families |
| Genuine owner request-bank probe | Seven distinct request streams;960 ticks/8 resets;no tick allocation | Host element bytes45650; no cadence/hardware proof |
| Owner runtime integration | 1440 state1 ticks;request1 reset/advance;packet hold;unload;PASS | Explicit host premises, not encounter AI |
| Complete build/package/deploy pipeline | PASS;109 existing owner-pack files validated | Cookers/extractors not invoked; local package assembly only |
| Actual native ARM build | PASS;0 warnings,0 errors;160 source hashes match current code | Hardware pending |
| Old-model Azahar runtime | Current boot/state/contact/movement/resource telemetry observed | Authored replay, no current screenshots or visual acceptance |

Actual binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261006-worldalive-01418/install-r2/build/Legends3DS_01418_WorldComesAlive.3dsx`, **585152 bytes**, SHA-256 **`0898e6d048abd5390beb8e4757583bc157b2026979e86d7c885183004344ec6b`**. Owner-local SD layout: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261006-worldalive-01418/install-r2/sd_out/3ds/Legends3DS`. Source-only ZIP excludes this generated binary and all SD game payloads. The `.3dsx` is also copied separately into deliverables for convenience.

Canonical full build command, run successfully from the project mirror:

```sh
python3 -B work/Legends3DS_0.0.14.18_WorldComesAlive/tools/build_hardware_test.py --pack work/pass-20261005-worldalive-01417/pack-runtime --out work/pass-20261006-worldalive-01418/install-r2
```

Exact child command argument lists, return codes and log paths are recorded in [build-result-r2.json](validation_01418/build-result-r2.json). The pipeline runs host CMake configure/build, CTest, cooked-area package validation/probes and actual native Make. For a repeat build, choose a new output directory; preserve install-r2 and all prior checkpoints.

Additional exact test/probe commands executed this pass:

```sh
cmake -S work/Legends3DS_0.0.14.18_WorldComesAlive -B work/pass-20261006-worldalive-01418/host -DCMAKE_BUILD_TYPE=Release
cmake --build work/pass-20261006-worldalive-01418/host -j4
ctest --test-dir work/pass-20261006-worldalive-01418/host --output-on-failure -j4
ctest --test-dir work/pass-20261006-worldalive-01418/install-r2/host --output-on-failure
work/pass-20261006-worldalive-01418/host/l3actor_animation_bank_probe work/pass-20261005-worldalive-01417/pack-runtime/ebd/area_01.l3ebd
work/pass-20261006-worldalive-01418/host/l3actor_behavior_runtime_probe work/pass-20261005-worldalive-01417/pack-runtime/ebd/area_01.l3ebd
work/pass-20261006-worldalive-01418/host/l3test_actor_contact work/pass-20261006-worldalive-01418/actor-contact/owner-contact-source-expanded work/pass-20261005-worldalive-01417/pack-runtime
```

Full final host output is [r2-host-tests.log](validation_01418/r2-host-tests.log); actual native output is [r2-native-build.log](validation_01418/r2-native-build.log). Original-instruction probes and their supported owner-image hashes are in `docs/research_01418/`; authored scalar regression fixtures are included so tests need no retail inputs. Source differential cases do not establish complete 1:1 behavior.

Failures/findings were preserved and fixed rather than hidden: zero-result player rollback, incomplete phase0B clearing in an older transition, stale action2 preparation and conflicting legacy/new pose cursors now have regression checks. The slope test now asserts explicit Unsupported because original full-wrapper evidence disproved its prior expected height. An initial non-detached Azahar process exited without current telemetry; a separate detached r2 run produced the current evidence. Build-r1 is preserved but superseded by r2, which includes the recovered story-predicate native connection.

## Memory and performance observations

These are40 sparse HLE samples from the authored replay, not physical Old3DS frame rates or complete process peaks:

| Metric | Observed range |
|---|---:|
| Draw calls | 15–18 |
| Submitted triangles | 1710–2185 |
| Update time | 2–60585µs, including loading work |
| Frame time before logging | 9787–2244095µs, including diagnostic load spike |
| Owned geometry linear memory | 71640–82560 bytes |
| Owned texture memory | 3932160–4194304 bytes |
| Sampled allocator live bytes | 1886712–1935424 bytes |
| Linear free | 27412992–27664128 bytes |
| VRAM free | 5523456 bytes |

Diagnostic area1→2 load took2222722µs; the approximately2.24s frame spike remains a problem. Native request bank uses41566 decoded element bytes and44618 total actor decoded element bytes. This excludes allocator overhead, resident poses, geometry/texture allocations elsewhere and total CPU/GPU usage. Load-time bank cap is524288 decoded element bytes; original class20 pool limit16 is respected. Host tick/reset/contact paths have allocation guards; these do not prove all render/update code allocates nothing.

**Memory-mode risk:** telemetry reports `new3ds=0`, `memory_layout=2`, a100663296-byte (**96MiB**) application region,66502656-byte main heap reservation,33554432-byte linear reservation and262144-byte stack reservation. Old-model emulator configuration does not verify the physical Old3DS homebrew loader's available memory mode. Total process residency, peaks and real device compatibility must be measured on hardware. Reservations must not be confused with sampled live memory.

The original widescreen400×240 projection and source-derived camera baseline are retained. Extra scenery at the sides is the user's chosen framing. Original PS1 top-screen HUD, full camera dynamics/occlusion and bottom-screen native touch acceptance remain unfinished; this pass makes no UI fidelity claim.

## Screenshots and remaining blockers

No current screenshots were captured: the computer-use tool reports the Mac is locked, and manual unlock was requested. [SCREENSHOTS_01418.md](SCREENSHOTS_01418.md) records the missing captures and genuine log paths. Prior screenshots belong to earlier builds and are not presented as current evidence. Current runtime logs/CSV are included in `docs/validation_01418/`; visual inspection remains pending.

The next engineering blocker is original admitted player-detection geometry/ownership, followed by BIOS RNG seed/call order and action3+ lifecycle. Until these are proved and wired, ordinary class03 enemy state remains held. Lock-on acquisition/retention, Buster hit delivery, actual HP selection/damage/reaction/defeat, sound/effect/proxy consumers and progression consequences are incomplete. Shared player slopes/steps/ledges/contact ordering and faithful camera/animation cadence still require source/live comparisons. Full EBD/event execution, trigger→message→natural transition→unload/load, original cutscenes/expressions/audio and broader early-game progression remain incomplete. Presentation replacement failure can retain stale visible resources while logical session groups differ; inherited diagnostics report it, but full rollback is not complete.

Current readiness for all34 requested systems is [CURRENT_READINESS.json](../state/CURRENT_READINESS.json); machine-readable source evidence and limitations are in [EVIDENCE_LEDGER.json](../state/EVIDENCE_LEDGER.json). No made-up completion percentage, full-game claim or hardware result is used.

## Checkpoint and recovery

Complete new code-only ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.18_Source.zip`. Its actual SHA-256 is in the adjacent `.zip.sha256` file; independent ZIP CRC/inventory/manifest verification is recorded outside the ZIP in this pass directory. The archive includes cumulative source, project files, build scripts, authored fixtures and source-safe documentation/diagnostics. It excludes only generated output/caches and all prohibited retail/raw/cooked assets: ROM/CHD/BIN/CUE/executables/saves, models/textures/audio/video and cooked packages. `SOURCE_MANIFEST.sha256` inventories every included file except itself. No original input or older checkpoint is replaced.

Verified separately named recovery ZIP was saved before usage became tight: `Legends3DS_0.0.14.18_RecoverySource_20261006_001.zip`, SHA-256 `9c37e3f7c283b95e421eae7f3ecee2793a8d5e618d45701947b8b9080d8bc1ae`. It predates the final native story-predicate connection; the final source is cumulative and newer. [RESUME_01418.md](RESUME_01418.md) gives exact paths, completed/pending work and next concrete steps. No usage reset was redeemed.

# 0.0.14.21 — partial World Comes Alive checkpoint

The native imported-actor path now consumes the original Action4 attack body/contact, pose requests21/22 and cue177 producer. This is substantial connected code, but World Comes Alive is **unfinished**. A complete attack, Buster→enemy hit, damage/reaction/defeat and normal tutorial event progression are not demonstrated. `PHYSICS_1TO1=false`.

## Project and reuse

Exact project: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.21_WorldComesAlive`. No Git repository, branch or commit exists. Parent `Legends3DS_0.0.14.20_Source_r1.zip`, SHA256 `579994e81a68e0040c1eb8f44e310d1e570ed610d5c9ce543c6ed184e8a630b9`, CRC and1081 manifest entries verified before copying. Synced `sources/` and parent remain unchanged. Shared actor/session/renderer/package/collision/animation/audio/save architecture is cumulative; no replacement engine. The0143 safe path and feature gates are preserved, compiled, but not newly exercised this pass.

## Research before coding

Pinned public MML1x10 commit `67c8f34fa94ada6ea225e31b36d841f10067fb6a` is incomplete, PS1/R3000/PsyQ-oriented and not an ARM/3DS replacement engine. Sample player/scene/sound wrappers still use41/59/34 active INCLUDE_ASM entries. Tutorial actor code is assembly/data. No software license identified; no external source implementation/assets copied. Useful symbol navigation is documented in [assessment](research_01421/DECOMP_ASSESSMENT_01421.md) and [source records](research_01421/RESEARCH.json). Primary [README](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/README.md).

Hash-guarded owner original-instruction differentials prove1568 Action4 body cases,135 full-slot child initializer cases,32 release cases (140405 instructions), and430 matching admitted contact cases/56 holds/4 corrections (1078288 instructions). These authored premises do not establish a live retail encounter.

Two inherited research mistakes are corrected: random call return is801022F8, later sample store801022FC; subtype<128 child dispatch uses scratch1F800060, not resident80089820. Stage initialization sets table8011A5D4: subtype6→8010F354, subtype9→null. Unrelated prior resident callback names are withdrawn. Unknown consumers stay unknown.

## Implementation

- `class20_03_action4` and `actor_action4_contact`: source timer/motion/rollback, request21/22, entry cue177 and recovery byteC. Signed16 timer wrap and four-lane pointed words retained.
- `actor_runtime`/behavior/native main: atomic shared body/contact/pose/audio bridge,26 stale/payload guards, source statistics from load-time binding, actor variant0 only. Caller-fabricated child readiness cleared. Spawn boundary deliberately holds `NeedsChildConsumer` until actual child callbacks are implemented.
- L3CBv2 keeps12-byte records, unsigned byteC plus presence flag. V1 exact roundtrip retained with byteC UNKNOWN; malformed/unsupported encoding rejected. Fresh113-output owner pack changes only56-byte `actor_combat.l3cb`;112 outputs identical. Requests21/22 already retained; no model/animation remake or broad asset recook.
- RNG validator fixes actual return address, order and captured context validation (27 cases). Strict bounded immutable tape API rejects mismatched input/update/consumer history, stale tickets, competing consumers and exhaustion. It is **host replay infrastructure**, not live native RNG binding.
- Hardware diagnostic actor copies test attack admission and child hold without changing active actors, source RNG or gameplay queue. Final revision r3 fixes console-only probe output to full CSV logging.

Current r3 area1 authored copy probes: all three actual imported actors commit phase1/timer12/request21 with one cue177 queue entry and available byteC. All three child-boundary probes reject caller-forged child readiness with no commit. Active actors remain unchanged; this is admission/rollback evidence, not live enemy attack or retail parity.

## Build and tests

Toolchain: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2; `-march=armv6k -mtune=mpcore` hardfloat. Old3DS is target. No New3DS fast clock requested.

Inherited baseline63/63 PASS. Final integrated67/67 PASS. Zero host/native compiler warnings; meaningful new C++ tests compile with warnings-as-errors. Cook metadata7/7 and RNG validator27/27 PASS within inherited/integrated suites. New authored source bridge512 ticks, body/child/contact640 iterations, RNG tape4096 held peeks allocate no heap. Host core/probes are built and exercised; a desktop graphical game build was not demonstrated.

Exact final command from source directory:
```sh
python3 -B tools/build_hardware_test.py --pack ../pass-20261007-worldalive-01421/pack-runtime-r1 --host-build ../pass-20261007-worldalive-01421/host --out ../pass-20261007-worldalive-01421/integrated-native-r3
```
The pipeline ran CMake Release configure/build, `ctest --output-on-failure`, package validation/runtime/inspection probes and actual native `make -j4`, all return0. Full argv/results: [build record](validation_01421/native-build-r3.json); logs beside it. All188 compiled input hashes match current source. Native binary `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01421/integrated-native-r3/build/Legends3DS_01421_WorldComesAlive.3dsx` is621024bytes, SHA256 `5b34a79e6d8dce2f30ccecf41ff6847943dbf2fa78e51d0eda86d5084980e602`. Local SD layout `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01421/integrated-native-r3/sd_out/3ds/Legends3DS/` contains owner data and is excluded from release.

Initial integration failures were diagnosed rather than suppressed: missing new helper closing brace repaired; an inherited assertion expecting Action4 UnknownAction updated to verify its new explicit sound/motion holds. r2 CSV lacked Action4 records because a console formatting helper did not write the file; r3 corrects that. Exact intermediate build logs remain in `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01421`.

## Emulator, memory and performance

Current-pass Azahar2126.0 Old-model (`new3ds=0`),100% CPU,1x recorded-input run revision `r3`. Playback exercises ground movement, jump/fall/land, a provisional projectile, SELECT diagnostic area0→1 and three real imported actor constructions. This is diagnostic progression, not normal tutorial progression. Live source16 holds NeedsRandomSample; other sampled actors remain waiting. No full callback/attack/retail parity claim.

Last sampled frame600: 18 draws/2184 triangles; geometry-owned linear71640B; owned GPU texture4194304B; malloc live1937400B; linear free27412992B. Area0 package load1.36s and diagnostic replacement2.31s in r2; texture residency rises2883584→4194304B, malloc live1683136→1937400B. These emulator observations are not physical loading time or hardware performance. Sparse timings exclude log write/flush, render includes synchronization, initial frame/load spikes are included; no FPS/worst-case claim. Actual peaks, per-frame GPU allocation, stack high-water and hardware residency are unmeasured. Emulated region96MiB with heap66465792/linear33554432/stack262144 reservation needs physical validation; reservation is not live consumption. [Observation record](validation_01421/native-observation.json).

## Reference and captures

Original PS1 reference uses isolated profile, official installed OpenBIOS and read-only observer with memory cards disabled. Current frozen trace has4 rows and no admitted enemy RNG. User reports START cannot start gameplay; saved Port1 Keyboard/Digital and Return=START/X=Cross are correct. Cause remains unknown. Latest UI inventory explicitly reports Mac locked. No fabricated input, seed, pending return or imported pose is substituted. Original reference/profile/trace remain local. [Status](validation_01421/ps1-reference-status.json).

No new screenshot files captured this pass yet. GUI service earlier timed out and now reports locked Mac; native CSV is execution evidence, not visual acceptance. Existing captures remain historical and are not labelled01421. [Capture status](SCREENSHOTS_01421.md). Physical Old3DS not tested; cancelled remote hardware setup was not resumed.

## Remaining blockers and next concrete work

1. Restore original PS1 gameplay input and capture a whole source-verified global RNG timeline plus independently measured input/update and callback-consumer schedule. Draw chronology cannot be replaced by one random value when native state needs it.
2. Decode actual stage subtype6callback8010F354 lifecycle, hit delivery/HP/reaction/defeat and pool release before wiring child production. Proven scalar allocator alone is insufficient.
3. Continue original callback/packet combat, lock-on and encounter proof; then proven trigger→event→message→normal area transition with explicit unload/load. Unknown EBD operations, save fields and behavior constants remain UNKNOWN.
4. Camera obstruction, top PS1 HUD, complete slopes/steps/ledges, audio playback, cutscenes/expressions and physical Old3DS performance remain open. Bottom companion behavior is cumulative; no unrelated UI changes this pass.

## Deliverables and exclusions

Cumulative code-only source archive: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.21_Source.zip`. Separately named recovery: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.21_Recovery_20261007_Source.zip`. SHA sidecars and external archive verification records avoid self-referential hashes. Source includes project/build scripts/docs/all authored tests and evidence. Retail inputs/cooked payloads, owner saves, capture media, emulator profiles/replays, generated binaries/build output and reproducible caches are excluded. Code-only release contains no packaged retail model/texture/audio/video. Original assets continue to be imported locally, not rebuilt from scratch. See [changed files](CHANGED_FILES_01421.json), [source manifest](../SOURCE_MANIFEST.sha256), [readiness](../state/CURRENT_READINESS.json) and [resume](RESUME_01421.md).

Assessment: native technical prototype, not a completed playable faithful port. No usage reset redeemed.

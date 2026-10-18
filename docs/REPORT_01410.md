# 0.0.14.10 terrain placement report

This is a cumulative technical prototype. Positive decoded area data now drives the native enhanced terrain path through shared APIs. It remains an incomplete port: the full tutorial, natural progression, retail physics and physical Old 3DS acceptance are not established.

## Source and toolchain

Exact source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.10_TerrainPlacement`. Complete parent 0.0.14.9 source preserved; 421 parent manifest identities passed before editing. No Git repository exists, so branch, commit and Git status are unavailable. Changed files are measured by hashes in `docs/CHANGED_FILES_01410.json`. Synced `sources/`, owner data, input archives and previous releases were not edited.

Native: devkitARM r68-1 / GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2, actual ARMv6K/mpcore target. Host: AppleClang 21 / CMake 4.4.2 / Python 3.9.6 on macOS. No desktop game frontend exists; host tests exercise portable code and tools.

## Changes and reused systems

Positive grid references now mark unique IDX identities. The area collector emits them once in ascending identity order, using authored XYZ and the whole-field raw6 bank rule. The decoder keeps X/Z signed and subtracts unsigned Y bytes; it preserves original vertex order, UVs, material words and prefix metadata. Global origin/local scaling is represented, while the current runtime uses the observed reset value 0.

`area_placement` bounds its temporary bitmap to 4,096 B (125 B for supplied ST00), validates ranges and counts primitives before VBO allocation. It runs at area loading, adding no per-frame allocation. `positive_terrain` is a separate decoder. The native enhanced adapter and one-command installer use the shared implementation and actual boot-package probe. The absent-descriptor sentinel is skipped consistently. Both new suites run with warnings as errors and checks active in Release.

Reused: generalized package loader, L3MP/L3RG retained bytes, area-local resource lifetime, PS1 VRAM load order/CLUT/material identity, existing controls/player animation/replay/save scaffolding and exact 0.0.14.3 fallback. The legacy cell loop, shaders, fallback draw/VBO/attribute functions and underlying helpers are preserved. Existing cooked assets and caches were reused; no data was recooked or bundled in the source release.

Research inspected the locally supplied executable using static control flow plus bounded synthetic coordinate probes. Executable hash `10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`; anchors, block hashes, methods and limits are in `validation_01410/placement-source-evidence.json`. The portable reproducer emits no game bytes or disassembly. Four positive blocks passed 128 cases and two negative blocks passed 256 cases. This proves selected decode fields over those fixtures, not GTE projection, complete rendering or retail gameplay parity.

## Build and checks

Executed from the source path above:

```sh
./build_mml3ds.sh '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted' --out '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261002-placement/install-final' --cache '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/cache'
```

Result PASS; all 15 CTest suites pass. Full exact argv, return codes and log paths are in `validation_01410/build-result.json`. The inherited parent baseline 13/13 passed before edits. Both new suites also pass AddressSanitizer and UBSan; leak detection was disabled on macOS. The initial sanitizer harness omitted the CRC provider, failed to link, then passed after adding `l3map.cpp`; original diagnostics are retained outside source. An early aggregate probe expected historical signed-Y addition; its expectation was corrected against executable evidence. No failing assertions were silenced.

The installed generalized package set validates 205/205: 31 geometry identities, 4,115 graph nodes, 3,916 edges. Cache: 339 hits / 0 misses; 100 tutorial files retain manifest `58d4b63e82ef33001f67222cfbfc86a0f772d7117d37ff69bcf104403dbd783a`. Nine host scenarios repeat using the native algorithm; they are not retail parity. Full install wall time 74.841991 s is a host measurement, not device loading time.

Actual final native binary 412,332 B; SHA-256 `7a4d4d426aa89a9649d4903df6ab1f2851f55fe83b466513dd5cc48ea642ec7e`. All 123 native snapshot files match final source. Build output `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261002-placement/install-final/build/Legends3DS.3dsx`; local owner-data SD layout `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261002-placement/install-final/sd_out/3ds/Legends3DS/`. It contains 103 files / 4,239,598 B and must stay local. Code-only standalone binary and source ZIP are listed in the handoff section.

## Area and memory observations

| Diagnostic area | Unique IDX | Legacy primitives | Current primitives | Negative cells pending | Current terrain VBO bytes |
|---|---:|---:|---:|---:|---:|
| 0 | 18 | 1752 | 488 | 8 | 58560 |
| 1 | 126 | 854 | 526 | 71 | 63120 |
| 2 | 202 | 674 | 674 | 14 | 80880 |
| 3 | 24 | 120 | 120 | 16 | 14400 |
| 4 | 624 | 12112 | 3028 | 0 | 363360 |

Current Old-model emulator telemetry matches these VBO allocations. Area 0 uses 58,560 B versus 210,240 B in the prior UV path. Area 4 calculates a 1,090,080 B reduction from avoiding repeated identity draws. This follows original visibility semantics, without changing gameplay rules. Recorded texture residency ranges from 1,048,576 to 3,145,728 B in the five-area run; malloc_live samples range from 630,552 to 1,159,848 B. Textures and VBOs use GPU-accessible linear memory; these counters must not be mistaken for total process or VRAM peaks.

The current ARM reader reports package size of 120,774 B, decoded admission estimate of 172,008 B and load-peak estimate of 340,656 B. The emulator reports new3ds=0 but a Dev1 96 MiB application region, heap reservation of 66,678,784 B, linear reservation of 33,554,432 B and stack reservation of 262,144 B. Those reservations are not measured live heap use or proof of real Old 3DS limits. Sparse frame/update/render samples and transition allocator events are retained; synchronization and logging affect them. No hardware performance, worst-case frame pacing, true total heap/stack peak or device loading claim is made.

## Emulator, screenshots and limits

Azahar 2126.0 was exercised with New3DS=false, CPU 100%, resolution 1x and isolated NAND/SD/config/data/cache. Synthetic controller movies select the renderer, move, turn, fire and switch diagnostic areas; screenshots are captured through Azahar's own Capture Screenshot command. A macOS direct-launch notice was acknowledged. Four unchanged PNG captures were saved: area 0 textured view, area 4 empty diagnostic view, 0.0.14.3 fallback and textures-off. Title and action captures remain pending because the Mac locked. An initial detached fallback launcher ended before testing; the attached retry was used. The Old-model setting and native ARMv6K build cannot establish physical Old 3DS performance.

All five diagnostic areas load through the current positive placement path with expected counts. Area 0 shows textured walls and player, with incomplete floors/grounding. Area 4's default diagnostic view shows the player but no terrain, despite admitted/submitted geometry; original spawn/camera behavior is not bound. The full scene is not visually accepted. The 0.0.14.3 fallback and textures-off gates both booted and rendered in the current build. Controls telemetry records 11 distinct sampled positions, changing headings, firing input with 1 active projectile and an area 0 → 1 transition; its final UI/capture was blocked by the Mac locking. After preserving telemetry, the isolated controls process (PID 23921) was stopped using SIGTERM, then SIGKILL, after its command exactly matched the launch argv; the PID was confirmed absent. Process cleanup does not verify final UI or playback completion. Original captures, build/device labels and paths are recorded in `validation_01410/emulator-evidence.json` and `docs/SCREENSHOTS_01410.md`. Screenshots are separate local artifacts, excluded from the code-only ZIP. No physical device captures exist.

## Blockers and unported behavior

There are 109 negative cells across the first four test areas. Research establishes their grid center, encoded flat Y, packed quarter-turns and default-STG descriptor branch; native geometry/material references and override/palette lifetime are not bound. They remain counted and omitted. No unknown palette or material was substituted. Three inherited unresolved CLUT cases remain.

Positive depth thresholds and multiple original dispatch families are documented, but the frontend still uses Mid LOD with all area positives admitted; original dynamic visibility, subdivision, alpha/culling and camera behavior are incomplete. Checked wide native coordinates do not establish original halfword wrap parity outside the measured/current domain. Diagnostic room switching is not natural tutorial progression. Existing map/contact helpers still use legacy cell conventions; a retail contact solver, floor snapping, jumping, ledges and height-changing movement remain unbound.

PHYSICS_1TO1=false. Lock-on/Buster/combat/enemies, broader early-game semantic bindings, EBD, original dialogue/transitions/cutscenes/expressions/audio and save parity remain incomplete. Generalized native binding still covers map/render with exact tutorial sidecars; semantic actor/event/audio/progression sections are unbound. Direct disc-image intake and Windows/Linux host installation remain outside this milestone. No full-game or first-Tron completion claim is made.

## Readiness

| System | Evidence status |
|---|---|
| ANIMATION | SOURCE_BACKED_PARTIAL |
| AUDIO | SOURCE_BACKED_PARTIAL |
| BOOT | SOURCE_BACKED_PARTIAL |
| BOTTOM SCREEN | SOURCE_BACKED_PARTIAL |
| BUSTER | CANDIDATE |
| CAMERA | CANDIDATE |
| COLLISION | SOURCE_BACKED_PARTIAL |
| COMBAT | CANDIDATE |
| CONTENT GRAPH | SOURCE_BACKED_PARTIAL |
| CUTSCENES | CANDIDATE |
| DIALOGUE | SOURCE_BACKED_PARTIAL |
| EARLY-GAME IMPORT | SOURCE_BACKED_PARTIAL |
| EBD / EVENTS | SOURCE_BACKED_PARTIAL |
| ENEMIES | CANDIDATE |
| EXPRESSIONS | UNKNOWN |
| FRAME PACING | HARDWARE_PENDING |
| GENERALIZED IMPORTER | SOURCE_BACKED_PARTIAL |
| GENERALIZED RUNTIME LOADER | SOURCE_BACKED_PARTIAL |
| LOADING | SOURCE_BACKED_PARTIAL |
| LOCK-ON | CANDIDATE |
| MEGA MAN MATERIALS | SOURCE_BACKED_PARTIAL |
| MEGA MAN MODEL | SOURCE_BACKED_PARTIAL |
| MEMORY | SOURCE_BACKED_PARTIAL |
| MOVEMENT | SOURCE_BACKED_PARTIAL |
| OBJECTS | SOURCE_BACKED_PARTIAL |
| OLD-3DS BUILD | MATCHES_MEASURED_FIELDS |
| OLD-3DS HARDWARE | HARDWARE_PENDING |
| PHYSICS | UNKNOWN |
| QoL | CANDIDATE |
| RENDERER | SOURCE_BACKED_PARTIAL |
| SAVE | SOURCE_BACKED_PARTIAL |
| TERRAIN GEOMETRY | SOURCE_BACKED_PARTIAL |
| TERRAIN TEXTURES | SOURCE_BACKED_PARTIAL |
| TRANSITIONS | CANDIDATE |

## Handoff and exclusions

Complete cumulative code-only source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.10_Source_20261002_001.zip`. Its SHA-256 is in the adjacent `.zip.sha256`; archive membership/CRC/hashes are verified after publication. Standalone compiled binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01410_TerrainPlacement_20261002_001.3dsx`.

Earlier recovery ZIP is separately named and preserved. Exclusions: raw ROM/CHD/BIN/CUE/executable/XA/STR/retail saves, Capcom models/textures/audio/video/dialogue payloads and all cooked packages; generated native/host builds, caches/bytecode; screenshot images. No original input archive was overwritten. Source includes cumulative project files/build scripts/tests/docs and hash/address/aggregate evidence only.

Next concrete step: bind negative default-STG render/material data through a neutral package representation, verify authored spawn/camera activation and contact origins, then compare stationary and moving tutorial scenes. Physical Old 3DS validation remains required. Continue from this coherent checkpoint, preserving the independent 0.0.14.3 fallback and all earlier releases.

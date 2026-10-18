# 0.0.14.9 local data installer report

The local installation path now connects supplied game data to validated neutral
packages and the actual native SD layout. This is a cumulative technical
prototype/installer milestone. It is not a complete tutorial, full game, 1.0 port
or measured retail-parity result.

## Source and toolchain

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.9_LocalImporter`. Parent: complete 0.0.14.8 checkpoint; historical 0.0.14.6c foundation.
No Git repository is present, so branch/commit and Git uncommitted status are
unavailable. The parent source manifest verifies all 394 listed files; changes
are measured by filesystem hashes against the preserved parent. Synced
`sources/`, input archives, original game data and earlier checkpoints were not
changed. The frozen 0143 and UV shader hashes still match.

Native: devkitARM r68-1/GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2; actual
ARMv6K/mpcore compilation. Host: macOS AppleClang 21.0.0.21000101, CMake 4.4.2,
Python 3.9.6. No desktop game frontend exists here; desktop validation exercises
portable libraries, importers and tools.

## Implemented and reused

- Added bounded directory/complete-extraction-ZIP intake and a hash-only 247-file
  profile of the supplied dataset. No game executable is run.
- Replaced the stale orchestration with fail-closed source checks, host tests,
  generalized cooking, tutorial validation, runtime/animation/material probes,
  deterministic host replays, native compilation and verified SD publication.
- The generalized importer's actual boot package is installed at the native
  loader's expected path. Map/render must be byte-identical to tutorial sidecars
  before the existing adapter binds messages/flow/EBD/materials.
- External task/recipe caches reuse unchanged source/tool inputs; every tutorial
  cache hit verifies exact inventory/sizes/hashes. Malformed task metadata rebuilds;
  invalid tutorial output is quarantined. Earlier output is never overwritten.
- Fixed destructive tutorial output deletion, obsolete native target/source
  checks, overwritten per-package validation logs, failed-launch diagnostics and
  SD publication that could replace an existing empty directory.
- Reused existing C++ decoders, PS1 VRAM load-order/last-writer material caches,
  generalized content graph, portable runtime/session, Old 3DS frontend, renderer
  gates, native snapshot and package validators. Gameplay code is preserved;
  native changes identify the current build/logs only.

The full changed-file list is `docs/CHANGED_FILES_0149.json`.

## Exact commands and results

Executed from `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.9_LocalImporter`:

```sh
./build_mml3ds.sh '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted' --out '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/install-first' --cache '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/cache'
./build_mml3ds.sh '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted/CDDATA/DAT' --out '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/install-repeat' --cache '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/cache' --host-only
```

All internal argv, return codes, durations and log paths are recorded in
`docs/validation_0149/build-result.json` and `repeat-build-result.json`. The host
build command is `cmake --build <host> --parallel 4`; validation uses
`ctest --test-dir <host> --output-on-failure`. Individual synthetic checks:

```sh
python3 -B tests/test_owner_data_intake.py
python3 -B tests/test_local_install.py
python3 -B tests/test_source_tools.py
python3 -B tools/verify_0146_source.py
```

Inherited baseline 10/10 suites passed before edits. Current 13/13 CTest suites
pass, including 24 intake, 28 installer and 12 source-tool synthetic cases. Both
host/native build logs have no compiler warnings or errors. All 247 profile files
match sizes/hashes ; 205 generalized packages / 31 geometry identities / 4,115 nodes /
3,916 edges validate with zero package errors. These are structural results.

The host runtime probe activates five map subareas, queries containing cells,
and expands render primitives using the same package reader/runtime code as the
native frontend. Nine native-algorithm host replay scenarios repeat; no retail
trace comparisons, jump/floor-solver parity or enemy behavior is established.

All 100 newly cooked tutorial outputs are byte-identical to the prior local pack.
The repeat run has 339 task-cache hits/zero misses, tutorial cache hit, no tutorial
recook command,205 byte-identical packages and identical tutorial manifest.
Host wall time: first 29.78648s including native compilation; repeat 4.65626s
omitting native compilation. This is not a device loading/performance comparison.
The host-only output contains no fabricated `.3dsx`.

The actual native binary is 407,224 bytes, SHA256
`8408e616234bf14083f35e0539e58e8c2ce2a343545c68eebb97bb94953ca046`.
All 119 source-snapshot identities match the final native source. Build output:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/install-first/build/Legends3DS.3dsx`.
Standalone compiled binary:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0149_LocalImporter_20261001_001.3dsx`.
Local SD layout:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/install-first/sd_out/3ds/Legends3DS/`.
It has 103 files / 4,234,490 bytes and includes owner-local cooked data; do not
redistribute the SD folder/cache. No retail asset ZIP was produced.

## Device/emulator observations and screenshots

Physical Old 3DS testing: HARDWARE_PENDING. Azahar 2126.0 was started with isolated
config/data/NAND/SD, New 3DS=false, CPU 100%, resolution 1x. The locked Mac blocks
computer-use inspection/capture. No current boot/telemetry log was obtained, so
current 0149 runtime execution is unverified. A sandboxed GUI-services failure
was diagnosed; an approved unsandboxed launch started but remained unverified.
Only the isolated task process was then stopped. No current screenshots exist;
original 0148 captures remain in their prior deliverable and are not presented as
0149 evidence. See `docs/validation_0149/emulator-evidence.json`.

Boot package 120,774 B; host estimated decoded CPU bytes 184,824 and load peak 353,472.
These are reader admission estimates, not total device RAM/linear/VRAM/stack/heap
peaks. No current frame/update/collision/render/event timing, draw-call count,
texture residency or physical memory measurement was obtained. Old-model emulator
configuration and ARMv6K compilation cannot substitute for hardware acceptance.

## Blockers and scope limits

- CHD/BIN/CUE/ISO and split archive intake are unsupported. The supplied mixed ZIP
  correctly fails with exit2 before extraction because it lacks one complete
  loose root. Directory real-data integration passes; complete-ZIP integration
  tests use synthetic data. Accurate PS1 MODE2/XA extraction needs its own pass.
- Runtime generalized bindings cover map/render. Actor/enemy roles, objects,
  events, camera, audio and progression metadata remain unbound; unknown semantics
  are not invented. Tutorial sidecars remain an explicit compatibility bridge.
- Geometry gaps/floating, negative-cell/placement/rotation uncertainty, three
  unresolved CLUT materials, incomplete collision/camera/combat/events/audio/
  natural progression/cutscenes/save semantics remain inherited blockers.
- PHYSICS_1TO1=false; no full tutorial or broader early-game native proof, no
  physical frame/memory result, no current screenshots. No website was built.
- The profile fingerprints this supplied dataset; the 13 inherited anchors passed,
  but full-profile hashes do not independently authenticate a retail master,
  certify ownership/permission, or demonstrate faithful gameplay.
- Only macOS host installation is exercised. Linux/Windows deployment remains
  unverified; Windows host-executable suffixes need separate handling.

## Readiness matrix

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
| TERRAIN GEOMETRY | CANDIDATE |
| TERRAIN TEXTURES | SOURCE_BACKED_PARTIAL |
| TRANSITIONS | CANDIDATE |

Evidence/details remain in `state/CURRENT_READINESS.json` and
`state/EVIDENCE_LEDGER.json`; no percentages are used.

## Source checkpoint and exclusions

Complete cumulative code-only source ZIP:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.9_Source_20261002_001.zip`.
Its SHA256 is recorded in the adjacent `.zip.sha256` file; the ZIP's complete
source identities are in `SOURCE_MANIFEST.sha256`. Publication rejects existing
checkpoint names and verifies archive CRC/source membership. Exclusions: raw
ROM/CHD/BIN/CUE/EXE/XA/STR/save data; Capcom art/dialogue/audio/video; all cooked
payloads; generated native/host binaries/build output; reusable caches and Python
bytecode. Hash-only metadata and source/build/test documentation are included.

Next concrete step: unlock the Mac and finish current 0149 Azahar boot/package
binding/captures, then test the SD layout on the user's original Old 3DS. The next
fidelity engineering task is original renderer placement/rotation/special-cell
research using the now-authorized supplied data; direct disc intake is a separate
format-verification task. Preserve the safe renderer and every earlier checkpoint.

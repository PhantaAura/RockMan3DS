# 0.0.14.17 — native build and Old-model emulator verified — partial

Continues verified0.0.14.16; no Git branch or commit supplied. PHYSICS_1TO1=false;
World Comes Alive remains incomplete. Parent source and owner inputs are unchanged.

Implemented optional CRC-bound L3MI section35 initialization shared by native render/
collision/runtime map, plus cold cooker and verified cooked-only installer wiring.
Exact HED admission, first match, early-null and original pointed-header seed retained;
unmatched rows preserved. Stage0 startup profile only; later variants remain UNKNOWN.
Initializer validation/application allocate no heap and run once at load. Metadata
released afterward. Legacy and malformed optional cases preserve safe boot behavior.

Owner source replay/native comparison:995 records identical,994 writes; allXYZ and
identities preserved. Three imported enemy floors now match originalY=-1. Bounds
rejection prevents unrelated diagonal groups from blocking those known samples;
intersecting/unknown/wrapped/asymmetric contacts remain Unsupported.

Session changes retain copied dialogue-bank lifetime, atomic copy assignment and
prepared save/cutscene destinations, clear scene transients and camera blends, and
separate diagnostic position recovery from real transitions. Recovery remains CANDIDATE.

Validation: inherited38/38CTestPASS; current42/42CTestPASS;24 cooked-wrapper Python
checks and14 source-tool checksPASS. Warning failures are not suppressed. Source
writer137 edge cases and original contact128 edge cases pass. More detailed source
callback research remains partial; no native fullAI or retail parity claim.

Toolchain: actualdevkitARM GCC16.1.0; ARMv6K/mpcore/hardfloat,libctru/Citro3D baseline
retained; native buildPASS (558036B,156sourcehashes,0warnings/errors). OldmodelAzahar2126.0 boot/movement/jump/fall/landing exercised; physical
Old3DS untested. No desktop playable frontend claim, portable shared core tested.

Memory: host package701943B, decoded estimate864080B, load peak estimate1911928B;
initializer transient26012B including file+decoded metadata, released afterload.
These are admission estimates, not measured device peaks. FiveEBD payloads remain
resident; broader streaming, stack/VRAM peaks and physical frame pacing pending.

Remaining: source-owned detection/BIOS RNG/contacts/animation packets for enemyAI;
lock-on/Buster damage/defeat/event/message/ordinary transition chain; exactHUD,
fullphysics/camera parity and physical hardware. Technical prototype, not full tutorial.

Source ZIP excludes retail/cooked data, original executables/discs/models/textures/
audio/video/saves, screenshots/emulators, generated binaries and reproducible caches.
See RESUME_01417.md, CURRENT_READINESS.json, EVIDENCE_LEDGER.json and changed-file list.

Current nativeSHA256: `a689f20e2d26c6152806c67db260fe10f3c57e2e67cc39203102be7eca6bf771`.
Binary: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/native-fast-r1/native-source/platform/3ds/Legends3DS_01417_WorldComesAlive.3dsx
Local owner-only SD layout: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/native-fast-r1/sd_out

Genuine capture: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/azahar-01417-r1/captures/native-current-top-bottom.png
It shows current native player/terrain/top-bottom layout during authored replay.
No physical hardware, full combat or1:1 screenshot claim.

Sparse emulator metrics (not hardwareFPS/peaks):
```json
{
  "samples": 40,
  "positions": 11,
  "states": [
    [
      "0",
      "0",
      "-1",
      "0"
    ],
    [
      "0",
      "0",
      "1",
      "1"
    ],
    [
      "0",
      "1",
      "-1",
      "0"
    ],
    [
      "0",
      "1",
      "1",
      "1"
    ],
    [
      "2",
      "2",
      "1",
      "1"
    ],
    [
      "4",
      "1",
      "1",
      "1"
    ],
    [
      "7",
      "0",
      "1",
      "1"
    ],
    [
      "7",
      "1",
      "1",
      "1"
    ],
    [
      "7",
      "2",
      "0",
      "1"
    ],
    [
      "7",
      "3",
      "0",
      "1"
    ],
    [
      "7",
      "5",
      "0",
      "1"
    ],
    [
      "8",
      "0",
      "1",
      "1"
    ],
    [
      "8",
      "2",
      "0",
      "1"
    ],
    [
      "8",
      "5",
      "0",
      "1"
    ],
    [
      "8",
      "6",
      "1",
      "1"
    ],
    [
      "8",
      "7",
      "1",
      "1"
    ]
  ],
  "malloc_live": {
    "min": 1886712,
    "max": 1893720
  },
  "owned_texture": {
    "min": 3932160,
    "max": 4194304
  },
  "owned_linear": {
    "min": 71640,
    "max": 82560
  },
  "draw_calls": {
    "min": 15,
    "max": 18
  },
  "triangles": {
    "min": 1710,
    "max": 2185
  },
  "update_us": {
    "min": 2,
    "max": 60069
  },
  "render_us": {
    "min": 9538,
    "max": 25849
  },
  "frame_before_log_us": {
    "min": 9790,
    "max": 2227383
  }
}
```

Exact validation commands: cmake -S SOURCE -B baseline -DCMAKE_BUILD_TYPE=Release;
cmake --build baseline --parallel4; ctest --test-dir baseline --output-on-failure.
Native: DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make -C
native-fast-r1/native-source/platform/3ds -j4. Source hashes verified before SD copy.
Full installer also runs at install-r1; current binary above was separately compiled
and exercised from the same frozen source. No reset redeemed.

Final one-command build/cooked validation/host checks/native compile/SD layout:PASS.
Installer binary is byte-identical to the current emulator-exercised binary.
Canonical build: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/install-r1/build/Legends3DS_01417_WorldComesAlive.3dsx
Canonical local owner-onlySD: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/install-r1/sd_out
Exact fullpipeline command:
```sh
python3 -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.17_WorldComesAlive/tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/pack-runtime --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/install-r1
ctest --test-dir /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/baseline --output-on-failure
```
Final source release: Legends3DS_0.0.14.17_Source_r1.zip, with SHA-256 sidecar.
Earlier source/recovery ZIPs are preserved.

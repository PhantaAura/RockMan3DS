# Resume 0.0.14.10 — 2026-10-02

Exact project path: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.10_TerrainPlacement`. Complete parent 0.0.14.9 is preserved. No Git repository, branch or commit exists; current changes are recorded by file hashes in `docs/CHANGED_FILES_01410.json`.

Completed: shared bounded positive IDX placement, authored XYZ/no index rotation, raw6 bank selection, unsigned height subtraction, enhanced native adapter and installer package probe. The 0.0.14.3 legacy loop/shaders remain unchanged. Both new APIs/tests, CMake and native target IDs are cumulative. The portable executable evidence tool accepts only the supplied profile and emits hashes/addresses/aggregate data.

Validation: all 421 parent manifest identities match; all 13/13 inherited parent suites and 15/15 current suites pass. Two new suites pass AddressSanitizer and UBSan (macOS leak detector off). Bounded original CPU probes pass 128 positive and 256 negative cases. All 205 packages validate; 339 cache hits / 0 misses; 100 tutorial files reused; nine native-algorithm host scenarios repeat, not retail parity. Actual ARMv6K binary: 412,332 B, SHA-256 7a4d4d426aa89a9649d4903df6ab1f2851f55fe83b466513dd5cc48ea642ec7e; all 123 native source snapshot hashes match.

Build command from project root:

```sh
./build_mml3ds.sh '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted' --out '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261002-placement/install-final' --cache '/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-local-import/cache'
```

Existing output paths are rejected. For another run choose a new `--out`. Internal exact commands/results are in `docs/validation_01410/build-result.json`; native artifact `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261002-placement/install-final/build/Legends3DS.3dsx`. Local owner-data SD layout `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261002-placement/install-final/sd_out/3ds/Legends3DS/`; do not redistribute it. Standalone code-only binary `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01410_TerrainPlacement_20261002_001.3dsx`.

Current code-only ZIP `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.10_Source_20261002_001.zip`, with adjacent SHA-256. Earlier separately named recovery ZIP is preserved. Full exclusions/build/status/readiness are in `docs/REPORT_01410.md`, current JSON state and manifest.

Azahar 2126.0 Old-model tests load all five diagnostic areas with counts 488 / 526 / 674 / 120 / 3,028. Area 0 shows textured walls and the player; the 0.0.14.3 fallback and textures-off paths also render. Four original captures are in `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01410_Azahar_20261002_001`; paths/hashes/build labels in `docs/validation_01410/emulator-evidence.json`. Controls telemetry shows movement/turning/firing and area 0 → 1. Title/action captures and controls final UI verification are pending because the Mac locked; no physical Old 3DS test occurred. After preserving telemetry, the isolated controls process (PID 23921) was stopped: its command exactly matched the launch argv, SIGTERM was followed by SIGKILL, and the PID was confirmed absent. This cleanup does not verify playback completion. The final controls UI and capture remain pending because the Mac was locked. No unrelated Azahar instance was stopped.

Known problems: 109 negative cells/floors omitted; default negative descriptor and material overrides not represented/bound. Area 4's diagnostic spawn/camera yields a blank terrain view despite admitted/submitted geometry. Positive runtime still uses fixed Mid LOD; original visibility/subdivision/alpha/camera and ground/contact solver are incomplete. Existing collision helpers keep legacy cell origins. PHYSICS_1TO1=false. Combat/enemies/events/natural progression/audio/cutscenes/save parity remain incomplete. This is a technical prototype, not a complete playable tutorial or full game.

Next concrete engineering step: represent and bind negative default-STG geometry/material lifetimes through neutral packages, verify original spawn/camera activation and contact origins, then compare stationary/moving tutorial scenes. Physical Old 3DS performance/memory validation remains required. Preserve all earlier releases and the 0.0.14.3 fallback.

Fresh usage during late validation was 32% five-hour used / 5% weekly used; that is a historical account snapshot, not a future current reading. No usage reset was redeemed by this task.

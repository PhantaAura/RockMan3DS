# 0.0.14.41-r2 report

This is a hardware-rendering investigation checkpoint. **World Comes Alive is unfinished; the physical cyan-screen fault is not yet proved fixed.**

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`. No Git repository, branch or commit. Cumulative parent: Legends3DS_0.0.14.41_Source.zip, SHA e9b654535a8985720d75f8013cb4bb8ba583aa540ab996c3eb97c1233f1af1c9. Original archives preserved.

Toolchain: devkitARM r68-1 / GCC16.1.0 / libctru2.7.0-1 / Citro3D1.7.1-2; ARMv6K/mpcore hard float. Actual compiled3dsx: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_DrawTest_01441r2.3dsx`;868916bytes; SHA `135f6517c449af35aa6f6f528f08f3be6e84e27340bb2e92c7b3379cf83f270a`.279 staged native source hashes match. Host portable core unchanged; inherited138testsPASS,18.74s. No standalone desktop visual run this pass.

Changed native code: main.cpp skips empty projectile, player-material and imported actor/model groups; X/LEFT/RIGHT choose all/terrain/player draws; camera calibration follows selection; actor inspection camera is suppressed in isolated modes; GPU queued/waiting/synchronized logs distinguish submission from completion; error console flushes. Makefile/version header identify01441r2.0143 rendering math and cumulative gameplay architecture are reused.

Prior device evidence: user Old3DSXL shows RGB bars and cyan GPU clear; X produces map/status and cyan top. Map-marker movement inconclusive.01441r2 physical retest pending.

Azahar2126.0, Old-model setting,100% configured CPU: all three0143 views render, GPU sync returns. All uses2draws/3990triangles versus prior3draws/3990triangles; terrain1draw/3504triangles; player1draw/486triangles. Owned geometry126144bytes,texture0 in each. These are emulator diagnostics, not physical memory/performance/retail parity. Isolation hides submissions without unloading resources. One player capture had a black bottom; later capture shows map. Possible bottom presentation flicker is unverified. Enhanced renderer inherited but not exercised this r2 session.

Screenshots:6 original400x480captures, device/build/scene/path/hash labels in emulator-observation.json. No hardware screenshots collected. Images are outside the code-only source ZIP; all originals retained.

Known blockers: - User physical01441: RGB bars and GPU cyan clear appear; after X top remains cyan and bottom map/status appears. Loop liveness inconclusive.01441r2 hardware retest pending.
- Empty draw groups removed; whether they caused physical failure is UNKNOWN.
- Inherited WCA: whole-world RNG chronology, true player Buster firing ownership, complete enemy encounter/reaction/defeat, original display UV reset and natural progression remain incomplete. PHYSICS_1TO1=false.

The empty-draw hardware-hang hypothesis remains CANDIDATE; source correction and emulator regression have evidence. No hardware-success or1:1 claim. Next: test X, then LEFT/RIGHT if needed; preserve boot_01441r2.log. Full readiness matrix: state/CURRENT_READINESS.json. Exact commands and build failures/fixes accompany this report.

SD archive: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01441r2_DrawTest_SD.zip`; SHA `da55b09cb4f79cef733aeb7be0a5d45ff7946899d476325229cf975d8ada295f`. Only README,3dsx,smdh. Source archive: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.41_Source_r2.zip`; its SHA is in the adjacent .sha256 sidecar to avoid a self-referential archive hash. Packaging excludes reproducible caches/build output and all retail ROM/CHD/BIN/CUE/executable/model/texture/audio/video/save/cooked payloads. None of those were added to this source tree.

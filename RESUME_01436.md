# Resume World Comes Alive 0.0.14.36

Project: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.36_WorldComesAlive
No Git repository, branch or commit. Baseline0.0.14.35 source ZIP is preserved unchanged. Synced sources/ is read-only. No usage reset used.

Completed: original class40 handlers0..12,20 imported request caches, original state4 mesh variant, guarded final player halfword mirrors, source131 sweep/deferred cleanup, exact16-slot Buster pool/lease/spawn producer. Existing runtime/renderer architecture and0143 fallback retained. The main firing caller is not connected.

Build: devkitARM r68-1/GCC16.1.0/libctru2.7.0-1/Citro3D1.7.1-2, actual ARMv6K .3dsx827580bytes SHAda9d074346a4e97c0d2105eb254b86b49481450953a1153d1268bdc5713b9b3d.131/131 host tests pass, native log0warnings;269 compiled files match source.

Commands (from project mirror root):
```sh
cmake -S work/Legends3DS_0.0.14.36_WorldComesAlive -B work/pass-20261013-worldalive-01436/host -DCMAKE_BUILD_TYPE=Release
cmake --build work/pass-20261013-worldalive-01436/host --parallel 4
ctest --test-dir work/pass-20261013-worldalive-01436/host --output-on-failure
python3 -B work/Legends3DS_0.0.14.36_WorldComesAlive/tools/build_hardware_test.py --pack work/pass-20261012-worldalive-01434/pack-runtime-r1 --out work/pass-20261013-worldalive-01436/native-r1 --host-build work/pass-20261013-worldalive-01436/host
```
Do not reuse native-r1 output for changes; choose native-r2 or a new checkpoint. No extraction/recooking occurred in this build. Detailed original differential commands are in docs/research_01436/WORLD_COMES_ALIVE_01436.md.

Azahar2126.0 Old-model traces reached all13 states/20 requests; no .36 screenshot because CUA capture twice reported locked. Input/safe-renderer visual acceptance not tested in this run. Physical Old3DS untested; PHYSICS_1TO1=false.

Next concrete step: log the first RNG mismatch once at frame2609, including raw actor fixed coordinates, rounded halfwords, scratch and pool. Then prove bounded original800172F4 lower-subtype route via scratch1F800064/table8011A604[2]/80116E68. Existing PS1 trace shows427calls at each of its3 RNG sites before first80100D6C. Native skips this callback. Do not inject reference positions/pools or bypass guards. Original timer agrees9336; firstactorZ is inferred−575or−574, not exactly observed.

Private ready work: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261013-worldalive-01436/scene-rng-context-r1/FINDINGS.md and context-evidence.json; /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261013-worldalive-01436/buster-muzzle-private-r1/RUNTIME_ADMISSION.md and proposal/. Muzzle proposal passes144 decoded+144 packed original comparisons and ARMv6K noheap tests; player mode106/secondary selection and selectors/counters still require ownership. Integrate only after those owners are admitted.

REA6.4.0 CLI acquired with task-local npm cache; direct PS-X analysis returned unsupportedformat and no Ghidra configured. No global setup or unrelated engine installs. Continue existing unchanged-MIPS oracle instead of repeating that failure.

Pending: complete enemy AI/attack, lock-on/firing/projectile hit/HP/reaction/defeat, EBD trigger/message/transition/unload/load/natural tutorial progression, normal physics and full camera/HUD/material/audio/save parity. World Comes Alive is not finished.

New source archive: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.36_Source.zip; checksum in adjacent .zip.sha256. Real binary: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01436_WorldComesAlive.3dsx. Private SD: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261013-worldalive-01436/native-r1/sd_out.

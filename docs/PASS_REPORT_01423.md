# 0.0.14.23 — World Comes Alive continuation

World Comes Alive is **unfinished**. This is a native technical gameplay prototype with substantial connected source-backed work, not a complete playable tutorial or faithful retail port.

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.23_WorldComesAlive`. No Git repository/branch/commit exists. Frozen cumulative01422 input SHA256: `b2c4bf51ddcdc0eacd67d90de7af63c4301d27ab27e329ac2d2e85fa355991c3`; all1239 source hashes verified before editing. Synced sources, owner binaries and original saves were not modified. Porting map was written before implementation; inherited81 tests passed.

## Connected changes

- Original actor pose/root translation, heading and raw scales now produce its fixed-point collision point; the exact outer proxy tail registers against bounded shared owners. Native frame540/600 show committed registration. Conditional actor replacement originally lost the new binding; the final r3 fixes both loader paths. Fresh diagnostic pools remain CANDIDATE chronology. Other actors can still hold; this does not prove Buster hits.
- Source event phase2 sound/voice-stop and command owners now bind through a validated neutral L3EQ metadata package and RuntimeSession. In the event-enabled Old-model run, phases1→2→3 queue `00000004 000000f5 03000000 00003ce0`, then correctly hold at the unported phase3/CD completion dependency. No fabricated completion, cutscene skip or audio playback.
- Original64-slot effect pool, allocation/release and full effect131 scalar lifecycle are connected to atomic child transactions. Effect128 source initialization is recovered; later consumers and native effect drawing remain blocked. No substitute model or attack effect was introduced.

Existing engine, owner-imported model/animation data, safe0143 renderer, feature gates, area ownership, collision, bounded child/proxy/event/audio queues, incremental build and persistence remain. Changed files are listed in `CHANGED_FILES_01423.json`; source-backed limits and labels are machine-readable in the readiness/evidence ledgers.

## Build and tests

87/87 host checks pass (integrated build3.79s); actual ARMv6K/mpcore/hardfloat target compiles with **zero host/native warnings**. Toolchain: devkitARMr68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2. Actual r3 binary669288B SHA256 `120ed790bca81b0ef650938368facd2d6597561738124da88d5da284833b2524`; 216 compiled source hashes match this checkpoint. Portable core/probes are tested; a full desktop game session is not claimed.

Exact high-level commands (use a fresh --out path to reproduce):

```sh
cmake -S "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.23_WorldComesAlive" -B "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/host" -DCMAKE_BUILD_TYPE=Release
cmake --build "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/host" --parallel 4
ctest --test-dir "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/host" --output-on-failure
python3 "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.23_WorldComesAlive/tools/build_hardware_test.py" --pack "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/pack-runtime-r1" --out "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/integrated-native-r3" --host-build "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/host"
```

Full nested argv, return codes and output are in `validation_01423/exact-commands.json` and build logs. Existing unchanged owner packages were validated and reused; no full recook/extractor was invoked by the native build. The new L3EQ cooker reads only exact owner-local metadata/scalars, rejects other executable hashes and refuses overwrites; its payload remains private.

Source differentials:576pose+864tail cases/582428 original instructions;128event cases+16initializers/202032 original instructions;1873effect cases/113071 original instructions. These authored input domains are not whole-game retail parity. Fixed-owner noheap tests and actor proxy ASan/UBSan checks pass. One intermediate host run caught a child fixture compared at the wrong source boundary; it was corrected to the actual source call and all87 pass. An earlier parallel run occurred before test executables finished linking; it is not final evidence. Initial manual main stack compilation lacked the p2 include path; corrected final compilation passes. Earlier emulator revisions exposed the missing conditional-load binding and are preserved separately.

## Execution, memory and evidence

Azahar2126.0 was configured as original3DS (is_new_3ds=false, CPU100%,1x). Enhanced actor/proxy replay, event-enabled phase3 hold, and safe0143 mask0 runs were exercised. Diagnostic SELECT replacement is not normal tutorial progression. Candidate RNG is opt-in; event boot and actor proxy diagnostics defaultOFF independently. Screenshots are genuine CUA captures with device/build/scene/path/hash labels in `validation_01423/screenshots.json`; they expose actor overlap/camera clipping and player texture/alpha problems. No physical hardware capture or test occurred.

The current executable reserves96MiB app region,66387968B heap and33554432B linear, with262144B stack; reservation is not actual live usage. g_children BSS33188B (+9248B for two bounded4624B effect pools), RNG1280B. Final compiler main static frame66336B; actor and child ticks2624B each, pose/proxy bridge216B, pose producer128B,event phase2 consumer192B,RuntimeSession tick120B. Frames exclude nested peak usage. Actual emulator malloc/linear/VRAM, texture/geometry, draws/triangles, transition snapshots and sparse timing maxima are in `validation_01423/native-observations.json`; repeated replay-end samples excluded from maxima. Hardware peak memory, SD spikes, nested stack peak and FPS remain unmeasured. No performance claim comes from emulator status FPS.

Original PS1 reference uses OpenBIOS and disabled memory cards/read-only observers. ST00 event phases and handoff were observed, but no admitted class03 chronology was captured. RNG observer reached its32-listener bound; reference later showed an unknown-exception dialog, with cause unestablished. Raw phase3 completion field was not observed and cannot justify native completion. The public decomp remains incomplete pinned original MIPS research, not a replacement engine; no external code/assets were imported.

## Remaining blockers

- Physical Old3DS execution, GPU/SD timing, stability and peak RAM/stack remain untested.
- Live original class03 input/RNG chronology is not admitted. Optional native RNG and diagnostic fresh actor pools remain CANDIDATE.
- Player/Buster GTE consumers, real shot-to-enemy contact, full enemy attack/AI, reaction/defeat and a complete damaging encounter remain unported.
- Effect131 scalar lifecycle is complete within its source domain; effect128 later consumers and native source-effect rasterization are incomplete. No visible attack effect is claimed.
- Phase2 queues the original type4 packet; CD/control handler8001B89C and phase3 completion bit are unported/UNKNOWN. Audio intent is not playback.
- Trigger/event/message/normal transition and natural package unload/load/tutorial progression remain incomplete; developer SELECT remains diagnostic.
- Player physics, slopes/steps/ledges, retail camera/FOV/top HUD, dynamic textures/materials and imported actor overlap remain incomplete. PHYSICS_1TO1=false.
- Dialogue, cutscenes, expressions, full retail save semantics and generalized early-game runtime semantics remain partial/unknown.

Next concrete step: trace and recover the original type4 CD/control consumer and its actual completion owner; independently finish original player/Buster GTE projection/proxy producers and admitted actor RNG chronology, then source hit reaction/defeat. Keep unknowns explicit and avoid another renderer rewrite.

Private complete SD layout: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261008-worldalive-01423/integrated-native-r3/sd_out`. SourceZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.23_Source.zip`. It includes complete cumulative code/build/project/docs/authored fixtures and excludes retail/cooked payloads, original saves, caches and generated builds. The source manifest, per-file ZIP CRC/hash verification and adjacent ZIP SHA256 establish integrity. Earlier archives remain intact; no usage reset was redeemed.

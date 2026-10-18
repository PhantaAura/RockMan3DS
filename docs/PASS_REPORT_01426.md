# 0.0.14.26 — imported tutorial head and source fade startup

This is a cumulative native Old3DS technical gameplay prototype. **World Comes Alive is unfinished; PHYSICS_1TO1=false.** The fresh tutorial head now comes from the proven original HEAD01/PL00H01P.PBD selector rather than H00. Player opaque-texture alpha handling is corrected, and the source phase4 fade startup reaches phase5 in the actual native emulator runtime. A complete imported enemy encounter and natural tutorial progression are still blocked.

## Source and reuse

Exact source: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.26_WorldComesAlive

Parent archive: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.25_Source.zip

Parent SHA-256: f8b44e9baa5e3331380270e181c860a9591ae2f7e1652f4a898e239315195c0a

No Git repository, branch or commit is available. Synced sources and earlier archives were not changed. The existing engine, generalized area package/loader, source actor/pose/collision/event/audio modules, PS1 VRAM last-writer material caches, save framework, independent renderer gates and0143 fallback were reused. No external engine or replacement character assets were introduced. Historical research remains cumulative; this report/current readiness overrides older intermediate diagnoses.

## Implemented changes and limits

- Source initializer8003BE6C fresh branch8003BF58 sets Player_work+16A=0. Loader8001F5E4 indexes80089200,fileID28,HEAD01.BIN and PL00H01P.PBD. Exact executable hash/bounds/container identity guards derive this without filename guessing.264authoredcases execute4476unchanged original instructions. Private recooking changes only the existing head output; H01 is102vertices/76triangles/31quads versus H00's112/46/61, with the same original fourpart IDs. Material/composition recook outputs remain byte-identical. These meshes/animations are imported originals; later gear/resumed/dynamichead choices remain unbound.
- Player material draws separate bounded opaque/semitransparent packet groups. Opaque TEValpha scale2 preserves zero texel discard and makes STP128 texels opaque; original head codes25/2D are opaque. Neutral128RGB gain and page/CLUT identity are preserved.8opcode/6authoredtexel-class tests pass. Semitransparent blendmodes, terrain/actor alpha, dynamic palette timing and complete visual parity are not proven. The original SDK rule is documented in the [Run-Time Library Overview](https://psx.arthus.net/sdk/Psy-Q/DOCS/LibOver47.pdf#page=106), page106/table8-11.
- Corrected the previous driver diagnosis:80017C30 is fade/display startup. Exact type2 busy-noop or owner-duration/step constructor writes and a bounded pending80012E10(slot2,callback80017E94) intent are now wired through source_event_pipeline, RuntimeSession and native main.517authoredcases/126709unchanged instructions (no helper hooks/patches) cover busy behavior and256GPUtype inputs. Optional L3FDv1 is68B with guarded identity/CRC; malformed/missing/full dependencies hold without mutation.4096updates allocate nothing in the tested module.
- Pending intent is explicitly not a BIOS thread receipt. No callback, fade completion or CD/audio acknowledgment is fabricated. Rebinding preserves pending state; accepted transitions/save invalidate it, rejected transitions retain it. Final Azahar telemetry reachesphase5/frame68 withpending1/presentation_complete=false, then holds on80100EFC/frame69. Fade rendering/callback and natural progression remain unported.
- Release guard now permits inherited textual .su stack-usage evidence while retaining binary/retail/cooked/symlink rejection. A meaningful guard regression passes. Native title/boot log/binary naming now consistently identify01426.

See docs/CHANGED_FILES_01426.json for every changed/new path and hashes; docs/research_01426 and docs/validation_01426 contain machine-readable proof and exact commands. No source warning was silenced; final host/native logs contain zero compiler warnings.

## Build and tests

Toolchain: devkitARM r68-1, arm-none-eabi-g++16.1.0, libctru2.7.0-1, Citro3D1.7.1-2; ARMv6K/mpcore hardfloat. Baseline96/96host checks passed before edits. Final99/99host checks pass, including all inherited tests; ps1_geometry/source_fade/runtime_fade pass3/3ASan+UBSan. Relevant CMake checks use warnings-as-errors and Release assertions/always-on guards.232compiled source hashes match the final build. The native binary is705744B, SHA-256 943db29d8b45b0b4063aa6150396a12d24a3e62078e5520e20e3d9e6f023d9f3.

Actual workflow used:

~~~sh
python3 -B "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.26_WorldComesAlive/tools/build_hardware_test.py" --pack "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261010-worldalive-01426/pack-runtime-r3" --out "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261010-worldalive-01426/native-r4" --host-build "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261010-worldalive-01426/host"
ctest --test-dir "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261010-worldalive-01426/host" --output-on-failure
~~~

The build workflow packages existing private cooked data and does not recook unchanged assets. The separate supported head/material and fade cooks are recorded in docs/validation_01426/commands.json. That file also records exact baseline/native CMake/make commands, sanitizer flags, original-instruction probes and return codes. Reruns must use new output paths for exclusive cook/probe/release commands.

Early failures are retained: the authored fake-EXE routing fixture needed the new cooker dependency mocked without weakening its real identity guard; duplicate arm resource extraction initially rejected matching copies, fixed by validating equality and keeping first; native-r3 still displayed an01425menu label, fixed by recompiling native-r4. Full logs and reasons are in test-results.json. No failing test was disabled.

Desktop: portable core/tools and inherited host checks compile/pass. No new desktop graphical game runtime or desktop visual-parity run is claimed.

## Emulator and physical hardware

Azahar2126.0, isolatedSD/NAND, original-model setting,100%CPU,1x resolution. Runtime model query returns new3ds=0. Enhanced renderer7f/source event+XA enabled and independent safe renderer flags0 both boot. Safeboot resolves renderer8 with the0143shader/legacy flat geometry; movement changes position from(-768,0,-2816)to(-768,0,-4416). Enhanced player movement/animation, turn input, existing projectile emission and diagnosticSELECT0to1 replacement are observed. No real enemy Buster-hit/defeat or natural tutorial transition is claimed.

Final runs used manual CUA input. Launch metadata retained an unused earlier movie hash; no movie argument was supplied, so these are not deterministic replay or retail parity results. Source probes use unchanged owner instructions with authored memory, not a fresh live original gameplay trace. Physical Old3DS/XL has not been tested.

Final native artifact: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01426_WorldComesAlive.3dsx

Complete local SD layout: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261010-worldalive-01426/native-r4/sd_out (private owner data; not distributed in source ZIP). Source-only binary needs that owner-local content to run. Current captures are originals listed with hashes in docs/SCREENSHOTS_01426.md; both native top and bottom screens are present. Camera-inside-wall failure is retained as evidence. No captures were generated, edited or passed off as hardware evidence.

## Performance and memory observations

Enhanced sparse emulator samples: median frame16341us (~16.34ms), maximum2062706us during loading,12–18drawcalls and1478–2154triangles. Audio open took about5.34s and maximum sampledsector read2312us. Measured stream245 completed3927168NDSPframes (PLAYING/DONE),0sampled underruns/DSPdrops; this does not prove audible quality, source decoder history, retail timing or physical playback.

Enhanced tracked peaks: ownedlinear71640B,tracked textures3670016B,malloc_live1937304B. Diagnostic transition shows0ownedPCM/index after unload. Safe0143 uses126144Bownedlinear/0textures. Fade retains fixed context/8intent capacity; host tests show4096allocation-free updates. Total SDK heaps/VRAM/stack/per-frame hidden allocations and sustained hardware peaks remain unmeasured. The100663296B/96MiB region plus reserved heap/linear/stack are reservations, not live consumption. Sparse emulator timing is not Old3DS performance acceptance.

## Remaining blockers and next step

- World Comes Alive remains unfinished: complete enemy attack, lock-on/Buster-to-enemy hit, HP/damage/reaction/defeat and natural tutorial progression are not proven.
- Event phase4 fade startup is now bound; phase5 next holds on80100EFC. Pending80012E10(slot2,80017E94) intent has no fade callback/presentation/completion consumer. Do not fabricate acknowledgments or wait semantics.
- Live original class03 shared RNG chronology is UNKNOWN; candidate native RNG remains defaultOFF. Host authored instruction comparisons are not live retail parity.
- Fresh tutorial head selection is source-backed HEAD01/PL00H01P.PBD. Resumed saves, later equipment and runtime head switching remain unbound; full material/camera/visual parity remainsUNKNOWN.
- Source camera/projection helpers are retained, but ordinary native camera ownership remains unbound. Genuine captures show camera inside walls, inaccurate terrain/material layers, dark primitives and actor occlusion. Original top-screen PS1 HUD remains incomplete.
- PHYSICS_1TO1=false. Full player slopes/steps/ledges and source-synchronized encounter controls are incomplete.
- Measured stream245 completes3927168 native NDSPframes in Azahar, but audible quality, cold-zero decoding history, retail seek/pause/loop/timing and physical DSP behavior remain unverified.
- Physical Old3DS/XL, real SD latency, worst-case frame pacing and total SDK/RAM/linear/VRAM/stack peaks remain HARDWARE_PENDING. The96MiB region reservation is not measured live usage.

The next concrete source step is original80100EFC phase5 and80017E94 fade callback ownership/native presentation. After this boundary, continue the source-supported encounter and event/message/transition chain; do not substitute elapsed-time completion. The source queue deliberately stops at unknown semantics. Camera/material presentation, PS1 HUD, full physics and hardware acceptance remain independent blockers. This checkpoint is not a finished tutorial, full game port or completed World Comes Alive.

## Readiness matrix

Only the required evidence labels are used. Historical importer counts describe structural coverage, not gameplay completion.

| System | Status | Evidence/limit |
| --- | --- | --- |
| ANIMATION | SOURCE_BACKED_PARTIAL | Owner-imported original bank/poses retained. Source root translation/heading/scales produce collision point with preserved work186; rendering alone is not used as a collision center. Complete retail state synchronization pending. |
| AUDIO | SOURCE_BACKED_PARTIAL | Measured owner XA stream245 completes3927168native NDSPframes in finalAzahar run,0sampled underruns/drops; explicit diagnostic unload0PCM/index. Fullstream buffer completion is not audible-quality/retail timing/seek/loop/hardwareproof. Otherformats remain unsupported. |
| BOOT | SOURCE_BACKED_PARTIAL | Final01426native-r4 runs in Azahar2126.0 Oldmodel/100%CPU/1x. Enhanced gameplay and independent safe0143 boot/movement captured. Physicalhardware untested. |
| BOTTOM SCREEN | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Optional companion, follow/heading, tab/zoom/pan/center commands, touch gestures and double-buffer reuse pass host tests and render in native emulator. Dedicated native touch/button acceptance pending. |
| BUSTER | CANDIDATE | Inherited replay records retain Y/projectile cases; current01426manual telemetry observes one existing projectile. Retail cadence/aim/damage/enemy targets and a real hit are not verified. |
| CAMERA | SOURCE_BACKED_PARTIAL | Original800384F0 directhelper and projection/fog setters pass1402authoredcases/ASan/UBSan/actualARMcompile. Native ordinary writer/context unbound; candidate currentrenderer camera/FOV retained, actorocclusion unresolved. |
| COLLISION | SOURCE_BACKED_PARTIAL | Original actor fixed-point collision point, bounded proxy registration and child/effect ownership are connected; actor floor/wall subsets retained. Complete player steps/slopes/ledges and GTE contact consumers remain partial. |
| COMBAT | SOURCE_BACKED_PARTIAL | Source80035508 lease/GTE module and ChildRuntime alias adapter connected, but native source camera/control owner UNKNOWN. No complete Buster-to-enemy encounter. |
| CONTENT GRAPH | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. 205packages/4115nodes/3916edges validated; structuralcounts do notestablishgameplaycompletion. |
| CUTSCENES | CANDIDATE | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Inherited cue scaffolding is not complete original scene playback. |
| DIALOGUE | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Existing verified-pair tutorial message adapter retained; generalized bank/event selection unbound. |
| EARLY-GAME IMPORT | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. 37geometrypackages/203areas checkedhostside; fivebaselineareas plus one structurallydistinct package nativelyloadedinAzahar; natural progression andcompleteboundaryproof remain pending. |
| EBD / EVENTS | SOURCE_BACKED_PARTIAL | Actual frame13 source audio command pop,event3to4,frame68 phase4to5 with exact type2 startup/pendingfade1. Next frame69consumer80100EFC holds.517authored unchanged-instruction cases and bounded runtime/unload/rebind tests pass. No callback or fade-completion acknowledgment fabricated; dialogue/natural progression incomplete. |
| ENEMIES | SOURCE_BACKED_PARTIAL | Inherited01423 evidence registered original class20/03 source pose-derived point/outer proxy tail in native frames540/600. Current01426run has source_actor_proxy=false and candidateRNG=false; no new live AI/attack/encounter proof. Original live sharedRNG chronology and other required owners remain unknown. |
| EXPRESSIONS | UNKNOWN | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Original expression/eye/mouth state mappings absent. |
| FRAME PACING | HARDWARE_PENDING | Enhanced emulated samplemedian16341us,max2062706us loading; drawcalls12..18,triangles1478..2154. Sparse emulation sampling cannot establish Old3DS worst-case or sustained pacing. |
| GENERALIZED IMPORTER | SOURCE_BACKED_PARTIAL | Cumulative importer retained. New exact-owner freshhead selector and canonical68B fadeprofile cooking/provenance wired into tutorialcooker.121runtime outputs hash-validated; historical contentcounts do not establish gameplay completion. |
| GENERALIZED RUNTIME LOADER | SOURCE_BACKED_PARTIAL | Cumulative area paths retained; optional68B fadeprofile loader bound to native session with malformed/missing held dependent path. Explicit pending8-entry queue/context resets on accepted transitions/save; rejected transitions preserve state. Broader gameplay semantics remain incomplete. |
| LOADING | SOURCE_BACKED_PARTIAL | Final emulator sourceaudio open about5.34s; sectorread maximum2312us. Diagnostic0to1 replacement unloads audio and host tests invalidate fadeowners. Natural transition/hardwareSD latency remain unproven. |
| LOCK-ON | CANDIDATE | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Inherited provisional integration retained; new packages do not establish retail behavior. 01416 source heading kernel validated against owner executable; native sparse queries do not perform acquisition/retention or aiming. |
| MEGA MAN MATERIALS | SOURCE_BACKED_PARTIAL | Owner load order/UV/CLUT/material cache retained. H01 recook leaves material caches/composition byte-identical. Opaque player STP rule tested across8opcodes/6authored texel classes and compiled/exercised; semi blendmodes/dynamicpalette/full presentation remain unverified. |
| MEGA MAN MODEL | SOURCE_BACKED_PARTIAL | Fresh source initializer+loader derives HEAD01/PL00H01P.PBD instead of previous H00.264authored original-instruction cases pass; private cookedhead updated and native player visible. Original other components/animation retained. Dynamic/resumed/equipment selectors and full visualparity UNKNOWN. |
| MEMORY | SOURCE_BACKED_PARTIAL | Fade context and8pending intents bounded;4096allocation-free authored updates. Enhanced samples peakownedlinear71640B,texture3670016B,malloc_live1937304B; safe0143ownedlinear126144B/texture0. Sampled partial counters exclude full SDK/stack/hardwarepeaks; region100663296B is reservation. |
| MOVEMENT | SOURCE_BACKED_PARTIAL | Recorded native movement,jump/fall/land replay retained; new Action4 original forward/timer/floor rollback arithmetic connected. This is not native-vs-retail movement parity. |
| OBJECTS | SOURCE_BACKED_PARTIAL | Existing object/child pools retained; exact64-slot72B source effect allocation/release and effect131 lifecycle integrated. Effect128 initialization is source backed; later resources/proxies/render consumers hold. |
| OLD-3DS BUILD | SOURCE_BACKED_PARTIAL | Actual finalGCC16.1.0/devkitARM r68-1/libctru2.7.0-1/Citro3D1.7.1-2 ARMv6K/mpcore build PASS;232compiled source hashes match.99/99host and3/3ASan/UBSan pass,0host/nativewarnings. Compilation is not hardwareproof. |
| OLD-3DS HARDWARE | HARDWARE_PENDING | No physicalOld3DS test. Original-model emulatorsettings and nativeARMtarget recorded separately. |
| PHYSICS | SOURCE_BACKED_PARTIAL | PHYSICS_1TO1=false.1568 Action4 body cases match original instructions under declared premises; full player physics/slopes and live retail chronology incomplete. |
| QoL | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Inherited options preserved; companion visibility and camera-independent map controls added. No invented inventory/EN/currency. |
| RENDERER | SOURCE_BACKED_PARTIAL | Independent0143 safe renderer preserved and exercised; enhanced player opaque polygons now ignore STP128 alpha while preserving zero texel discard. Legacy flat terrain and existing camera/material/actor errors remain; no full PS1 rendering parity. |
| SAVE | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. Safe native persistence/research retained; removed location IDs require re-registration before save restore. Retail field semantics incomplete. |
| TERRAIN GEOMETRY | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. 37actual map/render packages,203areas:58,000 formerly omitted negativeMid primitives now bound; positives unchanged. Native5baselineareas match496/597/688/136/3028. Original LOD/clipping parity unproven. |
| TERRAIN TEXTURES | SOURCE_BACKED_PARTIAL | Historical subsystem evidence; current inherited tests rerun, full subsystem not newly proven. PartialVRAM uploads/last-writer composition and recovered CPU palette bank staging; five terrain caches resolve9/12/12/4/2 materials;109baseline negativebindings. Other default/special material states UNKNOWN; dynamicpalette timing UNKNOWN. |
| TRANSITIONS | SOURCE_BACKED_PARTIAL | Final enhancedAzahar SELECT0to1 diagnostic replacement exercised; telemetry shows audio PCM/index0. Host tests cover pendingfade reset/rejectedtransition preservation. Original naturaltransition, destination/event/message chain remains incomplete. |

## Checkpoint and exclusions

New cumulative source ZIP: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.26_Source.zip

SHA-256 is stored next to the ZIP in its .zip.sha256 sidecar and release-result.json; the archive cannot include its own digest. SOURCE_MANIFEST.sha256 covers every included file except itself. The final release verifier checks ZIP CRC, manifest hashes and required source members.

Source ZIP contains complete cumulative source/build scripts/tests/text documentation. It excludes every ROM/CHD/BIN/CUE/EXE, Capcom model/texture/animation/audio/video payload, retail save, cooked package and DSP firmware. Generated build output/caches are excluded; actual compiled .3dsx is separately supplied. Original screenshots remain outside the code-only ZIP at their capture paths. See docs/SOURCE_EXCLUSIONS_01426.json. Earlier checkpoints are never overwritten; recovery_r1 is retained as a historical mid-work snapshot. No Codex usage reset was used.

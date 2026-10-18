# 0.0.14.15 resume

Exact editable source:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.15_WorldComesAlive`.
Parent0.0.14.14_Source_r1 SHA-256
`d9c5e7d2bb53352cc11be05e6b47e746bb3314c03084cfd5f712a929c24a9e61`.
No Git repository/branch/commit supplied. Synced sources and all originals remain
unchanged. No usage reset or physical hardware test occurred.

This is a partial technical prototype. World Comes Alive is unfinished;
PHYSICS_1TO1=false. Native complete enemy AI/HP/hit delivery and normal
encounter→event→message→transition→package replacement remain incomplete.

Final actual r6 ARMv6K build:548820 bytes, SHA-256
`eba03c7de31ee4e2b8294457bc325e0a12370dcb38ad4406f1088880783a56cb`.
Local path:`work/pass-20261004-worldalive/install-r6/build/Legends3DS_01415_WorldComesAlive.3dsx`.
Toolchain:devkitARM r68-1/GCC16.1.0/libctru2.7.0-1/Citro3D1.7.1-2.
36/36host suitesPASS; native compilation warning-free;150 snapshot source hashes
and build/SD binary identity match. Exact commands/results and logs are in
`docs/validation_01415/build-result.json` and `REPORT_01415.md`.

Changed files:complete parent comparison in `docs/CHANGED_FILES_01415.json`.
Completed:672 source running-jump cases, platform edge retention, floor interval
240cases and actor bound floor queries1944cases; L3SP catalog7regions/45records;
source gate/spawn into real class20/03 mesh and initializer pose; C0/03 palette
correction; bounded diagnostic area GPU/cache preparation/commit/restore.
Actor conditional reload now validates candidate before replacement, retains
old stale presentation after failure, consumes revision once and releases old
ownership on success. RuntimeSession logical source state is not rolled back.

New source-only kernels:2018 waiting/turning cases and675 distance cases;
240sequence cases and36dispatcher admission cases. FullAI/RNG/heading/detection,
request changes/attacks/hit delivery remain unbound. Original request0 eight-pose
cache uses one shared resource and allocation-free per-tick playback. Independent
owner-local gate `actor_animation_01415.cfg` with `RETAIL_ACTOR_ANIMATION=1`
is defaultOFF and enhanced-path only; class35 remains static. Retail cadence,
pause ownership/distance flag and allocation-first-frame remain unknown.

Final Azahar2126.0 Old-model CPU100/1x run:
`work/pass-20261004-worldalive/azahar-01415-animation-r6`.
Synthetic diagnostic start0,-1,-7200 heading2048. Package boots;3actors/1cache,
1836cache-element bytes/4888total actor-elements on ARM.58CSV samples show SELECT
area1→2 and continued movement10,-384,11185 (contact2Unsupported,grounded unknown).
Original player/top-bottom image and post-switch occlusion image stay outsideZIP;
see SCREENSHOTS.json. Clear enemy playback and hardware performance not accepted.
Historical r4 supported-fixture jumps/landings/captures remain separately labeled.
Finalr6 original0143 fallback boots with actor animationOFF in separate run.

Owner-only pack:`work/pass-20261004-worldalive/pack-spawn` (101existing outputs
reused plus1272-byte spawn catalog); local SD:`install-r6/sd_out`.
Code-only ZIP excludes every raw/cooked owner payload, executable/overlay, build,
image, movie, emulator state and save. Source release tool verifies UTF8/NUL-free
source including authoredLua, fails closed and never overwrites earlier ZIPs.

Exact reproduction from editable source using a fresh output path:

```sh
python3 -B tools/build_hardware_test.py --pack ../pass-20261004-worldalive/pack-spawn --out ../pass-20261004-worldalive/install-r6
```

This command performed CMake Release build,36CTest suites, package/probes and
native make. Existing output directories are exclusive; choose a new name.

Next concrete step:read-only original-game write observation during an authorized
area reload to identify the live IDX selector mutation. Original tutorial RAM8=0
and IDX78/94/123 raw6=1 select alternate lists while static/cooked raw6=0 selects
null defaults. Observe word+4 at8015C3AC/8015C46C/8015C5C8; see
`docs/research_01415/REDUX_CONTACT_REFERENCE.md`. Exact writer/timing/domain remain
UNKNOWN; no speculative floor patch. Player rawslot0 is not inactivity; latest
reference930,-1,-10135 flags3/state1/0/1, no active20/03actors observed. Statsselector1
is not a native HPdefault. User-controlled PCSX-Redux reference remains running;
no original save writes/cards enabled. Reference capture is not3DS evidence.

Then finish collision ownership, native sourceAI/HP/hit packet delivery and one
real encounter, execute proven event/message chain and a normal transition with
actual package unload/load. The final event branch alone is source proof under
completion premise, not complete cutscene execution. No fabricated event/physics
semantics, borrowed palettes or remade stand-in models.

Remaining:fullslopes/steps/ledges/wall/seam recovery, contacts, original HUD,
camera obstruction, actor materials/expressions, audio/cutscenes, area-local
package/EBD streaming (567232EBD bytes across5bindings remain resident),
physicalOld3DS memory/pacing/stability and clear enemy acceptance.

Cumulative code-only ZIP:
`deliverables/Legends3DS_0.0.14.15_Source.zip`, external SHA256 sidecar and independent
release verification. Earlier Recovery001–004 remain untouched. Read current
checkpoint/readiness/evidence/report before resuming; fullpass is not complete.

Final fallback note: r6 boot log reaches runtime ready renderer0 with actor animationOFF. Final fallback screenshot failed because the computer-use service could not start; no new fallback image is claimed.

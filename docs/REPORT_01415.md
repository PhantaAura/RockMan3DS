# 0.0.14.15 — partial World Comes Alive checkpoint

This is a buildable native technical prototype. It extends the cumulative
0.0.14.14 source with an ordinary running-jump subset, bounded floor-query
improvements, a source-backed conditional spawn catalog and actual class20/03
mesh/initializer pose binding. The historical r4 Old-model emulator replay exercises
jumps, landing, three source meshes and a diagnostic SELECT area switch. Latest
source also contains bounded enemy behavior/sequence kernels, a default-off
imported request0 pose cache and transactional actor presentation reload. The final r6 build passes36/36 host suites and compiles warning-free. Azahar Old-model boots the package and owner-opt-in cache; source/SD hashes match.
It does **not** complete the World Comes Alive objective: a real enemy encounter
followed by normal event/message/transition execution and continued gameplay.
`PHYSICS_1TO1=false`; physical Old 3DS acceptance remains pending.

## 1. Exact source and revision

Current source:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.15_WorldComesAlive`.
No Git repository, branch or commit was supplied; those fields are null.
The cumulative parent is `0.0.14.14_Source_r1`, archive SHA-256
`d9c5e7d2bb53352cc11be05e6b47e746bb3314c03084cfd5f712a929c24a9e61`.
Synced `sources/`, earlier checkpoints and owner inputs were not edited.

The final actual r6 binary is
`work/pass-20261004-worldalive/install-r6/build/Legends3DS_01415_WorldComesAlive.3dsx`,
relative to the project mirror root: **548,820 bytes**, SHA-256
`eba03c7de31ee4e2b8294457bc325e0a12370dcb38ad4406f1088880783a56cb`.
`validation_01415/native-snapshot-verification.json` verifies150 source snapshot
files and build/SD byte identity, with zero mismatches. The final source ZIP is
separate from this owner-local installation.

## 2. Toolchain and versions

Native target: devkitARM `r68-1`, GCC `16.1.0`, libctru `2.7.0-1`,
Citro3D `1.7.1-2`; ARMv6K/mpcore with hard float, the Old 3DS baseline.
The CMake core uses C11/C++17 and warning-fatal host compilation. Focused GCC
checks use the actual devkitARM compiler, including the historical renderer
warning negative control and the current serializers. No warning suppression
was used. The emulator is Azahar `2126.0`, Old model (`new_3ds=false`),
CPU setting 100%, native 1x resolution.

## 3. What was exercised

| Environment | Current evidence | Limit |
| --- | --- | --- |
| Desktop host | Final r6:36/36 CTest suites PASS | Host tests do not execute the full retail game |
| Native build | Final r6 ARMv6K/libctru/Citro3D compilation PASS warning-free | Compilation does not prove device acceptance |
| Azahar Old model | r4 boots; package/ref/flow/message/EBD/spawn bindings reach ready; standing action7 and running action8 jumps, landing, three instantiated class20/03 meshes and SELECT area1→2 occur in a synthetic input replay | Diagnostic start, sparse HLE samples; latest scene visibility remains occluded; no complete encounter or normal progression |
| Physical Old 3DS | Not tested | Hardware pending |

The historical r4 primary replay starts at diagnostic area1 `(352,-1,-8128)`, yaw2048. A separate yaw0 capture shows the imported player and both screens. Its recorded
standing jump starts action7 at frame194, launches with velocity-448 at frame198,
reaches Y-175 and returns to Y-1 by frame222. Running action8 launches at frame317;
subsequent travel reaches an unsupported contact branch before a later supported
landing sample. These are native emulator observations, not a new PS1 trajectory
comparison or proof that every jump/contact path is correct.

### PS1 reference follow-up

The original SLUS00603 disc reached user-controlled tutorial gameplay in
PCSX-Redux `f78f8ef8` interpreter with OpenBIOS `0b0359a7`, isolated portable
settings and memory cards disabled. Live floor/geometry/HP code hashes match
the supplied executable. The tutorial snapshot records player flags0=3, raw
slot marker1=0, state bytes8/9/A=1/0/1 and XYZ=(930,-1,-10135). Raw slot0 is
not an inactive generic-actor class guard.

Matching ST00 IDX78/94/123 identities have static raw6=0 but live raw6=1,
selecting non-null alternate contact lists. That is `MATCHES_MEASURED_FIELDS`
for the captured state; exact writer, initialization timing and domain remain
`UNKNOWN`. RAM8 was zero and raw stats selector1. No active class20/subtype03
was found across all16 slots in the latest snapshot, so live enemy context/HP
is still unmeasured. RAM8=0 does not prove a NULL-selected gameplay floor
call, and no blanket selector toggle or NULL-as-empty rule was added.

Source-API read-only counters establish delivered Start/Cross input. Earlier
CUA coordinate input failed with `NoWindowsAvailable`; subsequent Keyboard
configuration and user input reached gameplay. The owned reference remains
running under user control; no additional input or restart was performed.
See `validation_01415/redux-contact-reference-result.json`,
`redux-tutorial-live-profile.json` and `redux-tutorial-controller-proof.json`.
The intro-only profile is retained as historical evidence. Genuine tutorial
captures remain outside the source in the reference capture directory.

The next source boundary is observing the raw6 0→1 writer during an authorized
new-game/area reload, including aligned word+4 writes. No new live
PS1-versus-native trajectory parity is claimed. The historical 01414 forward
comparison remains only its measured 25-update field window.

## 4. Changed files and reused systems

The authoritative complete list is `CHANGED_FILES_01415.json`; it is generated
from the cumulative parent comparison. The systems changed in this pass are:

- Ordinary running-jump action8, short input-edge bridging, contact safety and
  allocation-free floor interval handling; original scalar cases accompany
  the implemented subset.
- Neutral `L3SP` v1 catalog/cooker and optional package section34. C0/21 enables
  the E0/00 spatial selector only where the source controller exists. State1
  chooses the last passing different-qualifier region; state2 filters story
  flags and spawns on the next tick. Static and conditional records share
  bounded family pools.
- Class20/03 resource and initializer pose admission, installed-body floor-query
  bounds and explicit-input HP/hit research. Mesh rendering uses real owner
  resource data; native instance AI/HP/hit delivery is incomplete.
- Source-backed class20/03 action0/action1, explicit-input RNG selection and
  distance-work80 scalar kernels; these are not yet bound as native enemy AI.
  Shared source sequence advancement and dispatcher admission are separately
  validated. The owner-local initializer-selected request0 cache has eight
  genuine distinct imported poses, shared by instances and allocation-free on
  ticks; native playback defaults OFF. Cadence is CANDIDATE, pause ownership and
  allocation-frame/first-tick timing remain UNKNOWN.
- Transactional diagnostic area preparation/commit/restore, transactional actor
  presentation replacement, material admission, source-release exclusions,
  regression coverage and checkpoint evidence. Failed actor reload retains old
  stale presentation and records the failed revision without same-revision
  per-frame retries; logical source state is not rolled back.

The existing RuntimeSession, shared map/EBD ownership, typed L3RB actor bindings,
player/model/pose pipeline, material caches, UV renderer, original0143 fallback,
message token cursor, companion bottom UI, native save scaffolding and owner-local
installer are reused. The pass does not rewrite the engine or replace unknown
source behavior with made-up event commands.

C0/03's historical script interpretation is explicitly superseded: its consumer
interpolates sixteen RGB555 palette halfwords. `0x800AD168` is color storage,
not gameplay bytecode. `research_01415/event-source-evidence.json` identifies the
correction and the separate real event poll/handler/interpreter addresses.

## 5. Exact build commands and validation

The final executed commands, from the source directory, use output outside
distributable source:

```sh
python3 -B tools/build_hardware_test.py --pack ../pass-20261004-worldalive/pack-spawn --out ../pass-20261004-worldalive/install-r6
cmake -S . -B ../pass-20261004-worldalive/install-r6/host -DCMAKE_BUILD_TYPE=Release
cmake --build ../pass-20261004-worldalive/install-r6/host --parallel 4
ctest --test-dir ../pass-20261004-worldalive/install-r6/host --output-on-failure
../pass-20261004-worldalive/install-r6/host/l3runtime_spawn_probe ../pass-20261004-worldalive/pack-spawn
```

The first command already performs the listed CMake build/tests, explicit cooked
package generation, area probes and native make. Existing outputs are exclusive;
a reproduction uses a fresh output path. Exact executed argument vectors and
return codes are preserved in `validation_01415/build-result.json`.
The installer did not invoke extractors or cookers. The pass's separate new spawn
cooker retained the owner overlay's selected conditional data; it did not recook
unchanged terrain/player/message/EBD payloads.

| Check | Result and evidence |
| --- | --- |
| Host suites | Final r6 36/36 PASS, `validation_01415/host-tests.log` |
| Actual native build | Final r6 PASS warning-free, `validation_01415/native-build.log` |
| Native snapshot identity | Final r6:150 files, zero mismatches, `native-snapshot-verification.json` |
| Running jump subset | 672 original scalar cases, `running-jump-source-cases.json`; other directions/modifiers and full tick ordering remain unknown |
| Shared floor intervals | 240 source cases, `floor08-source-evidence.json`; floor/ceiling interval transform only, not slope motion |
| Installed actor bounds floor query | 1,944 original rectangle cases, `class20-03-floor-source-evidence.json` |
| Spawn source/cook | 105 source gate cases, 16 flag reads; 7 regions/45 records, 1,272 cooked bytes; byte-exact records/scalars and pointer removal in `research_01415/spawn-owner-roundtrip.json` |
| Actual owner spawn | 3 meshes, 15 parts, 180 triangles, 147 quads, 3,252 shared decoded element bytes; `runtime-spawn-final.log` reports supported diagnostic contact1 |
| HP/hit research | 2 explicit HP profiles and 5 isolated hit-consumer cases, `class20-03-source-evidence.json`; source selector2 gives 128 HP, others 64 HP in the admitted context |
| Behavior kernels | 2,018 original action/selector scalar cases and 675 distance-store cases; explicit missing inputs preserve state; not native AI callback binding |
| Sequence subset | 240 original sequence scalar cases and 36 callback-admission cases; complete scheduler/cadence unknown |
| Request0 pose cache | Eight genuine owner-local poses; authored 480 ticks ×3 actors, 1,440 owner-probe steps, zero tick/matrix allocations; source payload stays local; native gate OFF |
| Package | Existing 685,745-byte cooked boot envelope with bounded optional spawns and actor bindings; `area-runtime-probe.log`, `pack-validation.json` |
| GCC checks | Historical warning fails as expected/current path passes; four serializer compile-only checks pass; `renderer-gcc-warning/results.json`, `serializer-gcc/results.json` |

The initial fixture omitted the newly required spawn cooker; its fixture inventory
was corrected and the final complete suite passed. Earlier recovery notes that
called contact_status2 a supported subset are superseded: enum2 means Unsupported.
Current actor diagnostics and the final grounded replay use the correct enum. `selected-contact-admission-audit.json` proves that the original zero branch tests the word read from selected_pointer+8; it does not establish NULL-as-empty handling. Tutorial RAM8=0 and live IDX raw6=1 are now measured; exact selector writer and
NULL-selected gameplay call semantics remain UNKNOWN.

The historical r4 native emulator command is:

```sh
/Applications/Azahar.app/Contents/MacOS/azahar -w -p /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261004-worldalive/replays/worldalive-ground-jump-01415.ctm /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261004-worldalive/install-r4/build/Legends3DS_01415_WorldComesAlive.3dsx
```

Use the isolated XDG paths and Old-model settings recorded in the run's
`launch.json`; arbitrary global settings are not an equivalent reproduction.

## 6. Performance, memory and remaining risks

The final r6 replay uses a synthetic diagnostic start `(0,-1,-7200)`, yaw2048,
and explicit local `RETAIL_ACTOR_ANIMATION=1`. Its boot log binds three class03
actors to one eight-pose cache:1836 cache decoded-element bytes on ARM,4888 total
actor decoded-element bytes. The58 sparse samples span areas1/2; after SELECT,
movement continues to `(10,-384,11185)` with contact2 Unsupported/grounded unknown.
Sampled live malloc is1,886,560–1,893,520 bytes; textures3,670,016 bytes;
owned linear71,640–82,560 bytes. These are HLE samples, not hardware memory peaks
or FPS. The genuine top/bottom capture shows the imported player; no clear enemy
animation closeup or native retail cadence acceptance is claimed. See
`validation_01415/azahar-animation-r6-summary.json` and `SCREENSHOTS.json`.
The actual final0143 fallback is separately tested with actor animation OFF.

The r4 CSV has 45 sparse samples from areas1/2. Sampled update cost is 2–1,423 µs,
render/synchronization 9,549–25,316 µs, bottom UI 18–6,942 µs, and 15–18 draw calls.
A sample containing the diagnostic area load takes about 2.044 seconds; its logged
transition work is 2.026 seconds, and initial package loading is 1.320 seconds.
These are HLE timings with logging, not physical-device FPS, worst-case pacing,
or acceptance thresholds. The gameplay accumulator retains its nominal 30 Hz
clock and does not accrue movement debt while area resources load.

| Quantity | r4 evidence |
| --- | --- |
| Retained EBD source payloads | 567,232 bytes across 5 area bindings; all remain resident |
| Spawn catalog | 1,272 cooked bytes; host decoded estimate 5,928 bytes |
| Package host admission | Decoded estimate 864,044 bytes; peak estimate 1,911,892 bytes |
| Package ARM admission | CSV decoded estimate 810,560 bytes; peak estimate 1,898,896 bytes |
| Sampled frontend linear ownership | 71,640–82,560 bytes |
| Uploaded texture ownership | 3,670,016 bytes, 14 pages |
| Sampled malloc live | 1,886,560–1,891,536 bytes |
| Reported free linear/VRAM | 27,926,272–27,937,280 /5,523,456 bytes |
| Reserved application regions | Heap 66,564,096; linear 33,554,432; stack 262,144 bytes |

Region reservation, accounting estimates and sparse allocator samples describe
different quantities; none establishes total peak physical memory or stack high
water. Per-area actor mesh admission is 512 KiB, sidecars 4 MiB each, terrain geometry
4 MiB, decoded material caches 8 MiB and uploaded texture pages 8 MiB. These are policy
caps. GPU/cache resources are released and rebuilt during diagnostic transitions;
outgoing CPU session/actors commit only after target preparation succeeds. Failed
preparation attempts to restore outgoing GPU/cache data. If restoration also
fails, the runtime stops with an error instead of continuing with mixed ownership.
It does not retain both full GPU sets or guarantee allocation restoration.
Actor presentation reload is a separate transaction: old and candidate decoded
actor elements can temporarily coexist up to twice the per-area cap. Failure
preserves old stale meshes/poses, consumes the attempted revision and disables
its pose ticking; it does not roll back logical spawn state or the entire session.
Request0 cache elements count against the decoded actor cap, not total memory.

## 7. Screenshot and emulator paths

All actual images, movies and emulator state stay outside the code ZIP. Selected text-only logs and CSV evidence are retained in source; all screenshots remain external. Latest grounded-fixture scene captures remain occluded and do not pass visibility acceptance.
The screenshot manifest is `validation_01415/SCREENSHOTS.json` when finalized.
The current r4 run root is
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261004-worldalive/azahar-01415-gameplay-r4`.

Verified paths include:

- `captures/after-diagnostic-transition.png`
- `launch.json`
- `sdmc/3ds/Legends3DS/boot_01415.log`
- `sdmc/3ds/Legends3DS/hardware_01415_4000086009747.csv`

The complete root-owned capture manifest distinguishes real emulator captures
from synthetic input or scalar test fixtures. Screenshots are not recreated or
replaced by illustrative game images.

## 8. Known bugs and unported behavior

- The static cooked selectors at authored class20/03 positions can choose
  missing default contact lists. Matching live ST00 records choose alternates
  after raw6 changes from0 to1. The exact writer/domain remains UNKNOWN, so the
  native route still lacks that reconstructed state. Diagnostic grounded samples
  prove only the admitted subset; missing lists are not borrowed.
- Complete slopes, steps, ledges, seam response, deep-overlap recovery and source
  full-tick contact ordering are unresolved. Degenerate interval inputs reject.
- Three meshes can spawn, but native enemy AI, actual per-instance HP selection,
  player/enemy damage packet delivery, attacks, projectile collision and complete
  defeat/pickup/persistence lifecycle are absent. Source health/hit routines were
  probed in isolation; that is not an encounter.
- Source action0/action1/distance/RNG and sequence kernels do not execute the
  complete native actor callback. Full request selection, later actions,
  controller removed-object flag side effects and generalized families remain
  incomplete. Actor materials/shade are UNKNOWN. Default-off request0 cache
  playback has CANDIDATE cadence and UNKNOWN pause/allocation-first-tick ownership.
- Failed actor presentation reload preserves stale output until a later source
  revision; logical spawn state is not rolled back and the same failed revision
  is not automatically retried. This is bounded failure containment, not full
  session rollback.
- Normal event/message/cutscene/audio/transition execution remains unbound. The
  source event1 final transition branch is proved under a completion premise;
  the native gameplay chain does not execute it. SELECT switches areas for
  diagnostics. EBD directories are not event interpreter bytecode.
- Runtime package replacement, lazy stage/EBD streaming and normal doors remain
  incomplete. Package loading occurs at boot; registered areas remain resident.
- Scripted/dynamic camera, obstruction handling, original top PS1 HUD,
  expression mapping and terrain/camera occlusion remain unresolved. Actor meshes
  use neutral shading. The original0143 fallback remains available.
- Physical Old 3DS pacing, memory peaks, touch acceptance and device stability
  have not been tested. No usage reset was called.

## 9. Playable port or prototype

**Technical prototype with partially playable diagnostics.** Movement, admitted
jump/contact paths and source actor presentation are executable, but the tutorial,
one real completed enemy encounter, the subsequent normal event/message/transition
chain and the first Tron fight are not complete. “World Comes Alive” names this
continuation's target, not an achieved readiness claim. Imported file counts,
source-backed addresses and a successful native build do not imply retail parity.

## 10. Complete code-only ZIP

Complete cumulative code-only source artifact:
`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.15_Source.zip`.
The external `.zip.sha256` sidecar records its digest. Final release verification
checks CRC, every manifest hash, source inventory and payload exclusions. The ZIP
digest cannot be embedded inside the same ZIP without changing that digest.
The final handoff confirms release verification after creating the archive.

The ZIP includes cumulative code, tests, tools, state, hashes and text evidence.
It excludes all owner raw/cooked game payloads, `.l3*` data, `.mmlpkg` files,
original executable/overlay bytes, builds, screenshots, movies, emulator files,
local SD trees and saves. The local owner pack and r6 SD installation are separate
and are not redistributable source artifacts. No protection bypass, unauthorized
save mutation, Git commit or hardware test is claimed.

Historical r4 continuation evidence records movement after diagnostic SELECT
loading in area2 from(-256,-384,11008) to(-17,-384,11168), with unsupported
contact. Later actor reload code now retains old stale presentation on failure,
without same-revision retries or logical-source rollback. Final r6 boots, admits the cache and continues after a diagnostic switch;
full actor playback visibility and physical performance remain unaccepted. The World Comes Alive gameplay chain remains unfinished.

Final fallback note: r6 boot log reaches runtime ready renderer0 with actor animationOFF. Final fallback screenshot failed because the computer-use service could not start; no new fallback image is claimed.

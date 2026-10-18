# 0.0.14.36: source state handlers and coordinate ownership

World Comes Alive remains incomplete. This pass connects original class40
states 0–12, their animation request banks and exact fixed-position producer,
then admits the final player position halfwords under the tail resource guard.
It also adds the phase5 subtype131 scalar sweep and deferred-release consumer.
The separate Buster source pool, scalar spawn producer and target lease owner
are implemented internally; gameplay firing is not wired.

`PHYSICS_1TO1=false`. Native shadow output, source audio delivery and timed UV
cues remain pending. Full firing/muzzle selection, projectile callbacks,
enemy damage/defeat, complete source scheduling, dialogue/transition parity,
retail parity and physical Old 3DS validation remain unproved. Emulator .36
testing is in progress separately; this report records source and build
evidence and makes no new emulator acceptance claim.

## Verified source behavior

| Owner | Verified result | Evidence |
|---|---|---|
| Class40 states | 145 native comparisons against 1,216,398 unchanged original instructions; 59 branch cases and 86 consecutive state6 calls; 1,018 atomic guards; no hot allocation. State6 retains all 47 fixed-X additions. | [State proof](source-scene-actor40-state-proof.json), [scope](SOURCE_SCENE_ACTOR40_STATES.md) |
| Final coordinate tail | 48 shadow, 220 cue and 14 authored material cases preserve all 1,444 actor bytes; 50,913 original instructions; seven final mirror CRC cases; 180 native guards and actual owner EBD admission. Shadow/audio/UV presentation remains pending. | [Tail proof](source-scene-actor40-tail-proof.json), [scope](SOURCE_SCENE_ACTOR40_TAIL.md) |
| Phase5 subtype131 | 76 original known-lane cases and 13 sweeps; 22,698 unchanged instructions. Unknown lanes or active unported callbacks hold the whole sweep; release masks commit atomically. | [Effect proof](effect131-source-proof.json), [scope](PHASE5_EFFECT131.md) |
| Buster pool and leases | 258 original scalar producer cases, 927,450 instructions; 498 allocation and 260 release lease cases. Internal pool/lease ownership is verified; this does not establish firing or damage. | [Spawn proof](source-buster-spawn-proof.json), [lease proof](source-hit-lease-proof.json), [scope](SOURCE_BUSTER_SPAWN.md) |

These original-instruction probes use no helper hooks or instruction
replacements. Owner EXE, overlay, EBD, transform snapshots and traces stay
private. Public fixtures contain authored scalar cases and derived results.
Synthetic consecutive callbacks do not establish native or retail cadence.
The floor maximum branch has one observed outcome; its remaining write
destinations are covered by the complete static helper audit, as documented
in the tail report. REA's earlier PS-X EXE adapter was unsupported; no
successful REA analysis is claimed.

Native area loading prepares request0 plus 19 additional original request
banks and the mesh-index20/21 variant. The actual owner decode reports
418,416 bytes against a 512 KiB area cap in
`work/pass-20261013-worldalive-01436/actor-bank-memory-with-variant.txt`.
This is a host decoded-cache observation, not a measured device memory peak.

## Build evidence

The saved result is
`work/pass-20261013-worldalive-01436/native-r1/build-result.json`.
All recorded configure/build/test/package/probe commands return zero.
`native-r1/logs/host-tests.log` records **131/131 tests passed**, zero failures,
in 4.82 seconds. `native-r1/logs/native-build.log` records the actual linked
`.3dsx` build and contains zero warning diagnostic lines.

The copied native Makefile selects devkitARM `3ds_rules`, ARMv6K/MPCore,
hard float, `-O2`, `-mword-relocations`, section garbage collection,
C++17, Citro3D and libctru. Selected new source owners compile with
`-Werror -fstack-usage`. The build log does not print compiler/package
versions. A separate read-only check after the build reports GCC **16.1.0**,
devkitARM **r68-1**, libctru **2.7.0-1**, Citro3D **1.7.1-2** using
`/opt/devkitpro/devkitARM/bin/arm-none-eabi-g++ --version` and
`pacman -Q devkitARM libctru citro3d`.

Built binary:
`work/pass-20261013-worldalive-01436/native-r1/build/Legends3DS_01436_WorldComesAlive.3dsx`,
827,580 bytes, SHA-256
`da9d074346a4e97c0d2105eb254b86b49481450953a1153d1268bdc5713b9b3d`.
The private SD layout is `native-r1/sd_out`, relative to the same pass path.
The build packages the already cooked .34 owner pack; it invokes no asset
cooker/extractor. Hardware remains `PENDING` in the saved build result.

## Paths and recorded commands

The absolute workspace paths below identify the existing evidence. Original
oracle output directories already exist; reruns require a fresh `--out`
directory because the probes refuse to overwrite their evidence.

```sh
project='/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca'
source="$project/work/Legends3DS_0.0.14.36_WorldComesAlive"
pass="$project/work/pass-20261013-worldalive-01436"
owners="$project/work/pass-20261012-worldalive-01434/scene-actor40-init-oracle-r1"
exe="$project/owner-data/extracted/ROCK_NEO.EXE"

python3 "$source/docs/research_01436/source_scene_actor40_state_probe.py" --exe "$exe" --overlay "$owners/owner-overlay.bin" --ebd "$owners/owner-ebd.bin" --out "$pass/class40-state-oracle-r1"
python3 "$source/docs/research_01436/source_scene_actor40_tail_probe.py" --exe "$exe" --overlay "$owners/owner-overlay.bin" --ebd "$owners/owner-ebd.bin" --out "$pass/actor40-tail-oracle-r4"
python3 "$source/docs/research_01436/source_effect_phase5_probe.py" --exe "$exe" --out "$pass/effect131-oracle-r1"
python3 "$source/docs/research_01436/source_hit_proxy_lease_probe.py" --exe "$exe" --out "$pass/source-hit-lease-r1"
python3 "$source/docs/research_01436/source_buster_spawn_probe.py" --exe "$exe" --out "$pass/source-buster-spawn-r2"

cmake -S "$source" -B "$pass/host" -DCMAKE_BUILD_TYPE=Release
cmake --build "$pass/host" --parallel 4
ctest --test-dir "$pass/host" --output-on-failure
make -C "$pass/native-r1/native-source/platform/3ds" -j4
```

The exact recorded build argv, local area-package inputs and output paths are
preserved in `native-r1/build-result.json`; its corresponding logs are under
`native-r1/logs`. The build reads the frozen copy under
`native-r1/native-source`. The copied source manifest is
`native-r1/native-source-manifest.json`. Those private artifacts are not a
source-release manifest.

## Changed source summary against .35

| Files | Change |
|---|---|
| `source_scene_actor40_update.h/.cpp`, `source_scene_actor40_tail.h/.cpp` | New original state/prefix transactions and guarded final mirror receipt. |
| `source_scene_actor40.h/.cpp`, `source_scene_actor40_pose.h/.cpp` | Prepared request bank, alternate mesh ownership and exact current-request pose consumption. |
| `runtime_session.h/.cpp`, `platform/3ds/source/main.cpp` | Connect scene state/effect consumers, final halfword application, source audio request receipts, load-time banks and explicit pending/chronology trace metadata. Audio playback delivery remains pending. |
| `source_effects.h/.cpp`, `source_event_phase5.h/.cpp` | Narrow known-lane subtype131 sweep and deferred-release mask ownership. |
| `child_runtime.h/.cpp`, `source_hit_proxy.h/.cpp` | Separate internal Buster pool, scalar producer and source target leases; no gameplay firing caller. |
| `CMakeLists.txt`, `platform/3ds/Makefile`, `checkpoint_version.h` | Register new tests/native owners and the .36 binary identity. |
| `test_source_scene_actor40_update.cpp`, `test_source_scene_actor40_tail.cpp`, updated effect/child/hit-proxy tests, .36 fixtures and research probes | Differential/atomic guard coverage and resumable proof data. |

The table summarizes verified module differences, not an exhaustive release
manifest. Original models, animations and owner-local material data remain the
source inputs. Source packaging excludes owner assets, cooked payloads and
private original snapshots. Release/readiness manifests and emulator results
are maintained by the main pass separately.

## Current emulator observation and handoff

Azahar2126.0 with Old3DS selected and CPU100% ran the actual final .36 binary. Native trace exercised all13 class40 states and20 imported pose requests. The scene reaches a strict RNG context hold. No natural tutorial progression or completed combat encounter is claimed. Current observation and memory sample are in `docs/validation_01436/emulator-observation.json`; the missing original subtype2 callback is described in `RNG_NEXT_BLOCKER.md`.

The native actor cache reports390004 decoded-capacity bytes under512KiB. Sparse telemetry is emulator evidence only, not a physical performance or memory peak result. No new .36 screenshots were captured: macOS capture reported locked twice. Earlier .35 captures remain historical. Top/bottom UI and safe0143 path are preserved but not newly visually accepted in .36.

This is a technical prototype. Normal retail physics, original firing and enemy encounter, complete event/message/transition chain, natural progression and physical Old3DS acceptance remain incomplete. The separately validated private integer muzzle proposal is retained for the next continuation; it is not wired into this binary.

Release exclusions: owner ROM/CHD/BIN/CUE/EXE, retail save data, Capcom models/textures/audio/video, cooked payloads, dumped DSP, build objects/binaries and reproducible caches. Code, authored fixtures, source evidence summaries and validation logs are included. The real compiled .3dsx is delivered separately; the private SD layout stays local.

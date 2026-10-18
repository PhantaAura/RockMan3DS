# Current handoff — 0.0.14.17

Current42/42hostPASS; fullone-commandnative/SDpipelinePASS; OldmodelAzahar
boot/movement/jump/fall/landingexercised. Canonicalbinary is byte-identical to
that capture. PhysicalOld3DS and fullWorldComesAlive remain pending.
Source=/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.17_WorldComesAlive
Build=/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/install-r1/build/Legends3DS_01417_WorldComesAlive.3dsx
SD=/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/install-r1/sd_out
Next: source-owned enemycallback/detection/wall/sequence/packet integration;
then genuinecombat→EBD/message→normaltransition, without invented semantics.

The following recovery notes record earlier checkpoints; the current status above
and finalREPORT_01417.md supersede their validation-pending statements.

# 0.0.14.17 resume — host verified, native pending

Active source: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.17_WorldComesAlive. No Git branch/commit supplied.
Parent: verified 0.0.14.16 source ZIP, 838 files / 837 hashes; originals unchanged.

Implemented: optional CRC-bound L3MI map initializer, source-derived HED admission,
section35 package/runtime wiring and local cooked wrapper, allocation-free apply,
matching three imported enemy floor samples, session dialogue lifetime and save/
transition/cutscene transactions, separate diagnostic position recovery.

New source includes map_initialization.h/.cpp, l3cook_map_init.cpp and initialization,
package, actor-floor and runtime-transaction tests. Cumulative parent systems preserved.
0143 shader and independent feature gates retained. Actor render cfg remains named
actor_render_01416.cfg for compatibility; the binary/log version is01417.

Completed checks: inherited38/38 and current strict42/42CTest passed. New initializer
strict focused tests,24 Python wrapper tests and focused actor-floor checks passed.
Source probes prove994 owner IDX writes,137 synthetic writer cases, stage0 variant
retention, and three original enemy floors. No complete native enemy AI/parity claim.

Pending: actual01417 native build, Old-model Azahar exercise,
original live writer observation, final current report/readiness/evidence/source ZIP.
Host build currently at ../pass-20261005-worldalive-01417/baseline; command:
cmake --build ../pass-20261005-worldalive-01417/baseline --parallel4
ctest --test-dir ../pass-20261005-worldalive-01417/baseline --output-on-failure

Local-only pack: ../pass-20261005-worldalive-01417/pack-runtime (108 old outputs
plus st00.l3init). No retail/cooked data belongs in the source release.
Build: python3 -B tools/build_hardware_test.py --pack ABSOLUTE_PACK --out NEW_OUT

Known gaps: PHYSICS_1TO1=false; full AI needs BIOS RNG, detection/contact/sequence/
packet ownership; lock-on/combat and ordinary EBD/message/transition chain incomplete;
HUD, later variants, streaming and physical Old3DS measurements remain pending.
No usage reset redeemed. Next step: finish integrated tests and compile current3DS source.

Native source frozen at ../pass-20261005-worldalive-01417/native-fast-r1/native-source.
The full build pipeline also runs at install-r1; logs and results remain outside source.
Next: verify actual native binary/source hashes, exercise Azahar Oldmodel, finalize report.

UPDATE: Current42/42hostPASS, nativeARMv6K compilePASS/156matchinghashes, Oldmodel
Azaharboot/movement/jump/fall/landexercised. NativeSHA=a689f20e2d26c6152806c67db260fe10f3c57e2e67cc39203102be7eca6bf771
Capture=/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261005-worldalive-01417/azahar-01417-r1/captures/native-current-top-bottom.png
Next concrete step: bind source-owned enemy callback inputs/detection/corrected wall
transactions and animation requests; preserve UNKNOWN BIOSrandom/packet ownership
until measured. Normal combat/EBD/message/transition chain still incomplete.

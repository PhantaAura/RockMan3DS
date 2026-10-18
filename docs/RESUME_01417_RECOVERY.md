# 0.0.14.17 recovery — validation pending

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

Completed checks: inherited strict host build and38/38CTest passed. New initializer
strict focused tests,24 Python wrapper tests and focused actor-floor checks passed.
Source probes prove994 owner IDX writes,137 synthetic writer cases, stage0 variant
retention, and three original enemy floors. No complete native enemy AI/parity claim.

Pending: all integrated tests, actual01417 native build, Old-model Azahar exercise,
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

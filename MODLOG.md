# 0.0.14.27 texture decoder repair

Universal Modder reverse-engineering workflow applied: original loader/data are the specification, an authored multi-tile regression checks the format, and acceptance requires the actual native build in Azahar.

Observed defect: selected H01 geometry was present, yet original hair faces became invisible and face/armor details were scrambled. The original type1 loader uploads each 2048-byte sector as a 32-VRAM-word × 32-row rectangle. The importer treated the whole payload as a row-major image. Correct this once in the shared decoder before cooking indexed textures and material caches. Keep original assets and all geometry/UVs/poses intact.

Source profile: owner ROCK_NEO.EXE SHA256 10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0. Loader dispatch80010284 → 8001BD98; sector stage8001025C[2] → 8001C350. Scalar/source evidence is saved in docs/research_01427; raw binary/assets stay in the separate owner-local work area and never enter source ZIPs.

The existing incremental tutorial cache key hashes cooker binaries, so rebuilt cookers invalidate cached texture output. Previously cooked row-major type1 textures/material caches must be recooked; changing runtime code alone cannot repair them. The safe0143 geometry path and render gates remain independent. V inversion is unchanged; the earlier V-direct experiment was superseded before gameplay acceptance.

Completed:100/100host checks,54affected texture/cache cooks and actualARM build pass; zero finalwarnings.232compiled source files match. Original spiky hair and coherent face/armor are visible in genuine enhanced rear/movement and separate front diagnostic captures; safe0143boots independently. Initialnative missingalgorithm include fixed explicitly. Fullcommands/failures/results in docs/validation_01427. No newhardware/fullretail-parity claim; WorldComesAlive remainsunfinished.


# 0.0.14.28 World Comes Alive continuation

Parent01427 preserved. Initial100/100host checks pass with zero warnings. Original80017E94 fade callback and80100EFC scripted-scene constructor are under instruction-backed investigation; native integration/testing pending. PluginKB CLI requires absent PyYAML; direct local search has no game-specific note. Primary decomp pinned-page/address queries unavailable in this pass, so the owner executable remains the oracle. No external code, retail payload or source instruction listing enters source releases.

Completed01428 source/nativevalidation:104/104host,actualARM0warnings. Original fade/phase5consumer wired; original instructionproofs retained. Emulatorblackdisplay andphase2hold remainunresolved; no newcallback/constructor/gameplayvisualclaim. Current report supersedesin-progressnotes. Noreset.


# 0.0.14.29 ownership and source scene continuation

Universal Modder reverse-engineering/game-automation workflows applied. The retained EBD now admits a bounded witness through actual payload SHA256, neutral directory identity and first-match resource joins. Active replacement/unload invalidates ownership, including shared storage held by a registered area. Source phase6 callback, area fog-field owner, cold retained scene vectors, camera commands and timeline are being wired using unchanged original-instruction oracles. No warm-state zero fallback, invented FOV, replacement asset or completed scene claim. Original01428 fullCSV reaches event phase6 and fade terminates frame77; CD queue phase2 is separate and earlier captures were outside the associated gameplay run.

Native-r1 failed GCC16 warnings-as-errors on a one-line loop. Explicit braces fix the format; no warning suppression. Final validation is pending.

## 0.0.14.46

Source review found outgoing scene CPU-resource retirement incorrectly nested under recorded-phase scheduling. Native source change moves retirement after committed area replacement and guards old cleanup/view calls by resource residency. Existing GPU terrain rebuilding already releases scene textures. Original gameplay transition-status/camera owners remain unknown; no guessed completion or camera clamp. See docs/validation_01446/IMPLEMENTATION.md and final runtime observation.

## 0.0.14.47

Recovered the original `0x80015840` state-1 completion tail and added a guarded native
acknowledgment after area, spawn and gameplay-camera commit. Old-model Azahar clears
transition status and resumes movement/collision in Area 1. Host 139/139 and the actual
Old 3DS native build pass. Candidate RNG/recorded clock, the first natural enemy encounter,
full combat, broader events, cross-package progression and physical hardware remain open.

## 0.0.14.48

Audited the owner-supplied `ROCK_NEO.EXE.1.c` Ghidra export as an incomplete
external research reference: 607 address-named functions, no dynamic `0x8010`
overlay bodies, and no claim that it is buildable/original source. It revealed
the exact `0x80035508` → `0x80034EB8` → `0x80035724` player targeting/Buster
chain. Added a bounded native selector/producer bridge, an owner-local neutral
target-window cooker, malformed-profile and allocation regressions, and an
unchanged-original instruction probe. Six broad-lane cases pass across 1,648
original instructions and 12 RTPS operations. Strict-lane retail values remain
owner-local. Live camera/projection, muzzle/counter/firing ownership, projectile
motion, enemy hit/damage/reaction/defeat and full WCA completion remain open.

## 0.0.14.49

Audited `ROCK_NEO.EXE.2.c` as external research evidence and traced
`0x800325D4` → `0x800331A0` → `0x80035724`. Added a bounded fixed-point primary
player-pose evaluator, source muzzle binding for part 4/part 7, the strict
neutral `L3BP/1` owner-local Buster profile, fresh-lane counter initialization,
and the direct `0x80040E9C` action 0–3 fire-state writes. Unsupported alternate
limb banks and unresolved action 4–7 helper outputs hold explicitly. The actual
owner-data cook, 143 host tests, and ARMv6K build pass. Projectile motion and
enemy damage/reaction/defeat remain open; WCA is not complete.

## 0.0.14.50

Recovered the ordinary subtype-`0x80` Buster callback states from the supplied
decompiler export and original executable evidence. The existing fixed shot pool,
hit-proxy bank, imported class-20/03 actor, damage transaction, reaction path,
effect pool and lease ownership now form one native chain. A normal shot performs
the original 11-frame callback setup, registers the two-point `0x28000008`
producer, delivers eight damage, creates effect `0x84`, clears owner busy state
and releases on the terminal callback without heap allocation. The opt-in native
source-actor path invokes the same implementation. Unknown player endpoint
writers, map bounds, exact fire cadence and full defeat lifecycle remain
explicitly held or candidate. Host and ARMv6K builds pass; no 0.0.14.50 hardware
claim is made and WCA remains incomplete.

## 0.0.14.51

Recovered the class-20/subtype-03 zero-HP tail and its reward wrapper boundary.
L3CB version 4 retains the exact reward descriptor, and the existing actor,
proxy, effect, audio and child runtimes now commit defeat atomically. Two Buster
hits exercise 16→8→0 HP, reaction, effect `0x80/0x41`, sound `0xA1`, actor and
proxy cleanup, and one bounded reward handoff; later ticks cannot duplicate the
transaction. Exact subtype-`0x87` RNG expansion and pickup collection remain
held. Host 144/144, owner-local package/replay validation and ARMv6K build pass;
physical 0.0.14.51 validation is pending.

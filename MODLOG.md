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

## 0.0.15.0 (M1)

Source-backed combat becomes the default on the enhanced renderer (gate profile,
single combat owner, fire admission, diagnostic Buster ribbon) and the
one-command tutorial pack gains the exact combat sidecars. Git introduced.
Host 145/145, sanitizer subset and native type-check pass in the cloud session;
devkitARM link and Azahar test pending on the Mac.

## 0.0.15.1 (M2 partial — map edge)

Root cause of the Area 0/1 escape: the default boot spawned inside a solid
Area 0 block; untranslated contact responses then accepted free motion; wall
pushes did not restart the candidate scan (corner landings sank through the
floor); the fall rule required a prior grounded state. Fixed with the source
handoff spawn (0x80101274), the 0x8004B6AC scan restart, the 0x8004B414
failure contract for untranslated responses, and the 0x8003C4EC floor rule.
Owner-local audit: 0 escapes/0 falls (was 191/182). Host 146/146; native
type-check of all 113 sources; devkitARM link and Azahar pending on the Mac.

## 0.0.15.2 (M2b — diagonal walls)

Translated the four original diagonal classifiers and the response dispatcher
paths through `0x8004F8BC`, `0x8004FBE8`, and `0x8004FF14`. Native contact now
projects penetration onto angled walls, preserves tangential movement, and
uses the source direction-bit rules at straight/diagonal corners. Four direct
executable vectors and deterministic sliding tests pass. The owner-local Area
1 audit reports 0 unsupported frames, 0 escapes, and 0 falls in 600,000 frames;
deep-overlap recovery `0x80050400` and broader measured retail traces remain
unresolved. `PHYSICS_1TO1=false`.

## 0.0.15.3 (M2c — deep-overlap recovery)

Ported the original deep-overlap recovery `0x80050400`, verified from the R3000
instructions rather than the Ghidra output alone. It moves the player to the
nearest face, with the original tie-breaks and blocked-axis rules, and shifts
the pre-move pose by the same amount. Wired it for code 1 and for airborne-upward
code 3, which was previously held. The same pass ported the `0x8004B6AC`
correction history (`0x1F800100..10A`). At each restart the previous response's
correction is undone unless the next response clears it, which changes the
result at corners. Deterministic vectors and a corner regression pass. The Area 1
audit is unchanged at 0 unsupported, 0 escapes and 0 falls. Across Areas 1–4,
0.0.15.2 held 1,016,352 embedded test poses as unsupported. Of these, 622,988
now recover through `0x80050400` and 389,292 reach the original 9-pass rollback.
4,072 (Area 2) still meet unported primitives. No PS1 trace comparison yet.
`PHYSICS_1TO1=false`.

## 0.0.15.4 (M2c — Area 1 boot)

The default boot now checks the `0x80101274` Area 1 handoff before the
terrain-XYZ inspection entry. Before this, that default-on renderer branch
pre-empted the handoff, so 0.0.15.0–0.0.15.3 booted into the Area 0
inspection camera. In Azahar's Old 3DS mode the build now spawns in Area 1 at
(2822,-1,-4833). Host suite 147/147; the native build passes.

## 0.0.15.5 (M2 — Oct 19 playtest regression)

Root cause of the Oct 19 Azahar fall: the generalized boot package carries no
map-initialization section, so the 3DS build ran Area 1 without the original
994 startup rewrites that the host tests apply. The floor in front of the first
door was missing; Mega Man sank through the door slab and left the grid. The
package path now applies the sidecar `st00.l3init`, bound to the package's
map/render CRCs. The recorded-input replay (`tools/l3door_replay`) fails
without the fix and passes with it. Host suite 147/147, Area 1 audit clean,
native build passes. Interactive Azahar check blocked by a locked screen.

## 0.0.15.6 (M2 — first Area 1 door)

Ported the player door event from the original instructions: event-primitive
query `0x800514FC`, gate `0x80051970`, `0x80050C2C` case 3 and lock check
`0x800519E4`, writing the `0x800C3558..71` request. The native commit is
immediate (CANDIDATE) in place of the unported event-0 cinematic. It applies the
loader spawn `0x80015EE8`, then the event-0 arrival `0x80048898`: Area 2 at
(−256,−2,6208), heading 2048. 3DS A sends Circle. `tools/l3door_replay` phase 2
and the new `source_door_event` test pass; host suite 148/148. Mega Man is left
in front of the closed Area 2 door until event 0 is ported.

## 0.0.15.7 (M2 — event 0 door sequence)

Ported event 0 (`0x800485E4`) from the original instructions: phases 0–2, the
14 phase-1 substates, the forced walk `0x80048A9C` (counter from `0x8001F070`,
limits 37 and `0x8008AA04`), player states 3 and 0, the `0x8003C07C` input
selection, and the ST00 door object (states 0–5) with its IDX descriptor swap.
The load is offered at substate 6; the commit runs substates 6–8. On the host
replay Mega Man walks through the Area 1 door and, after the load, through the
Area 2 door to (−256,−1,7104). CANDIDATE: lazy door-object spawn, frame order,
one frame per load substate. Host 148/148, native build passes.

## 0.0.15.8 (M2 — door terrain patch)

`RuntimeSession` exposes an IDX descriptor generation; the 3DS renderer
re-plans on change, collapses the swapped identity's baked vertices in place
and draws the live descriptor from an overlay (no area rebuild). Tracked only
for word0-bit-0x1000 descriptors. Owner-local data check: the ST00 Area 1/2
door descriptors (32→28, 33→30) share their render streams, so open and closed
draw identically and the doors still look closed; only collision changes. The
door object's draw and rotation `0x801126DC` stay UNKNOWN. Host 149/149
(new `terrain_descriptor_patch`), replay 2 hides / 2 restores, native build passes.

## 0.0.15.9 (WCA — original gameplay camera)

Ported the original follow camera: init `0x80038648`/`0x800397E8`, the ordinary
`0x80038894` update, yaw helpers `0x8003A218`/`0x8003A154`, the `0x80037FB4`
eye, and the `0x80049C54` eye-in-geometry test. The 3DS view uses it whenever
gameplay owns the camera, replacing the native rigid follow. New deterministic
camera test; host 150/150; Area 1 audit clean (distance 512..768, 0 unsupported
eye tests); door replay PASS; native build PASS. Lock-on stays blocked on the
matrix half of `0x80037FB4`.

## 0.0.15.10

The matrix half of `0x80037FB4` is ported and matches the unchanged R3000 code
on 640 authored states. The camera's `rsin`/`rcos` now read the `0x80092348`
quarter table (L3RF v2); 0.0.15.9 had used the packed RotMatrix table. The 3DS
loop binds the InitGeom + `0x80011F48` projection and runs lock-on `0x80034EB8`
with the source camera and the L3TG v2 window/mode. Some inputs are CANDIDATE;
see docs/research_01510.

## 0.0.15.11

Azahar 0.0.15.10 saw no Buster shot. The host replay shows it was spawned and
ran its 11 frames. The diagnostic ribbon drew shot +2C → +14, which
`0x80043708..24` makes equal right after `0x8003430C` (R3000-verified), so it
was empty since 0.0.15.0. `ChildRuntime` now records the registered segment and
the ribbon draws it. CSV `projectiles` counts the retired candidate projectile;
new traces `live=` and `# source_buster_draw` report source shots. Tests:
`test_child_runtime` segment case, `l3door_replay` phase 4.

## 0.0.15.12 (WCA — enemy reward pickups)

Ported the original reward lifecycle: wrapper `0x80047228`, expansion
`0x80046B34` (cooked L3RW tables, 7 original rand sites on the shared RNG), the
subtype-0x87 pickup states (`0x8004602C`: flight/bounce, rest, blink, despawn,
collect) and collection `0x80046A0C`, which pays money `0x800C1B2C` and energy
to HP. The 3DS loop expands queued defeat rewards and ticks the pickups. They
draw as a diagnostic marker until the stage model resource is ported. New
deterministic test; host 152/152; native build PASS, 0 warnings.


# 0.0.15.13 enemy actions 5/6/10

Owner Azahar log of 0.0.15.12: enemies held at action 10 (UnknownAction) once
close, so their proxies never refreshed and Buster impacts stayed 0. Ported
ST00 0x80103DFC (lunge), 0x80103554 (knockback) and 0x8010324C (damaged bounce,
then HP 0 + flags 0x41 into the defeat tail). Host 153/153, native PASS.


# 0.0.15.14 Mega Buster shot subtype 0x81

The port fired weapon lane +170 (subtype 0x80, a foot capsule with a CANDIDATE
forward ray), so no shot could hit. A pad-driven PCSX-Redux probe measured
the original: Square fires +171 = subtype 0x81 from the hand. Ported
0x800437F8/0x80043A8C/0x80043E78/0x80043E00 and effect 0x82. Added the opt-in
diagnostic_autoplay.txt for unattended Azahar runs. Host 154/154, native PASS,
Azahar scripted combat/reward/door loop PASS by trace.


# 0.0.15.15 tutorial message triggers (E0/06) and message box

Ported the ST00 E0/06 controller 0x80114424 (walker 0x80114504, handlers
0x8011CB24 types 0-15) with owner-local L3TR lists, a tutorial dialogue mode
(inline controls, CALL/MULTI/CLOSE) and a bottom-screen message box with
decoded glyphs. Azahar: the first radio call and the type-2 message match the
original order.


# 0.0.15.16 subtype-0x1F pop-out Reaverbot

Ported ST00 callback 0x801062EC: init 0x8010648C, wait 0x801066D8, lunge
0x8010673C with the +384 ScaleMatrix Z scale, hit 0x80106800, and the rewrite
into subtype 3 with parameter 0x40. Also added RNG RA 0x80106378 and L3CB v5
(record byte +0x0B). Azahar: three pop-outs trigger and hand off by trace; the
byte-0x0B fix is host-only.

Left-right mirror: the gameplay LookAt view over (x, −y, z) vertices was
mirrored against the original GTE camera. Camera-space x is now negated on
that path. test_frontend_handedness: 157/157.

Map face test: ported the original NCLIP rule (families 0x80024F30/0x80025900/
0x80021B14). Back faces are culled, and in mode 2 far back faces are filled
near-black (threshold 522 measured in PCSX-Redux). This fixes the doubled top
screen and the hidden Mega Man at the start. Host 158/158; Azahar visual PASS.

Tutorial route: mapped every trigger and the objects that raise its flags.
Ported trigger type 16 (0x801156C8), the E0/00 defeated flags (0x80111AD4) and
the E0/07 floor switch/gate (0x80115A4C, IDX 119). Azahar: Area 1 messages
0-1-2-3-4-6-7 in order. The subtype-0x19 trap-room fight is the next gap.

Subtype 0x19: ported the trap-room Reaverbot (0x80104864; states, six actions,
selector, hit/stagger/defeat) with L3CB rows and L3DP profile 6. It drops in
when the switch raises 0x607. Host 160/160 plus a real-map check (1200/1200
frames). Azahar run pending (screen locked).

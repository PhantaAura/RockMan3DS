# 0.0.14.45 — World Comes Alive continuation

The opt-in shared candidate RNG now connects the already source-backed effect,
lift and event consumers to one native service. In Old-model Azahar, the gated
scene reached its UV/audio handoff and replaced Area 0 with Area 1 without a
developer room skip. This is a technical prototype improvement; World Comes
Alive and faithful tutorial progression are not finished.

## Source and changes

Exact mutable cumulative source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`. No Git repository, branch or commit.
Parent: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.44_Source.zip` (SHA-256
77454c7b1b804cbe47f7724c75fdc7cc2c8ccd50d632b0d41932c22b37903092).
All 2134 parent manifest entries were checked before editing. Earlier archives
and synced sources remain untouched. CHANGED_FILES.txt lists this pass.

Reused: imported owner-local resources, original scalar leaves/planners,
64-slot ascending effect dispatch, event pipeline, NDSP transport, area staging,
collision, player, renderer and all inherited tests. Added typed candidate RNG
transactions for effects 2/3/4, lift/dust and event D6C, including rollback on late
holds and shared generation/ordinal receipts. Maximum burst is 120 calls for
24 children. No new renderer/layout, replacement actor/scene, unproved physics
constant or per-object random seed was introduced. See RNG_CONTRACT.md.

## Validation

Toolchain: devkitARM r68-1 / GCC 16.1.0 / libctru 2.7.0-1 / Citro3D 1.7.1-2;
ARMv6K/mpcore hardfloat. Actual native build PASS; 904248 bytes,
SHA-256 `ee85a5adf899824b2f4f9cab2237ec4cfc1d01ff262156a9b55c5b621a95fbde`. All 280 staged native input hashes match the cumulative
source. The current native log contains no GCC warnings. Host suite: 139/139
PASS, 5.06 seconds. Focused candidate/dispatch tests also pass, including actual
owner-local positive dispatcher data and 24-child/120-call bursts. Exact
commands and results are in TEST_COMMANDS.md and retained logs.

Desktop validation means host build/tests/probes; a desktop game window was not
tested this pass. Physical 01445 Old 3DS/XL testing is pending. Prior 01443 user
evidence shows a textured player/room followed by collision escape.

Azahar 2126.0 used Old model, CPU 100%, Vulkan and resolution factor 1. Normal
enhanced mode displayed the textured player/room and accepted movement, jump and
fire input. Safe 0143 displayed its preserved untextured subset. All three final
runs exited normally and released package bytes to zero. Owned emulator PIDs
were checked absent after GUI exit.

Candidate scene: shared generated RNG plus the existing measured constructor
clock snapshot, not fresh boot or retail parity. Head UV reset: frame 3082,
10 records. Frame 3085 applied all 254 mixer-prefix calls then paused actual
NDSP. Area 1 replacement completed by frame 3088; destination movement and
grounded samples followed. Both areas live in the same 701943-byte package:
this proves area-resource replacement, not different-package file streaming.
The destination view was wall-obstructed, so camera/spawn presentation remains
open. Original captures, CSV hashes and scope are indexed in
emulator-observation.json; candidate-handoff-trace.txt retains decisive rows.

The extended parent44 trace corrects the earlier short observation: callback
80100738/801007F0 eventually advanced. Normal source boot ultimately needs the
unknown 1F800002 counter origin. The strict recorded replay reaches D6C with
matching actor XYZ/counter but a differing pool bitmap; its context guard was
preserved. Missing offset 255 was an absent-offset sentinel; effect status 10
means NeedsRngReceipts. E0 status 1 is NotScheduled, not a missing-field hold.
Existing prop0 already moved in the baseline. No new live PS1 trace was taken.

## Performance, memory and limits

Candidate area replacement took 1780435 us in Azahar. Sparse Area 1 samples
report 14 draw calls/1680 triangles, 3670016 texture bytes and 71640 owned linear
bytes. Treat these as emulator counters, not physical budgets or measured peaks.
The retained JSON gives ranges for malloc and free linear/VRAM. Loading spikes
and log overhead prevent FPS/parity claims. There are no new hot-loop dynamic
allocations. Fixed RNG/audit storage grew to the supported 120-call bound.
Compiler per-function stack estimates are recorded in stack-r3.json; main is
124192 bytes (parent44 124184), session copy assignment 71056 and effect-34
wrapper 19216, with 262144 reserved. Call-chain high-water remains unknown.

Unresolved:

- Fresh source-scene boot still lacks the original 1F800002 counter origin; candidate scene evidence uses the existing recorded constructor clock.
- Shared RNG scheduling remains CANDIDATE. Whole-world ordering, original clock ownership and retail chronology are unknown. Strict recorded replay still rejects the differing pool bitmap.
- Area 0 to Area 1 replacement was exercised inside one multi-area package; different-package streaming and complete natural tutorial progression are not established.
- Destination camera/spawn presentation was wall-obstructed. Scripted prop selector 3 / handler 801133B4, full scene fidelity and expression ownership remain incomplete.
- Map-edge fall/escape, complete slopes/steps/ledges and original recovery behavior remain unresolved; no invented wall, Y clamp or rescue was added.
- Full enemy attack, live source Buster/lock-on/HP/damage/reaction/defeat and event-driven tutorial continuation remain incomplete.
- Physical 14.45 acceptance, real Old 3DS performance, memory peaks and stack high-water remain pending.

PHYSICS_1TO1=false. Strict replay is not silently converted to candidate mode.
The release does not enable research gates. Source/readiness/evidence statuses
remain conservative; no made-up completion percentages.

## Original screenshots

All images below are genuine 400×480 emulator captures of this exact final
binary. They are kept privately and excluded from the code-only source ZIP.
Mode/scope, hashes and paths are in emulator-observation.json. Some captures
include startup diagnostics; only the settled normal/safe and Area 1 captures
were used for the claims above.

- candidate: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-candidate45-r3/captures/_16.10.26_23.21.58.32.png`.
- candidate: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-candidate45-r3/captures/_16.10.26_23.24.33.295.png`.
- candidate: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-candidate45-r3/captures/_16.10.26_23.24.48.544.png`.
- candidate: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-candidate45-r3/captures/_16.10.26_23.25.21.19.png`.
- normal: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-normal45-r3/captures/_16.10.26_23.27.00.841.png`.
- normal: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-normal45-r3/captures/_16.10.26_23.27.14.431.png`.
- safe: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/azahar-safe45-r3/captures/_16.10.26_23.27.57.783.png`.

## Release

Complete cumulative code-only source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.45_Source.zip`.
App-only SD ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01445_WorldComesAlive_SD.zip`. Direct compiled binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01445_WorldComesAlive.3dsx`.
Adjacent SHA-256 files and the external release receipt verify archives, every
source manifest entry and expected source/renderer/test files. Exclusions:
ROM/CHD/BIN/CUE/executables, all cooked retail payloads/models/textures/audio/video,
DSP, saves, private profiles/captures, reproducible caches and generated binaries
inside source. The app ZIP contains only the compiled app, SMDH and instructions.
No extraction or recooking occurred. No reset was redeemed.

Next concrete work: diagnose the Area 1 camera/spawn presentation against the
original source and retained trace; recover fresh counter ownership, then
continue genuine enemy/combat/event progression. The map-edge fall remains an
independent high-priority hardware regression.

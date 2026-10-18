# Source-owned detector state and its remaining helper boundary

Status: **SOURCE_BACKED_PARTIAL**. This change translates the scalar state
machine at `800364B0`, preserving the original persistent fields and explicit
ownership of inputs. It does not establish retail parity, complete enemy AI,
an attack, or a substituted modern vision model.

`ActorDetectionWork` corresponds to original work+34C: three origin halfwords,
profile selector352, phase353, three vector halfwords354..358, countdown35A,
and signed vertical offset35B. These fields must persist per actor and commit
with the actor's behavior/pose transaction; a missing input leaves all of them
unchanged. The native API does not allocate.

## Profile and rotation

The source reads profile bytes at `800899AC + profile*4`. Profile byte0 seeds
phase0's countdown, byte1 seeds admission and refreshes phase2, byte2 forms
an unsigned range cutoff shifted left20. Byte3 is not read by this detector.
Profile4 is selected for the previously proved fresh class20/subtype03 path;
its owner bytes belong in a local neutral package, never distributed source.
An absent profile or table is a hold, not a guessed constant.

The phase0/2 test reads only mask10 from `80030628`. This is the sign of MAC3
local Z under the original pure heading matrix, rather than an invented
cosine-angle threshold. Original signed16 target-minus-origin differences wrap
before multiplication. Rotation angle is `2048 - signed16(heading56)`; the
original CPU matrix rotation selects the owner Q12 sine/cosine record with the
original negative-angle branch. Two signed16 products fit within the GTE's
44-bit MAC range. The original MVMVA command shifts by12 and stores MAC3; its
sign equals the sign of the unshifted sum. IR saturation and unused FLAG stores
do not change this detector's mask10 decision.

## State phases

An unsigned range rejection clears phase353 and returns false. The other
fields remain unchanged.

Phase0 (also the source default branch for other phase values) tests local Z.
A rejected orientation leaves the block unchanged. Admission stores the
actor's raw origin, applies signed offset35B times16 to its Y halfword, stores
wrapped player-minus-origin vector halfwords, seeds byte0 countdown, enters
phase1 and still returns false.

Phase1 checks each signed player-origin difference against the original
inclusive `[-26750,26750]` bound. It then requires actual helper outputs:
normalization `8007A694`, vector interpolation `80033F04`, and two wall segment
queries `80049C54`. These are **not supplied by an approximate native helper**.
The bounded scalar reducer can consume explicitly prepared source-owned values;
its fixture labels those outputs as authored premises. It stores the supplied
interpolated vector, advances origin by `(signed16(vector)*25)>>10`, rejects
nonzero segment results, decrements the unsigned byte countdown, and tests
wrapped halfword player deltas within `[-100,99]`. Admission may request
sound156 when scratch byte `1F800075==2`, changes to phase2, seeds byte1, and
returns whether signed global halfword `800BBD96<1`.

Phase2 resets when signed global `800BBD96>0`; otherwise it writes notification
byte `800BBD84=1` through `8005D4C0`, rechecks local Z, and either refreshes
byte1 or decrements/reset-expired countdown. The result carries notification
and sound intents so a caller can preflight their owners before committing.

**Initial contract boundary:** phase1 preparation is separate from the scalar
reducer. The new actor_detection_math module now translates the original
normalization/interpolation and carries owner lookup data in L3NRv1. The
segment module supplies only proved contact families; unsupported contacts or
missing scratch/audio owners still hold. Supplying an authored phase2 remains
a diagnostic test, never evidence of a retail encounter. See
ACTOR_DETECTION_MATH.md for updated helper proof and exact point-query inputs.

## Persistent global ownership

The actual initialization caller `80013464` invokes `8005C258`. That function
calls the original word-clear routine `800177CC` over an inclusive end pointer
from `800BBD78` through `800BBE4C`; it initializes `800BBD94`, `800BBD96` and
`800BBD84` to zero. This is an original subsystem initialization contract, not
an arbitrary zero chosen to admit enemies. The wider subsystem is not ported.

`8005C2B8` decrements each positive signed16 countdown94/96 once and leaves zero
or negative values unchanged. The original gameplay update `80015DB4` invokes
distance calculation at `80015DDC`, this global update at `80015DE4`, and class20
actor dispatch at `80015DF4`. Native scheduling must preserve this order for
admitted source updates. Full pause/loading/cutscene update cadence remains
unverified.

Setter `8005EBB4` stores a caller's low16 bits into96. The one direct EXE caller
at `8005F130` supplies1800 through a separate dispatch path; this document does
not invent its gameplay/item semantics or assume that path never runs. Other
overlays, indirect paths and save restoration require separate investigation.
Notification helper `8005D4C0` writes byte1; `8005D4D4` writes2 and `8005D4E8`
clears it. Actor-local state should not consume this global as an invented
boolean gameplay flag.

## Evidence and commands

The read-only, hash-gated probe executes the original CPU detector/rotation and
extends the established authored interpreter with only the source pure heading
MVMVA command. All table/profile values serialized in fixtures are authored
scalar contexts, not copied game tables. General GTE commands/flags, sound
consumers and phase1 helper bodies are excluded explicitly.

```
python3 -B docs/research_01419/actor_detection_source_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE --out /new/owner-local/proof-directory
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude \
  src/world/actor_detection.cpp tests/test_actor_detection.cpp -o /tmp/detector-test
/tmp/detector-test tests/fixtures/detection_01419/authored-detection.tsv
```

The first successful proof (r2) contains **2,588 original/native scalar cases**:
558 phase0/default cases,1,737 phase1 cases with authored helper outputs,
249 phase2 cases and44 unsigned range rejections. It executes503,803 original
instructions and711 restricted MVMVA commands. All native scalar comparisons
pass; hot-loop allocations are zero. Separately36 original global countdown
cases and the real source subsystem clear were exercised.

Fixture SHA-256:
`4f7a3cebe07b73aef83cd11cd955c79f7f866a4391964c1ce4231297b11ebba7`.

Authored synthetic tests cover strong holds, coordinate/angle wrapping,
range/countdown boundaries, missing helpers/global/profile, and3,600 no-allocation
global updates. These are bounded source-backed comparisons and not measured
retail-vs-native replay.

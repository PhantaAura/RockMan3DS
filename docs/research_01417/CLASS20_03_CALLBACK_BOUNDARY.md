# Class20/subtype03 callback boundary

Status: **SOURCE_BACKED_PARTIAL**. The original callback and action2 integer
control flow are inspected and tested on authored scalar premises. This is
not a complete native retail AI tick, encounter replay, detection solver or
projectile/damage system.

The read-only owner inputs are the established USA `ROCK_NEO.EXE`, SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`, and
ST00 PROGBIN SHA-256
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
No original instruction listings, code bytes, table payloads or assets are
included in this document or the reproducer.

## Admitted callback order

The existing original dispatcher proof remains authoritative: class20 has
16 slots of 960 bytes, and the callback is admitted when
`(work[0] & work[6] & 1) != 0`. The generic distance store precedes this
dispatcher in the examined gameplay update. Native clock cadence and all
retail pause/allocation ownership are still separate gates.

Callback `8010226C` performs the following operations in order:

1. Copies request byte AC to AD; copies the complete eight-byte raw position
   block 14..1B to 2C..33; copies the complete sixteen-byte fixed position
   block 1C..2B to 34..43; copies angle block 54..5B to 5C..63.
2. Stores 16912 at 15E, calls `8007FE50`, and stores the result's low halfword
   at 32A. Dispatches state byte 08. The selected states are initializer,
   active state1, and release.
3. Compares AD with AC. A changed request calls `800322A8(work, AC, 0)`;
   an unchanged request calls `80032488(work)`.
4. If work byte0 bit2 is set, calls `8002C8CC(work,48,512,0)`.
5. If work byte0 bit1 and flags328 bit4 are set, copies the eight-byte
   position block180..187 to334..33B and calls `800341BC` with the original
   proxy arguments. Its result and packet delivery ownership remain external.
6. Clears the complete packet word33C unconditionally at `801023F4`, then
   returns. The hit consumer itself retains the packet. A bridge must not
   clear it before state1 or deliver the same word again on later callbacks.

The callback does not exclude effects, packet finalization, RNG, or sequence
handling merely because an action happens to be stationary.

## Active state1 and transitions

State1 `80102600` updates flags328 bit10 with `8003648C(work+34C,work)`.
Parameter03 bit20 bypasses the helper and **clears** bit10. It then calls
hit consumer `801041B0` only when flags328 bit2 is set, dispatches action09,
and processes the action's new state.

When flags328 bit1 is set after action execution:

- Action1 restores byte0E to action09.
- Action6 with signed HP6E below51 chooses action5.
- Otherwise detection bit10 chooses action9.
- Otherwise `80103FAC` selects from the supplied sample32A. Its already
  verified scalar arithmetic chooses action2 with turn0/+16/-16 or action0.
- The tail clears both bytes0A and0B with a halfword store, clears parameter03
  bit40, and clears flags328 bit1. Byte0B is relevant; the earlier narrow
  behavior state intentionally has no field for it.

After this transition, signed halfwords35C,35E and332 each decrement when
nonzero, including negative inputs. Flags328 bit40 then admits a separate
defeat timer360. Once the decremented signed timer is negative, the callback
attempts an effect allocation through `80031618`, stores its pointer at368,
writes effect fields when allocation succeeds, frees this actor through
`80031824`, calls `80047228`, and emits sound161. A zero effect allocation
does not prevent actor release. Damage arithmetic and the defeat latch do not
prove this entire lifecycle.

## Exact action2 around external motion/contact inputs

Action2 `80102AE0` has a supported phase0/1 path. Other phases return without
motion or initialization. Entry phase0 sets phase1, request1 and work330=0,
then falls through to phase1. A signed byte A6 equal to1 emits sound176.
The action adds turn66 to heading56 modulo4096 and calls
`8002FEA4(work,52,0,0)`.

The existing original reference-table motion arithmetic supplies this
positive-forward, zero-vertical, zero-strafe path. For owner trig entry at
the new heading the displacement is
`dx_fixed=-sin_q12*52`, `dy_fixed=0`, `dz_fixed=-cos_q12*52`. These are added
with 32-bit wrapping to fixed words1C/20/24. `80030150` computes each raw
short as `(fixed >> 16) + ((fixed >> 15) & 1)` modulo65536. There is no
float scale or invented speed in this calculation.

The action then calls wall wrapper `8004B414`, stores its result at370,
calls floor wrapper `80048E6C`, writes the floor's low short to raw Y16,
and writes the returned word shifted left16 to fixed Y20.

If the absolute signed-short floor change from previous Y2E is33 or more,
it restores all eight bytes of raw position and all sixteen bytes of fixed
position, chooses action1, clears both0A/0B, and saves restore action0E=4
when detection bit10 is set or2 otherwise. The changed heading remains.
Otherwise wall status bitsF00 choose the same action1/restore path without
that complete rollback. Otherwise detection bit10 sets flags328 bit1 so
the state1 tail selects action9 in the same callback.

A contact return word alone is insufficient to implement this operation.
`8004B414` changes raw/fixed position components, velocity fields44/46/48,
and byte74. Positive return bits300/C00/3000 select scratch-corrected X/Z/Y;
negative returns select full or class-dependent prior-position restoration.
Its core `8004B6AC`, scratch bounds, class filter and byte74 must be respected.
The earlier static IDX state reported unsupported candidate data at the three
original area1 spawn positions. The [initializer floor proof](CLASS20_03_FLOOR_INIT.md)
now resolves all three to source floor -1 after source-owned IDX materialization
and conservative exclusion of unrelated diagonal groups. Actor wall response
and floor queries intersecting unsupported families remain separate gates.
A failed floor or wall query cannot be replaced by the current Y or a fabricated
zero wall result and then described as original action2.

The new reproducer executes the full action2 control flow, original fixed
accumulation and original raw conversion while supplying an authored fixed
delta and explicit wall/floor return values. Those cases prove this bounded
interface, including wrapping, the32/33 edge, phase exclusion, heading retention,
rollback and byte0B clearing. They do not prove the excluded GTE/contact inputs.

## Detection and random ownership

`8007FE50` is a BIOS trampoline: it jumps to A0 with function identifier2F.
The executable does not contain the called BIOS RNG implementation, state or
seed. The original callback consumes one sample even in state0 and stationary
actions. A native PRNG or seed chosen for convenience would introduce behavior
not established by these owner files. The source low-four-bit selector is
exact only when supplied an explicitly owned sample. Missing BIOS ownership
must remain explicit.

Detection wrapper `8003648C` passes heading56 to core `800364B0`. Its work
block begins at34C, with profile selector352, phase353, vector halfwords354..358,
countdown35A and signed vertical offset35B. Initialization `80036478` sets the
profile byte, stores offset -96 shifted right4 into35B, and clears phase353.
The profile selector is supplied from the selected combat record's byteF;
the EXE profile at `800899AC + selector*4` is owner data, not a new constant.

The range gate requires unsigned work80 strictly below the profile's byte2
shifted left20. A failure clears phase353 and returns false, without calling
the geometric helpers. Admitted detection is persistent:

- Phase0 calls original rotated-cone helper `80030628`, then records actor
  and player coordinates and moves to phase1 if its mask0x10 is clear.
- Phase1 bounds coordinate differences, normalizes and interpolates a vector,
  advances the recorded point, and calls wall-segment helper `80049C54` twice.
  It decrements the profile countdown and compares the resulting point with
  the player; admission changes phase353 to2.
- Phase2 reads signed global halfword `800BBD96` through `8005EBD4` and may
  reset detection. It stores1 in global byte `800BBD84` through `8005D4C0`,
  rechecks the rotated cone, and retains/resets a profile countdown.

The rotated-cone and normalization/matrix operations use original GTE-related
helpers. A nearby-player distance or generic cosine cone cannot stand in for
this state machine. Only its complete source-owned input path, or an explicitly
external detection result that is labeled partial, can admit a native bridge.

## Attack ownership and remaining actions

The attack child belongs to action4 `80102FC0`, which calls `80104078` after
its phase2 timer expires. Phase0 requests21, phase2 increments the request,
and the child call sets phase3 with timer11. Surrounding action4 phases move
with original forward values -112 and224 or a timer-derived negative value,
then use the same wall/floor helpers and33-unit floor rollback. Its recovery
counter35C comes from the selected combat record byteC.

Attack wrapper `80104078` uses allocator `80031518` and saves the returned
pointer at374 even when null. Nonzero child allocations get byte0=1. Parent
work32E selects two distinct paths: zero selects subtype6, parameter0, a fixed
Y offset of -60, and fields9C=32/A0=16 plus sound178; nonzero selects subtype9,
parameter3 and fixed Y offset -80. Both copy parent's fixed X/Z and heading56
to child heading64, run original raw conversion, then displace the child with
`8002FFC4(...,1760,0,0)`. This produces a source-owned child request; it does
not prove either child callback, child collision, player damage or allocation
capacity/lifetime. The earlier shorthand that describes every child as subtype6
is incomplete for nonzero32E.

Other admitted actions also depend on source-owned inputs:

- Action3 requests2, reads combat-record byteA, uses original timers and
  turn-to-player helper, moves forward768, and routes packet bit02000000,
  floor rollback and contact back into later actions.
- Action5 requests4, derives velocities from owner sin/cos helpers, updates
  fixed positions and reflected contact velocities, and can set health0 plus
  flags328 bits41. The signed health/floor/contact inputs are required.
- Action6 requests6, derives vertical movement from work46 shifted left7,
  and waits for wall/floor contact bit1000.
- Actions7/8 select different owner phase timing tables, move/bounce with
  original contact bits, and return through the common state1 transition.
- Action9 requests1 and turns/moves toward the player. Distance80,
  counters35C/35E, computed turn36C, relative player-facing field36E,
  detection and floor/wall results can choose action4 or10.
- Action10 requests4, reads combat-record byteA, uses timer45 and source
  forward52 motion, and sets35E=45 before returning through state1.

## Practical native boundary

A useful integration now can bind the existing source distance/heading kernels
and an action0/1 transaction to each existing class20/03 actor, retaining its
raw coordinates, fixed coordinates, source heading and all used work bytes.
It can accept source-owned request changes into prepared pose caches. This is
an actor action bridge and must report the exact unsupported action/input;
the existing request0-only pose cache cannot silently service requests1/2/21.

An action2 transaction can additionally reuse the resident owner trig table,
but it must preflight a source wall query that returns corrected position and
fields, and a supported source floor result. Stage queries that report
Unsupported reject the whole transaction. It can then reproduce the original
action2 stores/rollback and pass the transition bit into a verified state1 tail.
Supplying detection/RNG/predicate packets as explicit external inputs preserves
the boundary; labeling fabricated inputs as retail AI does not.

For the **whole** callback, the minimum complete input set includes the BIOS
sample, persistent source detection block/profile and geometry, hit/proxy packet
ownership, complete pose-request preparation, class wall/floor operations,
combat profile residency, allocator/free/effect behavior, and native scheduling
admission. All mutations, packet clear and frame advancement should commit
together after preflight. Missing inputs leave state and presentation unchanged.
This makes native progress observable without inventing combat or physics.

The source-safe reproducer is [class20_03_callback_source_probe.py](class20_03_callback_source_probe.py).
Its owner-local output is intentionally outside the distributed source:
`work/pass-20261005-worldalive-01417/enemy-source/`.

The main probe passed 1,439 cases: 576 action2 cases, 768 state1 transition-tail
cases, 64 whole callback ordering cases, 27 original range-gate rejection
cases and 4 attack child wrapper cases. It interpreted 130,004 instructions,
including the listed synthetic hook stubs. The authored scalar fixture
SHA-256 is
`ca1ba6138cdfeff6ea7c13e6d20349d9d7cb6efa2c0873091e2ff190afbe0fcc`.
Each input/output and the excluded helper premises remain in the owner-local
JSON rather than in the distributed source.

A second probe, [class20_03_packet_lifecycle_source_probe.py](class20_03_packet_lifecycle_source_probe.py),
passed 12 nonlethal hit-delivery cases over 24 consecutive original callbacks
and interpreted 4,896 instructions including the declared return-only hook
stubs. The original `801041B0` hit consumer executes inside `8010226C`.
With flags328 mask0x02 set, the first callback subtracts the admitted damage;
the second sees packet33C=0 and leaves HP unchanged. With mask0x02 absent,
the consumer is skipped and the outer callback still clears the packet. The
callback packet lifecycle is therefore source-proved on these premises,
without a claim of native projectile delivery. Its owner-local evidence
SHA-256 is
`e4b4738899841c3df9f2ea231a6207a13c50ac96f804454fe0944297c386347c`.

```sh
python3 docs/research_01417/class20_03_callback_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261005-worldalive-01417/enemy-source

python3 docs/research_01417/class20_03_packet_lifecycle_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261005-worldalive-01417/enemy-source/class20-03-packet-lifecycle-source-evidence.json
```

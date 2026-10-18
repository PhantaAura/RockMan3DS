# Source Action 4 and distinct contact producer

Status: `SOURCE_BACKED_PARTIAL`. The new reducer implements the bounded body
at source `80102FC0..8010324C`. It preserves signed timer wrap, phase0→1,
phase1→2, phase3→4 fallthrough, animation requests21/22, pre-motion cue177,
the byteC recovery field and all four raw/fixed rollback lanes. It does not
pretend the attack child or common callback is implemented by this body.

The independent phase plan selects exactly forward−112, a signed16 value
from `−decremented_timer*32`, or forward224. Phase2's child call and phase4's
unsigned statistics byteC installation are explicit prerequisites. Missing
motion, wrong forward/heading, missing byteC, unavailable child producer or
unavailable audio leaves all state unchanged and emits no intents.

The new contact preparation runs the existing admitted original motion
accumulator/rounding, wall correction and floor query for those distinct
forward values. It snapshots action, phase, request, heading, timer, variant,
raw/fixed/current and all four previous lanes. Unsupported geometry/cell
boundaries hold. The body does not write work370: unlike actions3/9, original
Action4 ignores the wall return. The outer transaction still commits the
wall wrapper's velocity and byte74 when the whole actor transaction succeeds.

Original phase0 emits cue177 from the position before movement, sets
phase1/request21/timer13/work330=0, and decrements to12 in the same update.
Phase1's negative timer selects phase2/timer16 for variant32E=0 or timer0 for
nonzero, followed by phase2's decrement. A phase2 timer≤0 becomes phase3,
increments the unsigned animation request, sets timer11 and calls80104078.
Phase3's negative expiry becomes phase4/timer6, followed by decrement5 and
forward224. Phase4 negative expiry sets flags328 bit1 and installs unsigned
byteC at35C. A floor difference≥33 in either direction rolls back all four
raw/fixed lanes without changing the action/phase independently.

ByteC ownership is exact: `80103158..80103190` indexes the pointer table at
`80098844` by unsigned subtype, adds signed variant32E×16 and reads unsigned
record+0xC. A selected-table tag is not the actor variant. The new base-record
sidecar may supply byteC only for actor variant0; variant1 requires its own
record. Unknown ownership remains absent.

The source pool API retains32×0x10C bytes and an MSB-first allocation bitmap.
It preserves every allocator-untouched byte; there is no whole-slot clear.
`800318D8` adds a pending-release bit and clears byte0, without making the slot
immediately allocatable. `80031B3C` later clears supplied allocation bits; the
pending-mask reset is outer ownership. Pool identities are source tokens,
never native pointers. The scalar child producer preserves variants6/9,
fixed Y offsets, heading64, fields9C/A0, forward1760 accumulator and cue178.
A full pool yields zero with no child writes or cue. Audio dependency failure
performs no allocation. This producer does not authorize native child dispatch.

The live native bridge deliberately holds at the phase2 child boundary until
child lifecycle, collision, hit packets, resources, effects and renderer
consumers are connected. It must not set a readiness flag merely because the
allocator and initializer pass tests. Host caller tests may supply authored
child results, separately labeled from actual gameplay.

Reproduce from the source root, choosing new private outputs:

```sh
python3 -B docs/research_01421/class20_03_action4_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/private/action4-proof
python3 -B docs/research_01421/actor_action4_contact_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/private/action4-contact-proof
```

Fixtures contain authored scalar premises/results only; no retail instructions,
asset bytes, tables or owner game state are released. Original-instruction
differentials prove this admitted subset, not retail parity or hardware success.

Final verification: 1,568 original body cases matched (768 motion cases,128
child-call boundaries,224 entry cues);135 allocator/initializer cases matched
the entire0x10C slot;32 original pending-release/batch-cleanup cases matched.
The body/pool proof executes140,405 original instructions. The distinct
contact proof executes1,078,288 instructions over486 authored contexts:
430 native-admitted results match all wall/floor/raw/fixed/velocity/state74
fields,56 unsupported contexts hold, and4 admitted contacts apply correction.
640-entry body, scalar producer and contact loops allocate no heap memory.
Standalone Clang C++17 builds use `-Wall -Wextra -Werror` and passed.

The `−32768` timer premise is included across all body phases. Source Action4
reads timerLHU, decrements, storesSH, then performs SLL16 (phase3 also SRA16)
before its sign tests. Consequently this body's comparisons observe the
wrapped signed16 timer, as implemented and differentially verified.

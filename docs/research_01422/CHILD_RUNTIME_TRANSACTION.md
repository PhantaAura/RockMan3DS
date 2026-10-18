# Bounded child runtime transaction — 01422

Status: SOURCE_BACKED_PARTIAL. `ChildRuntime` connects the already proved
class20/action4 producer and class60/subtype06 lifecycle to real bounded native
proxy, physical registry, shadow and audio producer storage. It embeds no owner
EXE instructions, UV data, textures, models or other owner payloads.

## Ownership and atomic commit

The owner boots the exact shared32-slot/268-byte class60 pool, including class
bytes60..7F. Allocated and pending-release masks retain original MSB-first slot
bits. Pending release clears byte0 without making a slot reusable; explicit
batch cleanup applies the pending mask and resets the native pending owner.

`prepare_parent_delivery` runs the existing80104078 producer against retained
scratch, stages its cue178 through actual8001A050 queue storage, and produces a
private `ParentChildDelivery`. Validation compares the owner, both pool masks,
all268 selected preimage bytes, producer raw/fixed lanes, variant, full behavior
premise, and every retained audio byte/count/initialization field. A zero token
is admitted only after the actual shared pool reports all32 slots allocated.
Subtype09 remains held when a slot is available because its callback is unowned.

The actor bridge prepares/validates this delivery after detection/audio staging,
passes its scalar token to the source reducer, and commits the child pool only
after body and animation consumers succeed. It publishes the staged parent
audio/state afterward. Arbitrary external `produced_child_374` inputs still do
not authorize the native bridge. A missing request22 pose leaves all child pool
and audio bytes unchanged.

`tick` reruns the pure scalar lifecycle at its published contact, proxy and
shadow call boundaries. It stages contact against the active `RuntimeMap`,
registers the exact800341BC proxy arguments, stages80051BAC append/clear, queries
the exact shadow floor wrapper and stages8002C8CC. Scalar readiness fields are
set only after these real producers succeed. One final commit publishes child
bytes and queue tails/targets/owner points together. Proxy overflow, malformed
counts, registry overflow, missing metadata/geometry and missing effects hold
all268 child bytes, both pool masks, every persistent queue record/count, all
persistent proxy targets and audio bytes. Scratch mutations never escape a hold.

Proxy target identity is the source child token+9C, rather than a cast native
pointer. Encoded record identity retains source class and low23 token bits.
The native bank's typed target pointers reference stable native storage only.
Exact proxy arguments are point90/92/94, extent32, a2=0, mode4000/8000,
stack10=class byte, stack18=08000000, stack1C=0 and stack20=128. Source stack
point construction writes three shorts and retains the fourth. A masked owner
+BC eight-byte update therefore requires an explicit fourth-lane premise;
missing padding holds that branch. No padding is observed when the source mask
selects no such owners. Global mask provenance remains the native owner's gate.
The same hash-gated probe verifies actual80033EBC startup mask255 and empty
registration count. Under this fresh owner, initial child flags16 become144
on first registration; its bit128 clears the low byte before the subsequent
registration returns128. The native owner models this startup subset; later
mask leases remain unavailable.

`consume_proxy_pairs` runs the existing proved kind0 subset and copies packets,
flags and pointed fields back only to children actually registered in that
batch. Other source actors may register into the same bank. This is distinct
from unported physical pair80051BE4 and the excluded earlier player/GTE proxy
consumers80034EB8/800358F0.

## Shadow wrapper proof

Original80048E6C passes work+14, bounds token+78, signed Y(+16) minus signed
previousY(+2E) as a32-bit delta, and unsigned class byte+1 to80048EA4. The new
`query_class60_shadow_floor_80048e6c` adapter admits null bounds and delegates to
the existing conservative bounded floor query. Nonzero bounds, unproved
primitive visits and deltas outside its admitted arithmetic range hold.
Valid32767 no-floor results retain their original meaning and shadow packing.
Full shadow queues skip the floor query before returning, as source8002C8CC does.

The read-only hash-gated probe runs288 authored scalar/geometry cases through
original80048E6C and80048EA4:211434 instructions, exact argument matches and
unchanged268-byte work in every case. Native246 cases match original returns;
42 unproved cases remain explicitly held. The fixture contains authored scalar
contexts only. Probe command:

```sh
python3 -B docs/research_01422/class60_shadow_floor_source_probe.py \
  --exe /OWNER/ROCK_NEO.EXE --dat /OWNER/CDDATA/DAT --out /NEW/private/shadow-floor
```

## Validation and limits

`test_child_runtime` covers real32-slot allocation order/exhaustion, pending
release/reuse, stale/replayed producer receipts, all persistent queue/target and
268-byte rollback, real proxy registration packing, shared actor/child kind0
packet delivery, UV/CLUT/tpage packing, full shadow behavior, missing effect128
and contact131 consumers, unknown geometry, and1024 heap-free child ticks.
`test_actor_child_delivery` covers the real action4 request21→22 transition,
pose failure rollback, arbitrary external token rejection, full audio drop,
true pool exhaustion, subtype09 holds and512 heap-free actor transactions.

Both new tests pass with Apple clang C++17, `-Wall -Wextra -Wpedantic -Werror`.
The modified lifecycle retains2160 callback/192 motion/9 primitive differential
passes, including816 releases,408 effect initializer intents and1008 contact
boundaries. Native devkitARM GNU++17/ARMv6K/O2 compiles this owner with the same
warning/error set. Compiler-exclusive stack frames: child tick2608 bytes,
parent actor bridge2624 bytes, parent producer preparation72 bytes, shadow
wrapper112 bytes; these are
compiler reports, not measured Old3DS stack/performance evidence. Pool and
queue scratch remain in the native global owner, never in a pool-sized hot-loop
stack allocation. The native owner should be retained rather than copied.

Effect128/131 allocator/callback ownership is absent. These paths hold; missing
consumers are never represented as allocator exhaustion. Physical pair80051BE4,
shadow rasterization, source GTE projection/fog, alternate primitive layouts and
unowned child subtypes remain partial. Producer queue reset does not establish
shadow/physical/audio dispatch. Native primitive helpers preserve packed UV
carry, CLUT high88, tpage low88 and color8C; this does not prove raster output.
No damaging player contact, observed retail encounter, retail RNG chronology,
1:1 gameplay, hardware performance or physical Old3DS validation is claimed by
these authored tests.

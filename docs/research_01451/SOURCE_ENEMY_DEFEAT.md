# Source enemy defeat transaction

Checkpoint 0.0.14.51 completes the bounded native transaction reached when the
imported class-20/subtype-03 enemy's source HP reaches zero. The supplied
decompiler export and owner executable were research evidence only; neither is
compiled or distributed.

## Verified original boundary

- The recovered tail at `0x801027CC..0x80102848` allocates effect subtype
  `0x80` with parameter `0x41`, stores its identity at actor `+0x368`, releases
  the actor, calls reward wrapper `0x80047228` with actor point `+0x180` and
  selector zero, and requests sound `0xA1` at that point.
- Allocation failure does not prevent actor release.
- `0x80047228` resolves the class/subtype stat record and forwards its first
  word plus the actor point to `0x80046B34`.
- Both applicable sampled class-20/subtype-03 rows carry reward descriptor
  `0x01010202`.
- `0x80046B34` expands that descriptor into three subtype-`0x87` child effects
  through the source RNG path. Exact RNG chronology, effect-`0x87` update and
  pickup collection remain unresolved.

## Native integration

The L3CB combat profile is version 4 and retains the opaque reward descriptor.
The existing class-20 callback, child/effect pool, audio queue, hit proxy and
imported actor runtime now stage the zero-HP operation as one transaction. On
success it publishes effect `0x80/0x41`, sound `0xA1`, one bounded reward
request containing the exact point and descriptor, then tombstones the actor,
unbinds its proxy and prevents further updates. Missing reward evidence or a
full reward queue holds the transaction before any partial publication.

The regression drives two eight-damage Buster packets through 16→8→0 HP,
checks reaction/defeat timing, removal, exact effect/sound/reward operands, and
proves later ticks cannot duplicate effects or rewards. The transaction uses no
heap allocation.

## Explicit limits

The queued descriptor is an exact handoff, not a completed pickup. The original
RNG-driven subtype-`0x87` expansion, effect callback, collection, and resulting
progression state are still held. Effect `0x80` parameter `0x41` is allocated,
but its complete source callback/visual lifetime is not reconstructed. The
encounter lifecycle is therefore `SOURCE_BACKED_PARTIAL`, not retail parity.


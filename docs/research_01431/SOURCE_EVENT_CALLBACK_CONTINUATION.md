# Source event callback continuation (0.0.14.31)

The frozen01430 emulator trace stopped at native frame585 with phase6 status5,
which is `UnsupportedCallbackTable`. `NeedsCameraField` is status4. The first
script timeline deadline515 advanced the table pointer by8. The prefix consumer
accepted only its original base pointer, so it could never execute the second
callback. This was a native table-dispatch limitation, not proof of an unknown
camera byte or a broken owner package.

The prefix now resolves the current bounded timeline record from the admitted
L3P6 data owner. It executes only original-instruction-backed callback identities.
Sixteen of the seventeen table callbacks have bounded scalar-store consumers;
the callback with shared random/effect chronology holds explicitly. Frame-specific
pointer, bitmap, header and read/modify/write owners are preflighted before stores.
A fixed64-entry store plan avoids copying the large actor pools or allocating in
the update loop. A downstream camera/timeline/caller hold retains the consumed
callback identity and does not execute that callback a second time.

The second callback clears the scene color-overlay control on its frame0 and
updates the original class40/E0 work fields. Those stores do not prove that the
class40 actor's complete animation/behavior/render consumers are implemented.
Callback-driven scalar motion, effect allocation and raw state requests remain
separate from gameplay-complete scenes.

The differential oracle caught an initial wrong allocator-family hypothesis
before the code was accepted: original800316BC uses familyC0 at800A2D08, sixteen
36-byte records. It does not allocate E0 actors. The inherited reset8002E924 also
resets this C0 pool. The native context now retains that bounded pool, its bitmap,
and raw word80098818. Original byte800A2D0B is C0 slot0+3, so the callback writes
the same owner rather than a disconnected duplicate global. No resource class or
visual effect is inferred from those allocations.

The optional prefix field1F800002 was previously labelled controller input.
Unchanged80011ECC..80011EE4 proves it is a shared main-loop iteration halfword:
load, increment, low16 store, then jump to the next main-loop iteration. The
native interface now calls it `scratch_half_1f800002`. Its initial value remains
UNKNOWN. The executable BSS clear does not include scratch memory; the observed
initialization prefix sets only scratch bytes0/1. No native frame count, button
mask, or fabricated zero supplies this missing owner. Explicit source-snapshot
admission and an exact tail consumer exist, but the native frontend leaves them
unbound until an initial owner is established. Bobbing and later camera branches
that require this shared word therefore hold.

The original loop orders event/actor/render services before that tail. The
current native E0 service is an opt-in diagnostic sweep. It shares active resource
ownership, existing reference tables, event-work pointer identities, and the
same optional scratch halfword; full original main-phase/cadence parity remains
CANDIDATE. First-update counts and later consumed-update counts are reported
separately. The old first-update-only service remains available.

The source view projection initializer is exposed only as an explicit fresh
executable-boot operation. Warm area/save lifecycle operations do not invent
projection controls. Source view production, tracked camera owners and native
near/far/depth policy are documented separately by the view adapter.

Evidence: `callbacks-source-proof.json` records156 authored selected-callback
cases and7413 unchanged original instructions. `main-loop-source-proof.json`
records eight complete reset cases (63960 original instructions) and thirteen
counter-tail cases (91 original instructions). All probes use the supported
owner EXE/overlay SHA-256 identities, with zero helper hooks and instruction
patches. Fixtures contain authored state deltas/scalars, not original code,
retail assets or saves. These are differential logic tests, not live retail
chronology or 1:1 game/hardware claims.

Validation: existing `source_event_phase6` test now includes the156 whole-owner
callback comparisons, eleven malformed/owner/capacity/RNG atomic guards,
thirteen counter-tail comparisons, C0 reset checks, and4096 allocation-free
unknown-counter holds. Inherited prefix/camera/timeline/profile/ownership tests
remain enabled. A standalone GCC16.1 ARMv6K compile with `-Wall -Wextra -Werror`
passed after explicit braces corrected misleading-indentation warnings; its
prefix stack report is1440 bytes. Root validation records the complete host and
native build outcomes separately.

Unresolved: initial shared scratch halfword; shared RNG chronology for the
remaining effect callback; complete C0/effect update/resources/draw; class40
scene behavior and presentation; tracked/target camera branches; full native
scene scheduling, original completion/transition handoff, and physical Old3DS
validation. PHYSICS_1TO1 remains false. No full scene or World Comes Alive
completion is claimed.

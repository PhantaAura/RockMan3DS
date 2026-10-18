# Porting map — 0.0.14.47

Reused: the 0.0.14.3 safe renderer, independently gated enhanced renderer, generalized area
package, two-phase destination staging, explicit unload, source event pipeline, exact spawn
receipt, recovered gameplay camera baseline and the audited owner-local cooked tutorial package.
No extraction or recooking occurred.

Changed: a bounded, allocation-free completion receipt for original callback `0x80015840`.
It requires matching owner generation, destination, source function, callback, camera initializer,
area commit, spawn commit and camera commit before changing transition state. The proven area-event
prefix and known completion flags are applied before acknowledgment.

Cannot yet verify: fresh source RNG/constructor-clock chronology, complete camera ownership,
natural behavior of the first imported Area 1 enemy, full combat, broader EBD semantics,
cross-package progression, physical frame time and memory peaks.

Milestone: prove a source-driven transition clears pending state and returns to controllable,
collision-backed destination gameplay. This passed in Old-model Azahar; hardware is pending.


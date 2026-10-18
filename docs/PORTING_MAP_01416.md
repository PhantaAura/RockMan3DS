# Porting map before implementation — 2026-10-05

Reuse: complete01415 cumulative source,0143 safe renderer and independently gated
UV/material path, original actor/player mesh/pose decoders, cooked package/map/EBD
bindings, source kernels, bounded actor reload/cache and owner-local build tools.

Change for Old3DS: reconstruct original mutable map initialization once at the
loading boundary; share its resulting selection with collision, actor floor and
terrain resource construction. Preserve bounded pools and no hot-loop allocation.
Then wire proven source actor scheduling/material/HP/event operations through the
existing interfaces. No wholesale engine rewrite or external code/assets import.

Not verified: IDX writer/timing/domain, full AI/hit delivery/event progression,
material shade/palette selection, original animation cadence and physical Old3DS
memory/pacing. Public decomp is unfinished; viewer guesses are not source parity.

First testable milestone: capture original IDX writer during isolated initial area
load, validate it with a bounded original-code fixture, implement the proved rule
in shared native load preparation, run all inherited36host suites, compile actual
ARMv6K target and exercise Old-model Azahar with genuine captures. If that writer
is blocked, retain UNKNOWN and progress independently on original actor materials
and source scheduler evidence; do not fabricate a floor or combat behavior.

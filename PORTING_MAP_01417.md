# 0.0.14.17 continuation map

Base: verified 0.0.14.16 cumulative source archive; no Git branch/commit exists.
The parent source, source archive and owner data remain unchanged.

Reuse the native ARMv6K/libctru/Citro3D frontend, 0143 shader fallback,
pointer-free map/render packages, bounded package loader, shared runtime map,
imported actor resources, source-backed partial contacts and host regressions.

This milestone connects the newly proved original map initializer to cooking
and package loading. Preserve its HED admission and pointed render-header seed
as optional, CRC-bound neutral metadata. Apply it once before geometry and
collision consume the same map. Keep old packages compatible and reject
malformed or mismatched metadata before any map mutation. Initialization is
area-load work; it introduces no per-frame allocations. The supported startup
profile is restricted to the source-tested stage-0 variant guard.

Also preserve immutable dialogue-bank ownership across session copies, prepare
fallible restore/transition data before publishing state, and separate diagnostic
position recovery from a real area transition. These changes retain existing
behavior while removing lifetime and stale-state failures.

First testable milestone: inherited host tests, synthetic initialization and
transaction regressions, full owner map comparison, an actual Old-3DS-target
build, and Old-model Azahar boot/movement/contact evidence. Source replay and
emulator output cannot prove hardware timing or complete retail gameplay parity.

Still unverified: full enemy callback, BIOS random state, detection ownership,
complete physics/camera/lock-on/combat/event progression, later map variants,
total device memory peaks and physical Old 3DS operation. Keep unknowns explicit;
World Comes Alive is not complete until its ordinary gameplay chain is exercised.

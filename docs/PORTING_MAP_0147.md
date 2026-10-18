# 0.0.14.7 integration map

Continue the 0.0.14.6d/r1 tree descended from the supplied 6c work. No Git repository, branch or commit exists. Preserve the older source trees/ZIPs and the unchanged vshader0143.v.pica fallback. Synced sources remain read-only.

Reuse: the neutral MML2 envelope, cooked L3MP/L3RG readers, RuntimeSession, map/contact query helpers, source-backed player helpers, Citro3D frontend, texture caches, existing animation/reference assets, and inherited tests. Generalized package sections 2/3 are the same cooked formats the frontend previously loaded loose.

Change: bounded streaming envelope validation; preflight decoded allocation and expanded terrain admission; feed package map/render into the SAME session and renderer; move rather than duplicate map/message ownership; explicit inactive-location cleanup; local cooked-only packaging workflow; before/after allocation diagnostics; corrected checkpoint metadata.

Not verified: current-build emulator execution (Mac is locked), physical Old 3DS behavior/pacing, complete terrain topology/UV/material fidelity, retail physics, object list selection, EBD behavior, enemy/event/camera/audio/cutscene/progression semantics. Generalized envelopes have no texture-state, player, animation, audio or progression payload contract. Skipped records are descriptors, not executed gameplay.

Small testable milestone: two synthetic area packages reach the existing session/contact/render helpers; the already authorized tutorial cooked pair is wrapped locally and admitted by the same loader; actual ARMv6K build succeeds. Current native execution and hardware acceptance remain separate pending checks. This is a technical prototype, not a faithful full tutorial/game.

Old 3DS admission policy: at native boot, file <=8 MiB, individual section/loose file <=4 MiB, estimated decoded CPU package <=4 MiB, estimated loading peak <=8 MiB, <=256 local areas, <=64 envelope sections. Each active terrain CPU/VBO buffer is capped at4 MiB using the current six-vertex quad expansion. These are failure limits, not measurements or proof of total RAM compatibility. Geometry is rejected, never silently reduced. The largest shared map/render remains resident per package; GPU material/geometry residency is area-local. Full cross-package streaming still needs content binding/progression work.

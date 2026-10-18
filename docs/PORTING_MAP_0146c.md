# Old 3DS hardware-test milestone — porting map (before implementation)

Baseline: supplied cumulative 0.0.14.6b source, unchanged from archive SHA-256 ca9a2f72cc7bd31e50ca47b678fb9abadbc2c3e18fd500fa4435c6959b083d0b. Neither workspace nor source is a Git repository; branch/commit unavailable and no source differences against that archive. Work continues in this separately named copy; no earlier checkpoint is overwritten.

Reuse: existing libctru/Citro3D frontend, frozen fallback and UV shaders, host core, player geometry/animation, fixed-point movement, projectiles, area loading, existing local cooked tutorial pack. User authorized local reuse of that pack on 2026-09-24. No commercial executable analysis, disc extraction, recooking, asset substitution or asset redistribution in this pass.

Changes for this milestone: share immutable map storage across registered tutorial areas to remove redundant allocations; make real frame/draw/triangle/memory observations visible and logged; add a simple diagnostic launch screen and explicit build identity. Preserve current game rules and platform boundaries. Fix concrete boot/render failures discovered while exercising these changes; do not expand into new gameplay systems.

Cannot currently verify: faithful retail physics/collision, original camera rules, enemy combat, audio playback, natural scene progression, save/load UI, cutscenes and full game. Existing host tests are not evidence of these features in the 3DS frontend. Hardware memory peaks, frame pacing, texture/transparency correctness and controls require the user’s Old 3DS test.

Acceptance: actual devkitARM build; host regression checks; scene and player visible; movement/turning/projectile input exercised; both screens visible; measured diagnostic samples and genuine Azahar captures labeled emulator-only. Use Azahar 2126.0 in explicit Old3DS mode at 100% CPU speed, and provide SD layout for user hardware verification. Remains a playable technical test, not a completed port. No stereo requirement. Stop at this milestone.

Environment: installed Azahar default is New3DS and must be explicitly changed for this test. Physical Old3DS is available to the user, not directly attached to this workspace. No supplied screenshots or design-image files found in the source mirror.

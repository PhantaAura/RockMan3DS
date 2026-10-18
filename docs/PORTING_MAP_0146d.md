# Porting map — 0.0.14.6d scene test

Written before implementation, 2026-09-30. Target: original Old Nintendo 3DS; milestone: a small playable hardware test. This is a continuation of the supplied integration source and the preserved 0146c-r2 changes. There is no Git repository, branch, or commit. See work/pass-20260930/BASELINE.json for the checked source baseline.

## Reuse
Keep RuntimeSession, map sharing, the existing cooked tutorial format, player composition and animation, material caches, Citro3D frontend, safe 0143 fallback, and no-cook SD builder. Keep the five manually selectable test areas, forward movement, turning, and projectiles. Existing cooked data is authorized for local use only and has already been audited for structure, hashes, missing materials, and unresolved behavior. Integrity does not establish fidelity to the original game.

## Changes for this milestone
Correct enhanced-renderer quad triangulation while retaining the old 0143 fallback for comparison. Correct the frontend camera X offset so it trails the movement direction after turning. Preserve the r2 texture V fix. Correct PS1 palette handling of the valid black 0x8000 texel with synthetic regression coverage. Clarify supported controls and version the scene-test build, logs and handoff. Diagnose runtime input and exercise movement/turning/fire on the real frontend; if automation cannot deliver held input, explicitly identify that blocker and keep any diagnostic replay separate from manual-input evidence.

## Old 3DS constraints and uncertainties
Use devkitARM r68 / GCC 16.1.0, libctru 2.7.0 and Citro3D 1.7.1. Native target remains ARMv6K, mono top screen 400x240 and bottom console, with no New 3DS speedup dependency. Measure draw/triangle counters, texture and linear allocation, malloc usage, frame pacing and memory layout through CSV. Emulator Old-model setting alone cannot prove physical timing or the usual 64 MiB app budget; previous Azahar run reported an extended 96 MiB app region. User has Old 3DS hardware, but has not yet tested our build.

## Cannot yet verify or port
No full original gameplay source or independently licensed loose full-game assets are visible in the supplied archives. The game ZIP exposes disc images and nested/split archive members; these are not approved extraction inputs. Do not run extraction/cooking, inspect owner-data binaries, or redistribute the existing local cooked pack. A full-game importer is blocked until approved asset/source inputs and permission scope are provided. Missing tutorial material words, terrain heights/LOD semantics, semi-transparency, model facing, animation equivalence, enemy combat, event progression, natural transitions, cutscenes, audio output, touchscreen and native SD save UI remain incomplete or unverified. Desktop has portable host tools/tests, not a graphical game build.

## Testable acceptance
Build the native .3dsx and host regression targets. Verify cooked pack integrity without recooking. Exercise and capture title and gameplay with visible textured player/terrain, turning and a projectile; record actual input or label replay input explicitly. Deliver a separately named source ZIP (no commercial assets), local SD folder and hardware checklist. Hardware acceptance remains pending until the user runs it on the original 3DS and supplies the resulting CSV/captures. Do not call this a faithful full tutorial or successful full-game port.

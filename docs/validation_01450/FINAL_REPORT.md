# 0.0.14.50 — original Buster callback to imported-enemy damage

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`.
This directory is not a Git repository, so it has no branch or commit. Parent:
0.0.14.49 source ZIP, SHA-256
`93ba57ee6901fc395b8d3f596074a0600dca730560aeeb81e64c0a07695d3021`.

## Original routines identified

The owner-supplied `ROCK_NEO.EXE.2.c` remains external research evidence. It is
not treated as original source or compiled. The implementation uses the
recovered `0x80035724` fixed-pool initializer, `0x80017154` dispatcher,
subtype-`0x80` wrapper `0x800433C0`, normal states `0x800433FC`, `0x800434F0`
and `0x800435BC`, segment producer `0x800435F8`, impact helper `0x80043740`,
map-query wrapper `0x80049C24`, shared proxy-pair traversal, and the previously
reconstructed class-20/subtype-03 hit consumer.

## Gameplay implementation

The existing `ChildRuntime`, hit-proxy bank, imported actor, HP/reaction code,
audio queue, effect pool and ownership masks are reused. The normal callback now
sets the 11-frame lifetime, handles the owner busy byte and lease, queues cue
`0x96`, registers the two-endpoint `0x28000008` producer, reacts to actor or
explicit map contact, allocates effect subtype `0x84`, and performs deferred
release. Pair packets are staged for the imported enemy's next callback.

The opt-in native source-actor frame loop now drives that chain from the existing
pose/muzzle and player work. The regression proves allocation through packet
delivery, 20→12 HP, impact effect and release without heap allocation. This is a
meaningful playable-combat integration, but it is still
`SOURCE_BACKED_PARTIAL`.

## Validation

The final results are recorded in `build-result.json`, `host-tests.log`, and
`native-build.log`. The native target is devkitARM r68-1 / GCC 16.1.0 /
libctru 2.7.0-1 / Citro3D 1.7.1-2, ARMv6K/mpcore hardfloat. No 0.0.14.50
Azahar or physical Old 3DS gameplay result is claimed.

## Remaining blockers

The original writers for player endpoints `+0x400/+0x418`, projectile map bounds
at `+0x78`, and fire-cadence bits at player `+0x174` remain unidentified. The
live endpoint is labeled candidate, and map collision is not fabricated. The
full zero-HP defeat/despawn/reward lifecycle also remains held. Complete enemy
AI, physics parity, wider events/dialogue, cross-package streaming and natural
tutorial progression remain incomplete. `PHYSICS_1TO1=false`; World Comes Alive
is not finished.

The source ZIP excludes ROM/CHD/BIN/CUE, retail executables, decompiler exports,
retail models/textures/audio/video, cooked retail payloads, DSP, saves, and
generated build output.


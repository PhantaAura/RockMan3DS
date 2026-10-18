# 0.0.14.51 — Complete First Enemy Encounter

The recovered zero-HP tail allocates effect `0x80/0x41`, releases the actor,
passes point `+0x180` and the stat-record reward word through `0x80047228`, and
requests sound `0xA1`. Native code now executes that boundary transactionally
for the imported class-20/subtype-03 enemy.

Two eight-damage Buster hits drive 16→8→0 HP, reaction and defeat timing. The
actor is removed, its proxy is unbound, the exact effect/sound operands and
opaque reward request are published once, and repeated ticks cannot duplicate
cleanup or rewards. Existing fixed pools and queues are reused with no gameplay
heap allocation. The owner-local cooker retains reward word `0x01010202` in
L3CB version 4.

Focused encounter/profile/cooker tests pass. The inherited host suite passes
144/144 in 4.89 seconds. The actual devkitARM ARMv6K build passes with no saved
compiler warnings or errors and produces a 919,748-byte `.3dsx`. Eleven cooked
tutorial packages and deterministic replays validate through the one-command
pipeline. Physical Old 3DS validation is pending.

The exact RNG-driven `0x80046B34` subtype-`0x87` pickup expansion, pickup
collection/progression, and complete effect-`0x80` parameter-`0x41` visual
callback remain blockers. The best next objective is to implement that bounded
reward consumer, then validate this encounter on physical Old 3DS hardware.
`PHYSICS_1TO1=false`; World Comes Alive remains unfinished.


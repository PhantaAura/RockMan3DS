# 0.0.14.49 player pose and fire ownership

The second owner-provided decompiler export resolves three previously separate
pieces of the Buster path without changing the renderer:

- `0x800325D4` builds the 15-part player hierarchy.
- `0x800331A0` writes each part position to player work `+0x3A8+part*8`.
- `0x80035724` reads part 4 at `+0x3C8` for its nonzero stack lane and part 7
  at `+0x3E0` for its zero stack lane.

`source_player_pose.cpp` translates the ordinary primary-pose path with the
same fixed-point reference table and bounded matrix rules used by the existing
source actor pose work. It refuses selectors 1..3 because they select the
second eight-part limb bank, which L3AN does not yet retain. Renderer float
matrices are not admitted as Buster coordinates.

`L3BP/1` retains the 16 source subtype bytes and their fresh row-zero counters
as a neutral 96-byte owner-cooked profile. The source ZIP contains the cooker
and decoder, but no cooked profile or executable data.

`l3_pl00_fire_state_80040e9c` implements the exact action 0..7 work mutation.
Actions 4..7 hold atomically until their `0x800423DC`/`0x80042458` helper result
is supplied. Camera/projection ownership, the later projectile callback,
damage and encounter progression remain separate gates.

# ST00 subtype 0x1F: the tutorial's pop-out Reaverbot

Status: **SOURCE_BACKED_PARTIAL**. The ST00 overlay (`9490c200…`) and the owner
EXE (`10d2008f…`) were decompiled with Ghidra 12.1.4 headless and checked
against the instructions.

## Placement
Area 1's conditional list (region 0, cell x 5, z −11..−10) spawns eight
class-0x20 subtype-0x1F objects with parameter 0, gated by story flags
1585–1592. The first three stand in the opening corridor at x 2560,
z −7040/−7552/−8064, y −129. Record 1585 (the first one) has raw byte 0x0B =
0x80, which becomes work +0x0F; the other seven have 0.

## Callback `0x801062EC` (work +4 = 0 in table `0x8011A4D0`)
Prologue, every frame:
- copies the previous animation (+AD), position (+2C..+40) and angles (+5C/+60);
- stores shade +15E = 0x4210;
- draws rand() (`0x8007FE50`, RA **0x80106378**) into +34A;
- dispatches `0x8011C000[+8]`.

After the state:
- the animation steps (`0x80032488`), or restarts when +AC changed
  (`0x800322A8`);
- while flags +348 bit 0x20 is set, `0x8007ADE8(+208, +37C)` runs;
- `0x8002C8CC(work, 0x30, 0x200, 0)` runs;
- while bit 8 is set, the hit proxy `0x800341BC` is registered at +180, radius
  0x50;
- the packet +35C is cleared.

`0x8007ADE8` is a ScaleMatrix: column j of the 3×3 matrix is multiplied by
vec[j] (Q12). The vector +37C is {0x1000, 0x1000, +384}, so **+384 is the model's
Z scale**: the hidden one is flat (0) and grows to 1.0.

| State | Function | Effect |
|---|---|---|
| 0 | `0x8010648C` | sets +0 bit 1; null-variant resource `0x8002F9C4`; scale +158 = 0x200; +37C = {0x1000, 0x1000, 0}; flags 0x2C; +34C/+350/+352/+35C/+360 = 0; HP +6E/+B0/+B2 = combat record byte +8 of `0x80098844[subtype]`; bounds +78 = `0x8011C014`; state 1; pose request 0 |
| 1 | `0x80106584` | detection only when flags bit 0x40 (never set here); hit `0x80106800` only when bit 2 (never set by 0x2C); action `0x8011C00C[+9]`; +352 countdown; flag bit 1 advances the action |
| 2 | `0x801066B8` | pool release `0x80031824` |

- **Action 0** (`0x801066D8`): waits until work +0x80 < 0x4E201, then sets
  flag bit 1.
- **Action 1** (`0x8010673C`): +34C = 8. Each frame it decrements +34C:
  - while it is ≥ 0: `0x8002FEA4(work, 0x40, 0, 0)` and +384 += 0x100;
  - once it is negative: `0x8002FEA4(work, 0x200, 0, 0)`, +384 += 0x800, and
    flag bit 1 is set.

  In total that is 9 frames, ending at a scale of 0x1000.
- **Bit 1 during action 1:** +2 = 3, +3 = 0x40, +8..+B = 0. The next frame's
  dispatcher runs subtype 3, state 0 (`0x80102410`). Parameter 0x40 selects
  action 8, the airborne entrance (`0x8011BEF8`), which needs stats-record
  byte +0x0B.
- **`0x80106800`:** a nonzero packet plays sound 0xA3. HP drops only when
  packet bits 0x18000000 are set; at HP ≤ 0 it goes to action 1 with flags
  `&~2 | 4`.

## Port
- `src/world/class20_1f_popout.cpp`: the reducer (all-or-nothing).
- `actor_runtime`:
  - spawn profile `0x8010648C`, and `tick_imported_actor_popout_1f`, which maps
    +384 to the render Z scale;
  - `morph_imported_actor_popout_to_class20_03` runs the subtype-3 init on the
    frame after the rewrite;
  - a request-{0} animation bank for the pop-out row.
- RNG: RA 0x80106378 is admitted on the shared class03 stream.
- L3CB v5 cooks stats-record byte +0x0B for action 8 with parameter 0x40.

## CANDIDATE / not ported
- Work +0x80 for 0x1F uses the subtype-3 distance query
  (`class20_03_distance_work80`, CANDIDATE writer).
- The morph frame draws one class03 rand() (RA 0x801022F8) for the subtype-3
  state-0 prologue (CANDIDATE).
- The hit proxy (bit 8) is not registered while hidden, so shots pass through.
  Damage is impossible anyway because bit 2 is clear.
- The 0x1F HP (`0x80098844` record byte +8) is not cooked. It is unused before
  the morph, and the subtype-3 init sets the HP again.
- Work +0x0F = 0x80 (record 1585) is outside the admitted L3CB identity, so
  that Reaverbot gets no combat profile and holds after the morph.

# Mega Buster shot: subtype 0x81

Status: **SOURCE_BACKED_PARTIAL**. Owner `ROCK_NEO.EXE` (SHA-256 `10d2008f…`),
Ghidra 12.1.4 headless decompilation checked against the instructions
(`docs/research_01503/psx_mips_dis.py`). The original game was measured in
PCSX-Redux `f78f8ef8` with OpenBIOS `0b0359a7` (no memory cards, new game,
first tutorial control) using `redux_buster_probe.lua`. That probe drives the
pad only and never writes RAM.

## Why
In 0.0.15.13 every Buster shot missed (Azahar: `impacts=0`, enemy HP stayed 64).
The port fired lane 1 (`+174` bit 0x10, weapon `+170`), which is weapon 0,
subtype 0x80. Its capsule runs from Mega Man's position to pose part 11 or 14
(`+400`/`+418`). Those are the feet: `0x800331A0` stores part positions at
`+3A8 + part*8`, and the skeleton chains 8..11 and 12..14 are the legs. The
port had replaced them with a 2048-unit CANDIDATE forward ray. The original
hit test `0x8003450C` first does a quick distance check against the shape's
first point (the far end of that ray) and then samples only 3 points, so
nothing near Mega Man could be hit.

## Measured (PCSX-Redux)
- Square spawns subtype **0x81** from the hand `+3E0` (lane `+171`), not 0x80.
- Player `+170`=0, `+171`=1, `+173`=1, level bytes `+4FC`=0, `+4FE`=0.
- Shot: radius `+98`=24, velocity `(0,0,-25600)` at heading 0 (100 units per
  game frame), `+14` advances and `+2C` trails one step, lifetime 18 frames,
  identity byte `+1`=0x10/0x11.

## Original code
- Spawn `0x80034EB8`: `+174` bit 0x20 calls `0x80035724(...,stack10=0)` (hand
  `+3E0`, weapon `+171`); bit 0x10 uses `+3C8`/`+170`. The subtype comes from
  weapon table `0x80089A84` byte 7: weapon 0 is 0x80 and weapon 1 is 0x81.
- `+170`/`+171` come from the restore branch of `0x8003BE6C`, which copies the
  game block `0x800B5F88/89`. The new-game writer of that block is untraced, so
  the tutorial values (0, 1) are MEASURED inputs.
- The class-0x10 callback table `0x80089A20` maps 0x80 to `0x800433C0` and 0x81
  to `0x800437A8`. The 0x81 states are in `0x8008A0D8`.
- State 0 `0x800437F8`: flags `|=0x2A`, `+6 |=0x20`, `+C4..CB` cleared,
  `+78 = 0x80097DA0` (bounds ±16). Velocity is RotMatrix(pitch `0x80039B80`,
  heading `+64`, 0) applied to `(0,0,0xF000)`, then times speed
  `0x80097DD4[+4FE]` >> 4. Radius `+98 = 0x80097DCC[+4FC]`, `+1A/+32` =
  0x4208/0x6208. The lease is released when the target point is zero. It then
  registers and plays sound 0x9A.
- Registration `0x80043E78`: `0x8003430C(+14, +98, +2C, +98, 0x200,
  +0C?0x6000:0x4000, +1, +C4, 0x800375D4(player,1) | 0x08000000, 0, 0x80)`.
- State 1 `0x80043A8C`: `0x80037694` returns 0x7FFF on a published pair packet,
  else `0x80049C24` -> `0x80049C54` tests the point `+14`. The second point is
  used only for the ±0x4000 sign check. With no hit: previous = current, then
  age `+0B` counts up while it is below the range `0x80037634(player,1)`. Then
  fixed += velocity × 0x100, `+9A += 5`, and it registers again. Otherwise it
  releases. A hit spawns the 0x82 effect through `0x80043E00` (flags 9, `+3`=1,
  `+0C` = map hit), then releases, along with any lease.
- Weapon row 1 (`PTR_DAT_80089EAC[1]`), level 0: damage 24, range 18.
- Impact effect 0x82 (`0x80089858` table -> `0x80044AB0` -> `0x8008A250`):
  state 0 calls `rand()` (RA `0x80044B50`, value & 0x3F), and plays sound
  0x9B for map hits. State 1 steps 6 timers from `0x8008A240` (all 1), then
  releases.

## Port
- `L3BP/2` cooks the weapon level rows, the radius and speed tables and the
  0x82 timers.
- `ChildRuntime::tick_source_busters_80017154` runs the 0x81 states.
- `source_effects` runs the 0x82 effect on the shared RNG service, as an
  admitted effect-pool caller.
- `main.cpp` fires lane 0 (bit 0x20 / stack10 0) with the measured loadout.

Lock-on homing (0.0.15.16, SOURCE_BACKED_PARTIAL), 0x80043B54..0x80043D24.
A shot with +3 bit 7 (a lease from 0x8003545C) steers each flight frame:
1. If the target point (+BC/+C0 words) is zero, release the lease and clear
   +3.
2. Otherwise compute d = target (+BC/+BE/+C0) − position (+14/+16/+18). If any
   component is beyond ±26750, shift all three right by 2.
3. Normalize with `0x8007A694`.
4. Call `0x80033F04(n, +A4 direction, turn, threshold)`. +0B has already been
   advanced, so the arguments depend on the age:

   | Age | Turn | Threshold |
   |---|---|---|
   | < 4 | 1060 | 0x0F746EA4 |
   | < 9 | 143 | 0x0FFD813C |
   | otherwise | 71 | 0x0FFF604C |

   In the last case the lease is also released (0x80043C98) and +3 cleared, so
   homing ends there.
5. Store the result at +A4, and the velocity +44 = dir × `0x80097DD4[+4FE]` >> 4
   (the shot speed).

The detector's `source_detection_interpolate_80033f04` is now the general
`source_interpolate_80033f04` with turn 357 / threshold 0x0FF069DA. Without
the `0x8007A694` table, a leased shot holds (`NeedsNormalization`). Host test:
an authored table steers a shot toward an enemy at +x and drops the lease on
frame 9. Not yet watched in Azahar.

UNKNOWN / not ported:
- Non-zero aim pitch: holds with `UnsupportedAimPitch`.
- `0x8003777C` breakable-cell damage, and the `0x8004B414` work update after a
  map hit (the shot is released in the same frame).
- `+7C` (cleared after every callback by `0x80051BAC`): taken as 0.
- Presentation fields (colour, sprite).
- The writer of the persistent loadout block.

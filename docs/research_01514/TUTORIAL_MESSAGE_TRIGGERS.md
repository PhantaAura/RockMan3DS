# Tutorial progression: E0/06 message triggers and the subtype-0x1F pop-out

Status: **SOURCE_BACKED_PARTIAL** (research; not ported yet).

**Sources.** The ST00 overlay (`9490c200…`) decompiled with Ghidra 12.1.4
headless, using a MIPS ELF wrapper at 0x80100000. The owner EXE (`10d2008f…`).
The original order was observed in PCSX-Redux (OpenBIOS, new game) with
`redux_tutorial_probe.lua`, which only drives the pad and logs RAM, call hooks
and screenshots.

## Observed order (original, Area 1)
1. Control starts at (2822, −1, −4833). At z −5643 Mega Man is frozen and
   Roll's radio runs: "Wait a second!", "It looks like a small one--", then the
   instructions ("Use the □ button to fire!", "Press the R2 button to
   automatically lock-on to the nearest target", …), then "Read instructions
   again? Yes/No". **Yes loops the instructions.** No sets flag **1552** (from
   RA `0x80114728`) and releases control.
2. Player `+0x44D` goes from 0 to 1 only after this step, which enables the
   follow camera's eye-in-wall pull-in (`0x80038894`). The port never sets it,
   so its camera can sit inside walls.
3. Further on (z ≈ −9400) flag **1554** is set from RA `0x80114820`.

## Controller
- The class E0 subtype table is at `0x8011A6CC`. Entry 0 is `0x801116DC` (the
  conditional-spawn room controller already ported); entry 6 is **`0x80114424`**,
  which dispatches `+8` over `0x8011CB18`: 0 `0x80114460`, 1 `0x80114504`,
  2 `0x801145D4`.
- Areas 1 and 2 each place one E0/06 at (0, 0, 0) in their static object list.
- State 0 (`0x80114460`) clears bytes `+8..+B` of every trigger record of
  every area, then sets state 1.
- State 1 (`0x80114504`) stores `+38 = playerX >> 9` and `+3A = playerZ >> 9`
  (0x800B51C4/C8, signed), then walks the list `0x8011CC9C[area byte
  0x800C1B61]`. Each 16-byte record is `{u8 type, s8 msg, s8 cellX, s8 cellZ,
  u16 width, u16 depth, u8 state @8, …, s16 timer @C}`, ending at 0xFF. It calls
  `0x8011CB24[type](controller, record)`.
- Region test `0x801159F8`: `(u16)(+38 − cellX) < width && (u16)(+3A −
  cellZ) < depth`.

## Handlers (`0x8011CB24`)
Common pattern:
- Nothing runs while master flag **0x622 (1570)** is set, or once the handler's
  own flag is set.
- State 0, when the player enters the region: state = 1,
  `0x80053A08(0, msg)` (message from the bank at 0x80153000), player `+0xBD
  |= 0x40` (freeze), `0x8002EE90(0x27E, 1)` (pause the object classes in mask
  0x27E), controller `+0 |= 1`.
- State 1, once `0x80098A58 & 0x08000000` clears (the message has finished):
  optional `0x8001D648(n)`, set its flag, `+0xBD &= ~0x40`,
  `0x8002EE90(0x27E, 2)`.
- `0x8001D648` looks up an entry through tables `0x80088F9C`/`0x80088EF8` and
  calls `0x8001D6D8`. It is probably the matching voice line (CANDIDATE).

| type | handler | own flag | notes |
|---|---|---|---|
| 0 | `801145F4` | gated by 0x610 | state 1 only; `0x8001D648(0)` |
| 1 | `8011466C` | 0x610 (1552) | the radio and instructions; `0x8001D648(5)` |
| 2 | `80114774` | 0x612 | no freeze; `0x8001D648(6)` |
| 3 | `80114838` | 0x613 | no freeze |
| 4 | `801148F4` | 0x614 | msg or msg+1 by heading `0x800B5206` |
| 5 | `80114A34` | gated by 0x615 | `0x8001D648(10)` |
| 6 | `80114AAC` | gated by 0x60A | needs 0x607; sets 0x615, then 0x61F once no class-20 object is left in the region |
| 7 | `80114C88` | 0x616 | `0x8001D648(14)` |
| 8 | `80114D90` | 0x617 | `0x8001D648(7)` |
| 9 | `80114E98` | 0x618 | needs player Y == −193 for 90 frames; `0x8001D648(21)` |
| 10 | `80114FE0` | 0x619 | sets 0x618 and 0x60B |
| 11 | `801150CC` | 0x61A | waits until the player stands on the floor (`0x80048E6C`); mask 0x256 |
| 12 | `80115210` | 0x61B | needs 0x608 or 0x609 clear |
| 13 | `80115330` | 0x61C | msg or msg+1 by 0x61B |
| 14 | `80115480` | 0x61D | starts on flag 0x60E (then clears it) |
| 15 | `801155A4` | 0x61E | **sets 0x622**, ending the tutorial triggers |
| 16 | `801156C8` | — | not decompiled yet |

Area 1 records (cells; world = cell × 512):
- type 0: x6 z−10
- type 1: x5 z−12
- type 2: x5 z−19
- type 3: x−1 z−20
- type 4: x−7 z−20, depth 2
- type 5: x−7 z−16
- type 6: x−8, width 3, z−10
- type 7: x−1 z−19
- type 8: x−1 z−16

Area 2 holds types 9–16. Message ids are per-area bank indices.

## The "small one": subtype 0x1F
- Area 1's conditional list (region 0) spawns eight class-0x20 subtype-0x1F
  objects (e.g. 2560, −129, −7040) and six dormant subtype-0x19 objects
  (state table `0x8011A490`). The port reads these records but builds only
  subtype 3.
- The subtype pointer table is at `0x8011A4D0` (scratch `0x1F800058`); work
  `+4` = 0 selects callback `0x801062EC` with states `0x8011C000`:
  - init `0x8010648C`: HP from the combat table, bounds `0x8011C014`, flags 0x2C.
  - active `0x80106584`: action 0 `0x801066D8` waits for work80 < 0x4E201.
    Action 1 `0x8010673C` lunges for 8 frames (forward 0x40, then 0x200).
    The object then becomes **subtype 3 with parameter 0x40** (the airborne
    Reaverbot entrance, action 8).
  - release `0x801066B8`.

## Port plan
1. An E0/06 controller, with the trigger lists cooked owner-locally and the
   handlers as specified above.
2. A gameplay message box over the `l3msg` banks, with input to advance and
   the Yes/No choice. The glyph-to-text mapping still has to be derived.
3. Subtype 0x1F, handing off into the existing subtype-3 code.

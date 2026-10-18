# Tutorial Runtime Evidence 0.0.6

This checkpoint turns more of the USA ST00 program overlay into pointer-free runtime data. It intentionally uses neutral names wherever behavior is not yet semantically proven.

## Five tutorial sub-areas are executable-proven

The ST00 setup routine at PS1 `0x8010038C` reads the signed current-area byte at `0x800C1B61`, bounds the selector to `< 5`, and dispatches through five cases. The common setup then uses the same area index to select:

- an object-list pointer from `0x8011AB3C`, consumed by the common 20-byte object dispatcher `0x8002EA50`;
- an auxiliary-list pointer from `0x8011AC40`, installed at scratch `0x1F800050`;
- one 40-byte setup blob from `0x8011AC54`;
- a setup selector from `0x8011ADE4`, whose real values are `{0,5,10,15,20}` and which selects an 8-byte seed from `0x8011AD1C`;
- a resource/setup pointer selected through the table at `0x8011A6F0` and passed to `0x80022FD4`.

The pointer-free `L3SF` format preserves all five area records and converts every retained absolute overlay pointer into an offset relative to the `0x80100000` ST00 payload base.

## Object lists

The common object parser `0x8002EA50` walks exact 20-byte records and stops when record byte `+0x00` is signed `-1` (`0xFF`). It dispatches on the class bits in byte `+0x01`. Real ST00 counts are:

| Area | Active object records |
|---:|---:|
| 0 | 1 |
| 1 | 16 |
| 2 | 20 |
| 3 | 11 |
| 4 | 1 |
| **Total** | **49** |

Observed class histogram across the 49 records: `E0=35`, `C0=9`, `00=3`, `20=2`. These are kept as class bytes; no enemy or boss names are assigned here.

## 24-byte auxiliary lists are structurally proven

Generic runtime code beginning around PS1 `0x80050CC0` loads the current area's pointer from scratch `0x1F800050`, reads a signed halfword at `+0x00`, stops on `-1`, and advances to the next record by exactly `0x18` bytes. Multiple branches compare record halfwords against current position/orientation-like values and then invoke common event/state functions. This proves a per-area 24-byte runtime record stream, but not enough semantic detail to safely name every field.

Real ST00 active-record counts are:

| Area | Active aux records | Storage including sentinel |
|---:|---:|---:|
| 0 | 0 | 1 × 24 bytes |
| 1 | 3 | 4 × 24 bytes |
| 2 | 2 | 3 × 24 bytes |
| 3 | 0 | 1 × 24 bytes |
| 4 | 0 | 1 × 24 bytes |
| **Total** | **5** | |

`L3SF` now cooks those five active records after the object table. Each area record stores `aux_first` and `aux_count`; the original source offset is retained only as an overlay-relative diagnostic value. The sentinel is source-format structure and is not needed in the pointer-free cooked array.

## Camera choreography is in ST00 logic, not MSG

The two tutorial MSG banks contain zero `CAM` opcodes. The ST00 overlay itself directly calls `Swing_Camera2` (`0x800398F8`) at five overlay sites:

`0x8010879C`, `0x80108A88`, `0x80108BB0`, `0x8010984C`, `0x80109B08`.

It also calls the related camera routine `0x800398E0` at:

`0x80111068`, `0x80111100`, `0x80115C10`.

For example, the first `Swing_Camera2` call passes `0x1E`, `0x7D0`, `0x1388`, and a pointer at object/work `+0x14`, then immediately performs a sound call. This is executable evidence for coupled stage camera/event choreography. It is not converted into a guessed cutscene script yet.

## Player pose matrix chain is executable-supported

The original player pose code around `0x800331A0` transforms a child local translation by its parent matrix, builds the local rotation with Psy-Q `RotMatrix`, multiplies parent and local rotation matrices, and adds the parent world translation before storing the child matrix/position. That matches the native bridge's hierarchy construction `world = parent * (T * R)`. `RotMatrix` uses X→Y→Z composition.

`pose_player_visuals()` now consumes the proven `L3PC` attachment map plus the 15 animation pose matrices and emits actual world-space player primitive vertices. This closes the old gap where primitives carried a transform ID but were not yet posed.

## Input reconstruction update

The turn-ramp routine matching original `0x80042154` now handles direction reversal exactly: if the current accumulator has the opposite sign, the first opposite-input frame zeroes the accumulator and does not also apply the new step. The following frame begins ramping in the new direction. Tests cover both reversal directions.

The directional accumulator path matching `0x8003E068` also preserves the observed asymmetry: one axis retains its accumulated value on no/opposing input, while the second axis is explicitly zeroed when both opposing inputs are held.

## Validation boundary

This evidence does **not** mean the tutorial is complete. The runtime still needs the full per-frame event dispatcher/EBD behavior links, collision and complete player state transitions, combat/lock-on, XA mapping/playback, cutscene execution, title/save glue, and a compiled Citro3D backend. No 3DS compilation is claimed in this environment.

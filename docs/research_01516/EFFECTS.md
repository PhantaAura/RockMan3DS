# Break and burst effects of the tutorial objects (0x80031618)

Status: **SOURCE_BACKED_PARTIAL**. Read with Ghidra 12.1.4 headless from
ROCK_NEO.EXE (USA, raw load at 0x80010000) and the ST00 overlay, checked
against the instructions in `exe_code.dis` / `ovl_all.dis`.

## Pool and dispatch (already ported, unchanged)
- Pool 0x800A3B40: 64 × 0x48, allocator **0x80031618** (`allocate_source_effect_80031618`).
- Common sweep 0x800173B4 → callback by subtype; renderer 0x80029B10 picks
  `0x800898D0[+0 & 0x1C]`: kind 0 = sprite **0x80029C18**, kind 1 = single
  mesh **0x8002A160** (the same consumer as the E0 meshes).

## Producers
| Caller | Header (+0/+2/+3) | Lanes | Port |
|---|---|---|---|
| E0/05 box break phase 0, 0x801141EC | 5 / 0x80 / 0x42 | +0x38 ← box +0x38..+0x3F, then y −= 0x80 | `SourceE005Step::break_effect` → session request |
| E0/08 state 2 end, 0x801165A4 (×3) | 1 / 0x83 / 2 | +0x1C = (x − rsin(+0x30)·0x40·k ≫ 12, y − 0x10, z − rcos·0x40·k ≫ 12), k = 0..2 | session request |
| E0/08 state 4, 0x8011669C | 5 / 0x80 / 0x82 | +0x38 = (x − rsin·100 ≫ 12, y, z − rcos·100 ≫ 12) | session request |
| class 20/03 defeat, 0x801027CC | 5 / 0x80 / 0x41 | +0x38 ← actor +0x14..+0x1B | `prepare_enemy_defeat_delivery` (now copies the point) |
| class 20/19 defeat, 0x80104D44 | 5 / 0x80 / 0x41 | +0x38 ← actor +0x14..+0x1B | same |

`RuntimeSession::take_e0_effect_requests()` carries the E0 calls;
`main.cpp` allocates them through `ChildRuntime::spawn_effect_80031618` and
traces `# effect_spawn`. A full pool drops the request, as the original does.

## Subtype 0x80 — burst (0x80043F34, states 0x8008A208)
- **State 0, 0x80043F70:**
  - parameter 0xFE becomes a silent 0xC2 (+0xC = 0), otherwise +0xC = 1;
  - +0 |= 2 (0x82 when +0xE ≠ 0);
  - velocity/acceleration +0x44/+0x46 from 0x8008A138[param & 15][+0xD];
  - +0x40 = 0x02808080, +0xA/+0x3E/+0x18..+0x37 zeroed;
  - the model selector follows `ebd_selector_plan_80047354`: bit 0x80 → 0;
    bit 0x20 → 2 plus helper children; low nibble 4..6 → itself; otherwise
    `0x8004920C(+0x38) − y > 29` → 1 (air), else 0 (ground);
  - 0x8002F9C4 with subtype 0x80097DE0[+0xE] (0x80/0x8B/0x8C/0x80) stores the
    EBD row pointer at +0x14 (unchanged when no row matches);
  - sound 0xA1 at +0x38 when +0xC is set.
- **State 1, 0x800441CC:**
  - sub-state 0x8008A214[+9]: 0 = **0x8004441C**, 1 and 2 = no-op;
  - 0x8004441C: velocity += acceleration, scale +0x3E += velocity, then
    colour per parameter & 15:
    - default: rows 0x8008A1A8 (end below 200, rebound −10, full above 255 for row 0);
    - case 1: fade under 0x8008A1C8 (200);
    - case 2: ends when velocity ≤ 0; fades by 14 above 0x8008A1E8 (270);
    - cases 4, 5, 6 have fixed constants;
  - then +0x18 = diag(scale) · RotMatrixY(+0xB << 7), 0x8007BC68;
  - then a **0x800341BC** hit proxy (radius scale/2; mask 0xA000 for bit 0x40,
    0xC000 otherwise, 0x4000 for +0xE = 1). **Not ported**: the effect does
    no damage; `proxy_800341bc_skipped` counts it in the `# effect_tick` trace.
- **State 2:** 0x80031988 release.
- Hand-derived from the tables (host test): 0x41 grows 50 → 275 and shrinks
  in 21 frames (colour 0x02606060 at 200); 0x82 grows for 14 frames and fades
  from 270 (0x02727272).

## Subtype 0x83 — puff (0x80044C58)
Already ported (`callback131`): 32×32 sprite, word 0x3D48002E, packet
0x2E404040, UV u = 32·phase, half size +0x24. Parameter 2 (the E0/08
debris) rises 5 per frame through three 8-frame phases and is released
after 29 frames (durations 0x80097DEC).

## 0x8004920C point floor
`runtime_point_floor_8004920c`: cell 0x80048C60 of (x, z); a cell ≤ 0 gives
`~((cell & 0x1F00) >> 1)`. Otherwise every family-0/1 group (zero bounds)
holding the point offers each primitive's top when its bottom lies below y.
The smallest top wins (0x7FFF when none). Diagonal groups (families 2..5) and
the unported 0x8004A060 codes return no value, so the effect waits.

## Camera shake and the gate's other calls (E0/07)
- **0x800398E0(n):** stores n > 0 in 0x800A395F. The follow tail at 0x80038FE4
  lifts the camera target by it, then lowers it by 10 per frame (30/20/10).
  Ported: `SourceGameplayCamera::shake_800a395f`.
- **0x8001B2F0(0xF):** calls 0x80071220, an Ss library call (UNKNOWN). Not
  ported.
- **0x80052528 (box):** clears the +0x76/+0xC0 ride links that name the box in
  the player, class 20, 40 and 60 pools. The port has no ride links, so it is
  a no-op.

## Drawing (3DS)
`draw_source_effects_uv`:
- Subtype 0x80: the class-0x80 EBD meshes (decoded like E0 meshes, Near tier
  only, CANDIDATE), keyed by +0x14, transformed by +0x18 and +0x38, through
  the E0 material plan with +0x40 (semi-transparent).
- Subtype 0x83: a camera-facing quad. Its left/right orientation follows the
  frontend mirror (CANDIDATE).

Textures:
- The burst page 0x3C80002B is uploaded by ST00_01.
- The puff page 0x3D48002E is uploaded by the common file (EXIT_SUP; DEMO_DAT
  holds the same common bank). CANDIDATE, as for the common sound bank.
- The actor cache now uploads EXIT_SUP first, and Area 2 inherits ST00_01.
  The tutorial reaches Area 2 from Area 1, so this is a CANDIDATE upload order.
- Recipe `01516-effects-1`. All 7 Area 2 actor/E0/effect words resolve.

## Open
- The 0x800341BC explosion proxy (damage from bursts).
- The bit-0x20 helper children.
- The depth-selected LOD tiers.
- The sprite fog flag.
- A PCSX-Redux check of the frame counts.

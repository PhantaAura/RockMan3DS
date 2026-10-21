# Map polygon face test (NCLIP) and back-face fill

Status: **SOURCE_BACKED_PARTIAL**. Read from the owner EXE (`10d2008f…`)
instructions and checked in PCSX-Redux at the Area 1 control start with
`redux_cull_probe.lua`. The probe only reads registers at breakpoints.

## Symptom
At the tutorial start the port's top screen looked doubled and blurry, and
Mega Man was not visible. The original follow camera sits 512 units behind
him (0x80037FB4, +0x44D = 0 so no pull-in). That puts the eye behind the start
cell's back wall at z −4608. The port drew every face from both sides
(`GPU_CULL_NONE`), so it drew that wall's back face over Mega Man, plus the
backs of distant walls.

## Original rule
Every map family runs RTPT on vertices 0..2, then NCLIP (`0x4B400006`), and
branches on MAC0 and the primitive word (the cooked `prefix`):

| Family | Used for | Rule |
|---|---|---|
| `0x80023EA8`, `0x80024F30`, `0x80026B04` | positive, mode 0/1/3 | MAC0 > 0 or bit 30 draws, else skip |
| `0x80025900` | positive, mode 2 | MAC0 = 0 skips. MAC0 > 0 or bit 30 draws. Back face: skip if min SZ ≤ scratch `0x1F8001C0` or bit 24 set, else draw as `0x2C010101` (texel/128, near-black) |
| `0x80021B14` | negative cells (`0x8002196C`, all LODs) | MAC0 > 0 or bit 30 draws, else skip |
| `0x8002235C` | negative near/replay (`0x800221E0`) | back face: bit 30 draws, bit 29 skips, else ends the stream (not used by the port's Mid path) |

How the pieces connect:
- **Mode:** byte `0x1F800075`, read at `0x80023C10`. Mode 2 goes to
  `0x800267AC` group 2 (table `0x80089890`) → `0x80025874` → `0x80025900`.
- **t0:** the minimum of the four vertices' SZ (`0x80025D3C..0x80025DA4`).

## Original measurements (PCSX-Redux, Area 1 control start)
- `0x1F800075` = 2; `0x1F8001C0` = **522** (writer untraced: OBSERVED).
- Per frame, the start wall's back faces (t0 = 284/284/270/270) are culled.
  17 farther back faces (t0 1036..7433 region) are drawn black.

## Port
- `classify_terrain_face` (`src/render/terrain_visibility.cpp`) runs the
  ported RTPS (`source_gte_rtps_0180001`) with the `0x1F800008` camera words
  (OFX/OFY 160/120, H 384).
- **Host check (retail trig):** the four culled faces match exactly (min SZ
  284/270/284/270). The port finds 108 black faces where the original draws 17,
  because it has no per-cell visibility or Near LOD. The extras are off-screen
  or behind front faces.
- **3DS:** each frame the terrain batches are drawn through per-quad index
  lists (`C3D_DrawElements`), and a second pass draws the black faces as
  texel × 2/255.
- **When it runs:** only with the gameplay follow camera, a known mode byte and
  scratch shift 0.
- **Unsupported quads:** those with a vertex behind the camera (SZ 0), or that
  the RTPS port refuses, are drawn as before.
- **Not ported:** the door-swap overlay, the legacy/0143 paths, and the
  threshold for areas other than Area 1 (CANDIDATE: same 522).

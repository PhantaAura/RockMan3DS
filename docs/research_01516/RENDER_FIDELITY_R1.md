# Renderer fidelity pass R1: terrain depth cue and the back-face threshold

Status: **SOURCE_BACKED_PARTIAL**. Read with GhidraMCP (headless) on
`ROCK_NEO.EXE` imported through the `ghidra_psx_ldr` loader. The plain ELF
import truncated every function at its first GTE opcode. No PS1 emulator was
used in this pass.

## Original frame (0x800238BC)
1. Bind the camera words at `0x1F800008`.
2. `0x80020A4C` (mode 1) or `0x80020BB4` (others) builds the cell lists
   `0x8015BC00`/`0x8015BD00`.
3. `gte_rt` of the player position (+0x14), stored with `stlvnl` at
   `0x1F8001B8..C0`. So **`0x1F8001C0` is the player's view depth**, and
   `0x1F8001BC = 0x1F8001C0 + 0x172`.
4. `0x80023C08` draws the lists (`0x800267AC` mode 2/3, or `0x80024BAC`).

## Per-cell visibility (0x80020BB4 / 0x80021258 / 0x80021834), not yet ported
- **Search rectangle:** centred 2·|cos pitch| (q12) ahead of the camera
  (`0x1F800028/2C`) along its yaw (`0x800A3A12`). Half-size |cos pitch| >> 8
  cells, clamped to the area bounds (`0x1F800040..43`).
- **Near window:** the cells around the player (`0x800A39A0/A8`), at
  ±|sin pitch|/0x160 + 3 cells.
- **Wedge:** slopes from yaw ± 0x100. The wedge fill uses `0x80021258` or
  `0x80021548`, chosen by yaw quadrant.
- **Per-cell test (`0x80021834`):** the cell centre (y from its descriptor) is
  transformed with RT. The cell is drawn when both hold:
  - `z/4 < 0x1F800076` (far: `0x80097C78[0x1F800074]`, 3584/1664/768 for
    LOD 0/1/2; 0 in tutorial gameplay);
  - `|x| ≤ (106·(z/4) >> 6) + r`, where r = 400 for the wedge and 600 for the
    near window. 106/64 · ¼ ≈ 0.414 = 160/384, the PS1's horizontal half
    field of view.

  Negative cells go to `0x8002196C`. Positive cells set a bit in BD00 when
  `z < 0x1F800004` (0x400 in modes 0/1, 0 in modes 2/3), otherwise in BC00.
- **Port impact:** at LOD 0 the far cut is 14336 units, beyond every tutorial
  room. The side cut only removes off-screen cells, and the 3DS's 400-pixel
  width would need a wider cut anyway (CANDIDATE). These rules are left as
  documentation, not code.

## Depth cue: ported
Every positive terrain family (`0x80023EA8`, `0x80024F30`, `0x80025900`,
`0x80026B04`) does `ldrgb` of the primitive word, then `dpcs`, unless prefix
bit 28 is set. IR0 comes from the RTPS of vertex 3. The negative family
`0x80021B14` always applies `dpcs_b`.

DQA/DQB come from `SetFogNear` (`0x8007A1CC`) with the area record +0x24 and
denominator 384. FarColor comes from bytes +32..34 (`0x800371A0`). For ST00
Areas 1–3:
- fog distance 1280, so DQA = −1066 and DQB = 0x01400000;
- FarColor = (0, 16, 16).

So IR0 ≈ 5120 − 17056·384/SZ: no fog nearer than 1280, and the full far
colour at about 6400.

## Back-face threshold: ported
The mode-2 black fill and cull compare each face's minimum SZ with
`0x1F8001C0`. The fixed 522 measured in Area 1 is replaced by the player's
per-frame view depth.

## Found, not ported (next candidates)
- **CLUT depth ramp:** `0x80025900` ORs `ramp[(z >> (9 + LOD)) & 31] << 16`
  into the primitive's CLUT word. The ramp is area record bytes +0..31
  (Areas 1–3: 0 ×10, then 1 1 1 2 2 2 3 3 4 4 5 5 6 7 …). Distant faces
  therefore use palettes up to 7 entries to the right in VRAM, presumably
  darker copies. It needs those palettes cooked into the material caches.
- **See-through walls:** a front face nearer than the player (max SZ <
  `0x1F8001C0`) whose screen box overlaps x 150..170 / y 120..160 gets
  `0x1F80007C` ORed in. That is 0x02000000 (semi-transparent) when
  `0x800C4C48` is clear.
- **Ordering and subdivision:** ordering-table bucket z >> 3; large near
  faces are subdivided (`0x8002466C`) when SZ > 0x200 and the screen
  size/area exceeds the limits.

## Port
- `terrain_visibility`:
  - `make_terrain_face_camera(words, &area_record)` applies SetFogNear and
    FarColor;
  - each face returns `ir0` and the DPCS `rgb`;
  - `terrain_view_depth_1f8001c0` returns the player's view depth.
- `main.cpp`:
  - each classified batch has a float colour stream (RGB/128, matching PS1
    texel·colour/128);
  - drawn faces take their cued colour; black and unsupported faces stay at
    unity;
  - the threshold is the player's depth each frame.
- **CANDIDATE:** the colour is flat per face from vertex 3, as on PS1. The
  RGB-to-gain mapping assumes TEV scale 1, which is exact for colours ≤ 0x80.

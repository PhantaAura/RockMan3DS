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

## Per-cell visibility (0x80020BB4 / 0x80021258 / 0x80021548 / 0x80021834): ported
`include/legends3ds/render/terrain_cell_visibility.h`, test
`tests/test_terrain_cell_visibility.cpp`. Called from `0x800238BC` before the
terrain families draw. SOURCE_BACKED_PARTIAL: ported from the decompiles and
the `0x80037FB4` tail instructions; not compared against a PS1 cell list.
- **Search rectangle:** centred 2·|cos pitch| (q12) ahead of the camera eye
  (`0x1F800028/2C`) along its yaw (`0x800A3A12`), in cells (truncating /512,
  + 0x40). Half-size |cos pitch| >> 8 cells, clamped to the area bounds
  (`0x1F800040..43` = start x/z, start + width/height).
- **Near window `0x1F800100..106`:** around the camera target
  (`0x800A39A0/A8`), from −3 − |sin pitch|/0x160 to +3 + that (exclusive),
  clamped to the area.
- **Wedge:** slopes `0x1F800108/10A` from yaw ∓ 0x100, stored as halfwords.
  With `(yaw − 0x200) & 0x400` clear, the slopes are (cos+1)·0x1000 / −(sin+1)
  and `0x80021258` walks one z run per x column. Otherwise the slopes are
  (sin+1)·0x1000 / −(cos+1) and `0x80021548` walks one x run per z row
  (offsets 0x41/0x3E swapped). Both skip the near window and test each
  non-zero cell with radius 400. The near window is then tested with radius 600.
- **Scratch `0x1F800075` == 1** uses `0x80020A4C` instead: every cell in the
  area, radius 800.
- **Per-cell test (`0x80021834`):** the cell centre (y = IDX +2, or
  −((packed & 0x1F00) >> 1) for a negative cell) goes through `gte_rt` with
  the `0x1F800008` rotation and TR = `0x1F800030`. That vector is the
  `0x80037FB4` tail's ApplyMatrixLV of the matrix on the negated eye
  (`0x800382CC..F4`), so it equals the `0x1F800008` translation at shift 0.
  - **Far cut:** `MAC3/4 < 0x1F800076`. The far tables are `0x80097C78` =
    3584/1664/768 for LOD 0/1/2, and `0x80097C84` = 3456/1568/704 when
    `0x1F800078` bit 2 is set. That bit's gameplay value is UNKNOWN; the port
    uses 0.
  - **Side cut:** `|MAC1| ≤ ((z4·8·15 − z4·14) >> 6) + r` (≈ 160/384 of the depth).
  - **Result:** a passing positive cell sets its IDX bit in BC00/BD00, so each
    IDX is drawn once whatever cell aliases it. A passing negative cell is
    drawn per cell (`0x8002196C`).
- **CANDIDATE (3DS width):** the 3DS view is 400 pixels at the same focal
  length 384, so the depth term is widened by 5/4 (200/384) and the radius is
  unchanged (`TerrainCellViewInput::widen_for_400`, on). With the PS1 cut, the
  outer 40 pixels on each side would lose cells.
- **Port behaviour:**
  - `draw_terrain` drops a quad whose IDX or negative cell was not marked,
    before the NCLIP/depth-cue/ramp classification, which is unchanged;
  - a cell outside the area (where the wedge leaves the grid and the original
    reads past it) counts as empty;
  - a slope divide by zero (the original traps) disables the cull for that frame;
  - the door overlay (`g_terrain_overlay`) is always drawn.

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

## CLUT depth ramp and near cull: ported (R2)
Every positive family (`0x80023EA8/0x80024F30/0x80025900/0x80026B04`) does
the following:
- **Near cull:** a bit-31-clear face with max SZ < 0xC1 is skipped.
- **Ordering depth (otz):**
  - bit 31 clear: `(max + (sum >> 2)) >> 1`, or `max − 16` with bit 27;
  - bit 31 set: max, or SZ0 with bit 27.
- **Ramp:** unless bit 28 is set, ORs `ramp[(otz >> (9 + LOD)) & 31] << 16`
  into word 3, moving the CLUT k palettes to the right. The ramp is area
  record bytes +0..31 (Areas 1–3: 0 ×10, then 1 1 1 2 2 2 3 3 4 4 5 5 6 7 …).
  So the ramp starts at otz 5120.

**The ramp palettes** (resolved from the original uploads for all Area 1/2
terrain words) are darker copies of the base palettes:
- Mean ratios for k = 1..7: 0.86 / 0.77 / 0.66 / 0.57 / 0.43 / 0.39 / 0.28.
- No single integer rule fits them: (9 − k)/9 floor matches 58% of entries.
- Some variants also flip STP bits.

The PICA has no palette textures, and a texture page per step would cost
about 32 × 7 × 256 KiB. So the cooker fits a least-squares RGB scale per word
and step (L3CR sidecar, `materials/area_NN.l3cr`). The renderer multiplies it
into the face's depth-cued colour.
- **CANDIDATE:** mean error 0.67 of a 5-bit step; 74% of entries round
  exactly.
- **CANDIDATE:** bit-28 faces reuse the previous face's ramp in the original;
  0 here.
- **Not modelled:** STP changes in the variants.

## See-through walls: ported (R3)
Status: **SOURCE_BACKED_PARTIAL**.

**Which families.** Mode table `0x80089890` (positive, negative, ... per mode):

| mode | positive family | see-through rule |
|---|---|---|
| 0 | `0x80023EA8` | no |
| 1 | `0x80024F30` | yes |
| 2 | `0x80025900` | yes |
| 3 | `0x80026B04` | no |

`0x80023C08` draws mode 0's near list (`0x8015BD00`, cells with z < 0x400)
with mode 1, so in mode 0 those faces also use the rule. The port doesn't
model that list: **UNKNOWN** impact, Area 1 is mode 2.

**Rule** (after the depth cue, on a face that passed NCLIP: MAC0 > 0 or
double-sided):
- Gate:
  - prefix bit 29 clear, or min SZ < 0x181;
  - mode 1 also needs bit 24 clear with bit 29 set.

  Faces that fail the gate take the other (bit-29) path.
- Prefix > 0, i.e. bit 31 clear and the word non-zero.
- Max SZ < `0x1F8001C0`, the player's view depth.
- The int16 SXY0..3 box overlaps the centre region: max SX > 0x96 and
  min SX < 0xAA, max SY > 0x78 and min SY < 0xA0.

Then `0x1F80007C` is ORed into the code word. That is 0x02000000 (ABE) while
event byte `0x800C4C48` is 0, else 0.

**Port:**
- `classify_terrain_face` sets `see_through`; `TerrainFaceCamera::see_through_1f80007c`
  follows `source_event_work()` byte 0 when it is known (otherwise the
  gameplay value, **CANDIDATE**).
- `draw_terrain` keeps those faces out of the opaque pass (they sit at the
  back of the drawn index range).
- `draw_terrain_see_through` draws them after the player, actors and effects
  through `plan_scene_e0_material`:
  - STP-0 texels (alpha 255) opaque;
  - STP texels (alpha 128) blended with the TPAGE ABR (bits 5..6 of the
    material word), with no depth write.

  On PS1 a semi-transparent textured primitive blends only its STP texels,
  so how see-through a wall looks depends on its CLUT's STP bits.
- Without a retained STP mask (untextured batch or alpha gate off) the faces
  stay opaque and count as `see_through_held`.

**CANDIDATE / caveats:**
- OT order among semi-transparent primitives is not reproduced.
- The negative family `0x80021B14` writes `0x1F8001C0 = 0x1F80013C &
  0xFFFF0000` while it runs; whether that changes the threshold for positive
  faces drawn after a negative cell is **UNKNOWN**.

**Azahar (Area 1 start, idle autoplay):**
- `see_through=0`, consistent with the rule: the camera looks at the start
  wall's back face, which is skipped and never see-through.
- The rest of the counts are unchanged (259 drawn / 108 black / 72 skipped);
  command buffer 29% of 512 KiB.
- The Area 2 wall spawn did not boot, so no on-screen case was seen.

## Found, not ported (next candidates)
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

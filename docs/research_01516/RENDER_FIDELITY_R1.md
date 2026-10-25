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

## Near LOD and the off-screen test: ported (R4)
Status: **SOURCE_BACKED_PARTIAL**. At the Area 1 start, the host replay
reproduces the PCSX-Redux black-face set exactly.

**Per-IDX LOD (`0x800267AC`; mode 0 uses the same body at `0x80024BAC`).**
- For each IDX bit in BC00/BD00, gte_rt runs on the IDX SVECTOR (x, y, z,
  raw6). MAC3 is shifted right by `0x1F800074`.
- Depth < 0xAF1 takes the Near descriptor stream (+0x18), < 0x1C21 Mid
  (+0x1C), otherwise Far (+0x20). raw6 == 0 adds 3 words (the secondary bank).
- `0x8002196C` uses the same cuts for negative cells. Depth < 0x401 there
  queues the cell for the near replay renderer (`0x800BBE50`), and Mid/Far
  clear bit 15 of the packed cell.

**Off-screen test (`0x80025BE8..0x80025D38`, mode-2 family `0x80025900`).**
It runs after the MAC0 == 0 skip and the double-sided flip, and before the
near cull, depth cue and back-face test:
- a face is kept if any vertex has SX < 320 and SY < 240 (unsigned);
- otherwise it is kept if the midpoint of v0/v3 or of v1/v2 passes the same
  compare;
- otherwise it is skipped (`0x80026220`).

**Host replay, Area 1 start** (retail pack, start camera, cell visibility
with the PS1 side cut):

| Port step | black | faces past the screen test |
|---|---|---|
| Mid only, no screen test | 28 | – |
| + off-screen test | 18 (adds Mid 7433) | – |
| + per-IDX LOD | **17** | **115** = 94 drawn + 17 black + 4 culled |

The 17 black t0 values match the PCSX-Redux probe exactly: 5377×4, 5391×4,
6398, 6412, 6909×3, 6922×3, 7419. The probe also logged 115 hits per frame at
`0x80025E70`, the same as the 115 faces past the screen test. The 144 faces
entering NCLIP (`0x80025B28` hits) match as well.

**Port:**
- `terrain_lod_800267ac` and `TerrainCellVisibility::idx_lod`, filled for
  each visible IDX.
- `classify_terrain_face`: the off-screen test for positive mode 2, with
  `TerrainFaceCamera::screen_x_min/screen_width` (default 0/320). Off-screen
  faces return `off_screen`.
- `main.cpp`:
  - terrain batches hold Mid first, then only the Near/Far geometry that
    differs from Mid (an equal LOD sets a bit on the Mid quads instead);
  - each frame a positive quad draws only in its IDX's LOD;
  - an unclassified draw uses the Mid range only.
- **Deliberate port feature (not a gap):** the 3DS uses the window SX
  −40..359 (400 pixels), matching the widened cell side cut. With it the start view has 25 black faces: the
  PS1's 17 plus 8 in the extra side strips.
- **Not ported:** negative cells still use only Mid, and the near replay
  renderer (`0x8002235C`) is not ported. Area 1's negative cells draw no black
  faces either way.

## GP0 packet translator: terrain routed through PS1 packets (R5)
Status: **SOURCE_BACKED_PARTIAL**. This is the StarHaven-style layer: the
port writes original GPU packets and a citro3d translator draws them.

**Original packet writer (`0x80026158..0x80026218`, mode-2 family):**
- **POLY_FT4, 40 bytes.**
  - Word 1 = `(prefix & 0x02FFFFFF) | 0x2C000000` after DPCS (bit 25 keeps the
    semi-transparent code bit); `0x2C010101` for a back face; OR `0x1F80007C`
    for a see-through face.
  - Word 3 = uv0 | CLUT << 16 (`0x1F800158`) | ramp << 16 (`0x1F800148`).
  - Word 5 = uv1 | TPAGE (`0x1F80015C`); words 7 and 9 = uv2 and uv3.
- **Linking.**
  - Bucket `otz >> 3` from the OT base pointer `0x1F80003C` (AddPrim
    prepend). A back face uses its max SZ (`0x80025EBC`).
  - Prefix bit 28 inserts the packet right after the previous packet (s4)
    instead. The 40-byte primitive pointer is `0x1F800070`.
- **Subdivision (`0x8002602C`).** |MAC0| ≥ 2049, or 513..2048 with
  (x0+x3−x1−x2)/2 or (y0+y3−y1−y2)/2 outside −3..3, goes to `0x8002466C`
  instead of one packet.

**Port** (`include/legends3ds/render/psx_gpu.h`):
- **Library:**
  - `PrimitiveBuffer`, and `OrderingTable` with ClearOTagR / AddPrim /
    DrawOTag order (far buckets first; last added first within a bucket) and
    the bit-28 `insert_after`;
  - `decode_gp0_polygon` for codes 0x20..0x3F;
  - `terrain_poly_ft4_80026158`, `terrain_ot_bucket`,
    `terrain_needs_subdivision_8002602c`;
  - host test `test_psx_gpu`.
- **3DS translator (`main.cpp`, `psx_draw_ot`).**
  - Walks the OT, decodes each packet and resolves CLUT|TPAGE to an uploaded
    page. A CLUT k rows past a page's own CLUT is ramp palette k, which uses
    the L3CR scale.
  - Draws same-state runs from one vertex buffer, with vertices pre-projected
    at w = 1, so UVs interpolate affinely like the PS1 GPU.
  - Depth comes from the 3D camera (a port-side extra after the packet), so
    actors still occlude.
  - Semi-transparent packets (prefix bit 25 or see-through) draw after the
    actors, in OT order, with the TPAGE ABR blend.
- **Terrain routing.** Positive Draw/Black faces become packets. Negative
  cells, unsupported faces and faces the original would subdivide stay on the
  3D index path.
- **Widescreen.** The 400-pixel top screen is a deliberate port feature (no
  pillarbox; user decision 2026-10-24). Packet SX gets +40.
- **CANDIDATE:**
  - OT size 4096;
  - bit-28 "previous packet" is the previous packet of the port's batch walk;
  - vertex x/y currently come from the 3D camera, not the packet's SXY.
    Whole-pixel SXY next to the 3D-drawn subdivided faces left visible
    seams. The SXY↔camera error is traced (1.19 px max at the Area 1 start).

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

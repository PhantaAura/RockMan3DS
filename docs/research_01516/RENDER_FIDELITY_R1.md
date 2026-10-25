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

## Subdivider `0x8002466C`: ported (R6)
Status: **SOURCE_BACKED_PARTIAL**. Decompiled with GhidraMCP; the tables
were read from the EXE.

**What it does:** one level of subdivision, with no recursion.
- **Midpoints.** It halves the world SVECTORs at `0x1F800198..0x1F8001B7`:
  m01, m02, m13, then m23 and the centre = (m02 + m13) / 2.
  - Two RTPTs store the SXY at `0x1F800170..0x1F800183`, after the four
    corners at `0x1F800160`.
  - UVs halve the same way into `0x1F800190..0x1F800199`, but the centre UV
    = (uv m01 + uv m23) / 2.
- **Four POLY_FT4.** Index table `0x80097C90` (copied to `0x1F800120` by
  `0x800176DC` at `0x80023860` and `0x80023BA8`): {0,4,5,6}, {4,1,6,7},
  {5,6,2,8}, {6,7,8,3}. Points 4..8 are m01, m02, centre, m13, m23.
  Code word `0x1F800154`, CLUT|ramp, TPAGE.
- **Edge POLY_FT3s.** Table `0x80097CA0`: {0,1,4}, {1,3,7}, {3,2,8},
  {2,0,5}. Code `0x1F800150` = code & ~0x08. Each is written only when its
  NCLIP > 0, i.e. the projected midpoint bulges past the straight edge; this
  fills cracks against unsplit neighbours.
- **Linking.** The packets are chained contiguously and the chain is added
  at the OT entry `0x1F80013C` (bucket otz >> 3, or s4 for prefix bit 28).
  It returns the last quad, which the caller keeps as s4 unless bit 28 is set.

**Port:**
- `terrain_subdivide_8002466c` (`terrain_visibility`) and `write_poly_ft3`
  (`psx_gpu`);
- `psx_emit_terrain_face` writes the 4 + n chain;
- every positive face is now a packet, so packet x/y is the GTE SXY again,
  which restores the PS1's whole-pixel vertex snapping;
- host tests cover the midpoints, the UV rule, the corner SXY and the NCLIP
  rule; on a flat wall, rounding gives NCLIP up to about 33 and the PS1 would
  draw those slivers too.

**Caveat:** actors are still drawn by the 3D camera. Packet SXY differs from
it by 1–5 px on near faces at the Area 1 start and during a spin, and by up
to 50 px for a vertex at the camera, where the GTE saturates as on the PS1.

## Negative family `0x80021B14` as packets (R7)
Status: **SOURCE_BACKED_PARTIAL**. Decompiled with GhidraMCP.

**Packet writer (`0x80021EA0..0x80022100`):**
- **Face test.** NCLIP ≥ 1 draws. NCLIP < 1 draws only with bit 30
  (flipped); a skipped face without bit 28 clears the chain pointer s1.
- **Code word.**
  - Bit 29 gives a POLY_FT3: RTPT only, `(prefix & 0x03FFFFFF) | 0x24000000`,
    the depth cue from IR0 after vertex 2.
  - Otherwise RTPS of vertex 3 and a POLY_FT4 with `| 0x2C000000`.
  - DPCS always runs and keeps the code byte, so bit 24 (raw texture) and
    bit 25 (semi-transparent) survive.
- **Bucket.** max SZ >> 3 (3 or 4 values). It is OT +8000 bytes when
  `0x1F800078` bit 0 is set and the cell y ≥ 0; that flag is UNKNOWN and
  taken as 0. Bit 28 chains after s1.
- **UV/CLUT/TPAGE.**
  - Special cells (packed bit 7) take the stream UVs and the descriptor word
    before the record.
  - Other cells take table `0x800BADB0` (CLUT, TPAGE 0x1D, UV offset byte)
    with the fixed 0..31 UVs plus offset. Near LOD with packed bit 15 uses
    the stream UVs plus per-face offsets.
  - uv0 word = uv0 | CLUT (`0x1F8001C0`) | ramp (`0x1F800138`); uv1 word =
    uv1 | TPAGE (`0x1F8001C4`).
- **Ramp (`0x8002196C`).** `0x800C0DC0[(cell depth / 512) & 31]`;
  `0x800C0DC0` is the area-record copy made by `0x800371A0`. Cells with
  depth < 0x401 go to the near replay renderer `0x8002235C` instead.

**Port:**
- `terrain_negative_packet_80021b14`, `terrain_negative_ramp_8002196c`;
- `TerrainCellVisibility::negative_depth`;
- triangle handling in `classify_terrain_face`;
- `psx_emit_negative_face`.
- Default negative cells with a known binding are packets. Special cells (no
  binding) and near-replay cells (depth < 0x401, `0x8002235C` unported) stay
  3D. Drawing the near cells as single affine quads smeared the floor next to
  Mega Man, which the near renderer avoids on the PS1.
- **CANDIDATE:** the bit-28 "previous" is per batch walk. Negative LODs are
  ported in R8.
- The resolver falls back to the page's own palette when a ramp CLUT has no
  fitted L3CR scale (`gp0_unfitted_ramp`; 0 at the Area 1 start).

## Near replay `0x8002235C` and negative LODs (R8)
Status: **SOURCE_BACKED_PARTIAL**. Decompiled with GhidraMCP; `0x800221E0`'s
third argument was checked against the instructions (the decompiler dropped
it).

**What the original does:**
- `0x8002196C` picks a negative cell's descriptor from its centre depth
  (MAC3, unshifted): < 0xAF1 Near (+0x18), < 0x1C21 Mid, otherwise Far.
  Mid/Far clear packed bit 15.
- Depth < 0x401 queues the cell at `0x800BBE50`. `0x800221E0` later replays
  it through `0x8002235C` with the Near descriptor, the ramp
  `0x800C0DC0[(depth/512) & 31]` and TR `0x1F80001C`.
- `0x8002235C` writes the same FT4/FT3 packets as `0x80021B14`, with two
  differences:
  - a back face with neither bit 30 nor bit 29 **ends the stream**;
  - special (bit-7) cells go to the subdivider `0x80022A94`, which is
    unported (those cells stay 3D).
- **Near UVs (bit 15 kept).** Stream UVs plus a tile offset ((t & 7)·32 in u,
  (t >> 3)·32 in v). Face i of n uses byte `0x1F800187 − (n−1−i)` while
  n−1−i > 3: the four bytes of `0x800BADB4`, the second word of the
  `0x800BADB0` row, copied to `0x1F800180`. Later faces use the row's tile
  byte. Mid/Far use the fixed 0..31 tile UVs.

**Port:**
- L3NM v2 stores the four near tiles per row; the loader still accepts v1;
  recipe `tutorial-install/01516-l3nm-near-2`.
- `negative_material_near_uv` and `expand_negative_terrain_lod`.
- Negative cells join the Near/Far batch passes (sharing Mid where the
  geometry is equal) and pick their LOD per frame from `negative_depth`.
- Near-replay cells are packets; the stream-end rule is ported; a skipped
  face without bit 28 clears the chain, as in both families.

## Actors in the OT: Mega Man (R9)
Status: **SOURCE_BACKED_PARTIAL**. Decompiled with GhidraMCP; the SZ
store was checked against the instructions.

**How the original draws:**
- **Frame order (`0x80027678`).** Player `0x800B51B0` and a second player
  slot `0x800B57A4`: `0x80027F78`, then `0x8002786C`. Then the E0/effect
  sweeps `0x80029A38`, `0x80029B10` and `0x80029B94`, then `0x8002CB18` and
  `0x8002D50C`.
- **`0x80027F78`:**
  - RTPS of the player point (+0x3A8); the stored value is SZ3 >> 2
    (`0x80028018`);
  - colour `0x1F8000E4` = 0x24808080 after DPCS, with a damage flash from
    `0x800BBCF8` and | 0x02000000 while `0x8005EBD4` is nonzero;
  - ramp `0x1F8000E0` = `0x800C0DC0[(SZ/4 >> 7) & 31] << 16`;
  - returns 0 (not drawn) beyond `0x1F800006`;
  - splices the 512-entry player OT (`0x8009BF88` + frame·0x800, OTZ ±256)
    into the main OT `0x800A4D40` (4096 buckets), two entries per bucket.
- **`0x8002786C`.** Five part groups (tables `0x800898EC` / `0x80089928` by
  player +0x170/+0x171). Per part it composes the camera with the bone at
  +0x1C8 + i·0x20 and the offset +0x3A8 + i·8, then calls `0x80027BE0` (tris)
  and `0x80027D94` (quads).
- **Packet writers.** They patch packets pre-built in the mesh, one set per
  frame buffer (68-byte FT3 and 84-byte FT4 records):
  - NCLIP > 0 only;
  - word 1 = `0x1F8000E4` (| 0x08000000 for a quad);
  - CLUT bits 0..2 = ramp;
  - bucket = AVSZ3/AVSZ4 OTZ minus the player OT base.
  - InitGeom sets ZSF3 = 341 and ZSF4 = 256, so OTZ = average SZ / 4, and the
    main bucket = OTZ / 2 (the same scale as terrain's z >> 3).
- **E0/effect mesh writer `0x8002A6E8`** (decoded, not yet routed):
  - FT3/FT4 with code modulation | 0x24/0x2C;
  - flag 0x40 skips NCLIP and flag 0x08 skips the depth cue;
  - bucket AVSZ OTZ >> 1; CLUT | ramp `0x1F8000E0`.

**Port:**
- `gte_avsz3/4`, `player_packet_80027be0`, `actor_ramp_80027f78`;
- `psx_emit_player`, which projects the posed world points with the frame's
  GTE camera;
- the resolver also finds player pages (CLUT & ~7);
- the OT is flushed after the player.
- **CANDIDATE:**
  - projection of rounded posed world points instead of per-part composed
    GTE matrices;
  - player OT sub-order and the splice position are not reproduced (main
    bucket only);
  - the damage flash/semi branches and the far clip are not modelled.

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

# Missing player hair: shared type1 source-image layout defect

The imported H01 mesh was complete, but the texture decoder misread its texture
image. It treated all source image sectors as one row-major image. The original
loader writes each 2048-byte image sector as a separate **32 VRAM words by32
rows** rectangle, advancing horizontal tiles before vertical tiles. Thus cached
bytes could match source bytes exactly while the cache still represented the
wrong pixels at a given UV. The prior cache-byte equality and PICA/tex3ds swizzle
checks did not validate this earlier source decoding boundary.

## Source chain and retained proof

The exact owner executable SHA-256 is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
No commercial code or resource bytes are included in this note or its probe.

- Resource dispatch table80010284 selects type1 header handler8001BD98.
- Handler8001BDC8..8001BE00 preserves image X/Y/width/height into metadata
 800C560C+16/+20/+24/+28. Its no-CLUT-copy branch still initializes the upload
 RECT at8009BE48 to width32,height32 at8001BEEC..8001BEF4.
-8001C28C..8001C2A0 sets loader stage800989C8 to resource type+1, hence2.
 Stage dispatch table8001025C[2] points to8001C350.
-8001C350..8001C3A8 forms the RECT destination from imageX+tileX*32 and
 imageY+tileY*32, then calls original LoadImage8007D2F0. Its source pointer is
 the buffered sector address800B5DB0+(sectorCounter800987A8<<11).
-8001C3AC..8001C408 increments horizontal tile counter800C5634 until
 imageWidth>>5, clears it, then increments vertical tile counter800C5638 until
 imageHeight>>5. This proves the source block order and upload size independently
 of the corrected decoder.

`type1_sector_source_probe.py` executes unchanged owner instructions on six
authored image sizes, observing every RECT/pointer at the LoadImage boundary and
resuming at its existing return site. `type1-sector-source-proof.json` retains
the complete scalar result. No instructions are replaced and no helper hooks
are installed. The probe isolates image layout through the original explicit
no-CLUT-copy branch; it does not exercise GPU rasterization, live retail load
chronology, palette ownership or complete hardware parity.

An initial local probe address calculation incorrectly retained the LUI upper
half without signed ADDIU displacement and failed an assertion before producing
evidence. Correcting the three local authored work addresses to80098xxx/8009Bxxx
made the retained probe pass. No owner instructions or decoder logic were changed
to make that proof pass.

## Symptom and correction

The existing area0 cache sampled index0 (transparent) at52 of86 sampled UV
centroids in original head part1. Applying the original sector placement reduced
that count to0. For example the original UV locations(215,237),(216,238) and
(215,238) changed from index0 to index7; location(199,217) changed from0 to3.
This is a diagnostic scalar sample, not a whole-face coverage or raster parity
measurement. The source data/UV/CLUT/mesh remain unchanged.

`parse_mml_type1_texture` now canonicalizes source sector tiles into row-major
little-endian VRAM words once, before neutral texture cooking or stateful material
composition. The conversion leaves BPP, word bits, palettes, upload order and
UVs intact. No V inversion or borrowed palette is added. This is shared by
terrain/player/actor texture consumers rather than a special head patch. Whole
32-word/32-row tiles and physical1024-word/512-row VRAM bounds are required;
partial tile shapes remain unsupported and fail explicitly.

The new authored test covers12 complete4/8bpp layouts, including single tiles,
horizontal/vertical grids and2x8-sector images. Every canonical word, unpacked
pixel, neutral cooked image and stateful material-cache pixel is checked against
independently authored coordinate colors. It also exercises prefixed resource
offsets and14 malformed shapes/bounds/sizes. The inherited tiny synthetic type1
fixture was corrected to one full source sector; neutral in-memory fixtures can
still express smaller rectangles. Native visual confirmation and release results
are recorded by the root report separately.

Reproduce the scalar source proof with:

```sh
python3 -B docs/research_01427/type1_sector_source_probe.py --exe /path/to/ROCK_NEO.EXE
ctest --test-dir /path/to/host --output-on-failure -R '^(texture_sector_layout|material_vram|core)$'
```

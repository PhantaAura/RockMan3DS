# First imported actor material connection

Status: SOURCE_BACKED_PARTIAL for owner material/UV identity and native binding;
shade, semitransparency and a same-frame retail VRAM comparison remain UNKNOWN.

The read-only authored probe `first_enemy_material_probe.cpp` inspected the
existing owner-local area01 EBD row0 class20/subtype03. All five part descriptors
retain material `0x3cc00027`. Its60triangles and49quads have identical matching
TPAGE/CLUT bindings; no bindings or UVs are lost in the decoder. Neither the
inherited terrain cache nor player cache contains this word. The01415 native
actor draw unconditionally selected flat color and supplied no face UVs.

Owner resource `ST00_01.BIN`, `..\OBJ\ST00\AR01\EM0300.TIM`, header offset159744,
matches the page/CLUT:4bpp256x128, image word origin(448,0), CLUT(0,243),16colors
and8stored palette banks. The existing cooked texture is byte-identical to the
current decoder's in-memory cooking of this owner resource; SHA-256 is
`f702e0d448f6161215cfcbf520045e73632d3262f330719bcdc6bc46f535288b`.
The area02 cooked resource is identical. This identifies actual supplied data;
no external texture or palette was used. The author-maintained MML1 texture
mapping was only a research clue, confirmed against the supplied bytes.

The inherited ordered VRAM resolver supplies all16requested palette words and
8192of16384image words. The remaining half-page is explicitly unwritten. The
first part includes source V128; the renderer preserves that original coordinate
and the existing zero-filled page, rather than clamping to row127 or borrowing
another image. A live original VRAM comparison is still needed to establish
whether another runtime upload owns that boundary. Palette/timeline changes
during gameplay also remain outside the static-cache proof.

`actor_material` uses the same neutral L3MC pages, palette identity and ordered
VRAM composition as terrain/player caches. The cooker collects actual material
words only from the class20/03 and class20/35 resource profiles already admitted
by the runtime. It does not assign guessed resources to other actor classes.
The native draw keeps source face UVs associated with their original vertices,
retains the PS1 quad split(0,1,2;1,3,2), and groups only consecutive same-material
parts, preserving their order. Tick/draw code performs no new heap allocations.

`sdmc:/3ds/Legends3DS/actor_render_01416.cfg` with exact `ACTOR_TEXTURES=1` enables
the independent owner diagnostic. It defaults OFF and requires the enhanced UV
renderer;0143 is unchanged. Textured actors use white unlit modulation to inspect
the supplied image, explicitly labelled a diagnostic. This is not a reconstruction
of original shade or lighting. Missing, malformed, unresolved, over-budget or
failed-upload actor sidecars preserve the flat gray presentation and boot.

Optional `materials/actor_area_XX.l3mc` caches are area-local. Old textures are
released through the existing all-render-resources teardown on transitions,
rollback/rebuild and shutdown. The actor cache has64entry and1MiB decoded-element
limits and shares the existing8MiB combined terrain/player/actor GPU page policy.
Input buffers, allocator overhead and two transient262144-byte RGBA/tiled pages
are additional; these policy limits are not total hardware RAM measurements.

The incremental command reads only already supplied files, copies a verified pack
to a NEW directory, and cooks just five optional actor caches. It re-verifies the
old pack and preserves all previous output hashes:

```sh
python3 -B tools/add_actor_material_caches.py --pack EXISTING_PACK --dat OWNER_CDDATA_DAT --host-build HOST_BUILD --out NEW_OWNER_LOCAL_PACK --result RESULT_JSON
```

No cooked pixels, palettes, EBD bytes or models are placed in this source tree.
`first-enemy-material-audit.json` contains scalar metadata and hashes only.

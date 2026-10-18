# Flight Sprint 0.0.14 — Stateful ST00 Material Cache Foundation

0.0.14 starts the first faithful texture/material bridge for the native 3DS renderer. The checkpoint deliberately stops before claiming hardware-textured terrain because the validation container does not have devkitARM/libctru/Citro3D. What is completed here is the portable owner-side/runtime material representation needed to make that hardware binding deterministic instead of approximate.

## Why the cache is per-area

The original PlayStation renderer does not treat a texture-page address as a globally unique asset ID. VRAM is stateful: common ST00 resources upload pages first and an area-specific resource may overwrite the same VRAM coordinates later. For example, the area-04 source contains an image upload at a page coordinate also used by the common ST00 source. Therefore a global `material_word -> first matching source image` table is not fidelity-safe.

`resolve_material_word_last_writer()` takes an upload-ordered resource list and resolves the image page and CLUT row from the last matching write. `l3cook_material_cache` applies this to the material words actually reachable from an area's positive L3MP cells. The resulting cache is independent of host pointers and can be loaded directly by the portable runtime/3DS backend.

## L3MC v1

Each resolved material record preserves:

- the original 32-bit material word;
- TPAGE-derived 4bpp/8bpp depth and ABR mode;
- source image dimensions;
- a padded 256x256 palette-index page so original byte UVs can remain usable;
- the exact raw PS1 BGR555 palette row (16 or 256 entries).

Unresolved records carry the material identity but no guessed payload. The file has fixed records, explicit offsets/counts, and a payload CRC. `material_page_rgba8()` exists as a backend helper; PS1 STP is retained as distinguishable alpha in this intermediate conversion and is **not** a claim that final PS1 semitransparency/blending parity is solved.

## Exact owner-data result

Using the reconstructed AA+AB extracted Mega Man Legends (USA) corpus and the tutorial load-order model:

| Area | Unique Mid-LOD materials | Resolved | Unresolved |
|---|---:|---:|---:|
| 00 | 8 | 5 | 3 |
| 01 | 10 | 9 | 1 |
| 02 | 10 | 10 | 0 |
| 03 | 3 | 3 | 0 |
| 04 | 2 | 2 | 0 |

The unresolved identities are only:

- `0x7C810016`
- `0x7C830016`
- `0x7CC20017`

These correspond to the same three dynamic/runtime palette cases already isolated by the earlier material research. A whole-disc scan can find later resources at the same CLUT coordinates, but those matches are not accepted because unrelated stages reuse PS1 VRAM. The correct next fidelity task is to recover the original runtime palette-copy/write behavior from the USA executable/overlay, not substitute a visually convenient later-stage palette.

## Cooker/runtime integration

New pieces:

- `include/legends3ds/material_cache.h`
- `src/texture/material_cache.cpp`
- `tools/l3cook_material_cache.cpp`
- `tools/l3inspect_material_cache.cpp`
- synthetic last-writer/cache round-trip tests in `tests/test_core.cpp`
- `build_tutorial_pack.py --with-material-cache`
- material-cache inspection in `tools/flight_qa.sh`
- portable material-cache sources included by the 3DS Makefile

A fresh full tutorial QA cook now produces 96 owner-side files and all player/runtime/render probes still pass. Source safety scanning confirms no Capcom or cooked owner data is present in the source tree.

## Not claimed yet

0.0.14 does **not** claim:

- Citro3D terrain texture upload/submission on hardware;
- the three dynamic CLUT writes are recovered;
- exact PS1 semitransparency/ABR blending;
- all later-stage load orders are known;
- the tutorial is 0.1-complete;
- a compiled `.3dsx` exists from this environment.

The next renderer pass should bind the resolved per-area L3MC pages to Citro3D, preserve material/UV identity through draw batching, and keep the three unresolved dynamic CLUT materials visibly/diagnostically separate until their retail write path is proved.

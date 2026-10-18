# Cooked ST00 terrain topology audit — 2026-09-30

The decoded terrain does **not** justify a uniform PS1 strip triangulation. Restoring the enhanced terrain renderer's baseline fan is a containment change; this audit does not establish uniform fan triangulation or original-game fidelity.

The standalone analyzer used the existing `validated-d/host/liblegends3ds_core.a`; the project was not rebuilt for this audit. It read only the authorized cooked `st00.l3render` and `st00.l3map` through `load_l3render`, `load_l3map`, and `expand_st00_descriptor_lod`, with CRC validation enabled. It reproduced the frontend's positive-cell, Primary/Mid, zero-extra-scale expansion for the first five areas. No raw game files were inspected, no cooker or extractor ran, and no game assets or application source were changed. No original vertex coordinates are included in the output.

For each decoded four-point X/Z record, the analyzer calculated unique points and a convex hull, then compared two triangle lists:

- Strip: `(0,1,2)` and `(1,3,2)`.
- Baseline fan: `(0,1,2)` and `(0,2,3)`.

Coverage passes only when the triangles have zero intersection area and their union equals the hull area, within a relative numerical tolerance. This catches matching overlap and missing area that a sum-of-triangle-areas check can miss. Synthetic strip-order and perimeter-order squares self-check the hull and coverage calculations before cooked data is read.

For **strictly convex four-vertex hulls**, coverage is conclusive about whether a triangle list fills that decoded geometric quad:

| Expanded Primary/Mid area | Strict convex records | Strip only | Fan only | Both | Neither |
|---|---:|---:|---:|---:|---:|
| 0 | 857 | 249 | 540 | 0 | 68 |
| 1 | 353 | 127 | 201 | 0 | 25 |
| 2 | 212 | 88 | 111 | 0 | 13 |
| 3 | 44 | 10 | 30 | 0 | 4 |
| 4 | 2,156 | 84 | 144 | 0 | 1,928 |
| All five | **3,622** | **558** | **1,026** | **0** | **2,038** |

The stored source primitive records, before world expansion or area repetition, also contain mixed topology: of **717** strict convex records, **235** pass strip only, **400** pass fan only, and **82** pass neither. Stored records are counted once per stored stream; identical geometric shapes are not deduplicated.

Across all **15,512** expanded records, **11,850** have a triangular convex hull and **40** have zero hull area; **368** have only three unique points. None meet the analyzer's axis-aligned rectangle criterion. The general hull comparison is diagnostic only for triangular hulls, interior points, or concave orderings: the original intended shape need not equal the convex hull. These records therefore do not establish that the renderer should submit three vertices or reinterpret retained compressed bytes as heights.

The result supports keeping the independently documented PBD player strip correction separate from the unverified proprietary ST00 terrain order. It does not identify the original ST00 coordinate gathering, height semantics, primitive classification, or UV ordering. Those questions remain unresolved.

Full aggregate output: `terrain_topology_audit.txt`. Reproducible analyzer source: `terrain_topology_audit.cpp`.

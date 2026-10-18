# Cooked material audit — 2026-10-01

The existing cooked tutorial pack has **1,184 expanded terrain primitives with unresolved materials**, out of **15,512** Primary/Mid primitives across five areas. The inherited native batching path retains their geometry and submits it as opaque untextured triangles. Missing material resolution therefore does not, by itself, remove these triangles.

| Material word | Area 0 primitives | Area 1 primitives | Exact cooked CLUT matches |
|---|---:|---:|---:|
| `0x7c810016` | 288 | 0 | 0 |
| `0x7c830016` | 176 | 0 | 0 |
| `0x7cc20017` | 360 | 360 | 0 |

All **39 cooked indexed textures and 30 cooked palettes** passed the existing CRC readers. Exact image-page matches exist for the three unresolved words, but none of the 69 resources matches their CLUT address and depth. Dynamic palette ownership, upload timing, and surrounding VRAM state remain **UNKNOWN**. No replacement palette is supported by these inputs.

For resolved materials, **136 area-0 primitives** have at least one transparent UV-corner sample, all under material `0x7c800016`. No resolved primitive has four transparent corner samples. This measures the four stored UV endpoints only; it does not measure interior texels, prove intended transparency, or establish rasterized coverage.

Across areas 1–3, **358 primitives** have at least one UV corner outside the retained width of a 128-pixel-wide image: 84 in area 1, 222 in area 2, and 52 in area 3. These sampled corners are opaque in the existing caches. The cache pads a material page with index zero, which may select an opaque palette-entry-zero color. This is a texture fidelity concern; reconstructing the original surrounding VRAM content requires additional evidence. Clamping UVs, reusing another palette, or treating all padded indices as transparent would invent behavior.

The audit inspected the inherited native material batching, shader, texture environment, alpha gate, and cache readers. The batching path creates VBOs before attempting material texture upload. Unresolved entries use primary color with alpha one; the inherited alpha test accepts them. Resolved color zero is transparent, while `0x8000` remains visible. PS1 semitransparency blend behavior is still incomplete.

Only already authorized cooked files and supplied source were read. Existing 0147 host readers performed CRC validation and expansion; no extraction, cooking, raw owner-data access, palette substitution, or asset modification occurred. The counts are host observations, not retail equivalence or an Old 3DS performance result. The current 0148 emulator comparison is separate and this audit claims no visual success.

Machine-readable counts, probe commands, and evidence hashes: [material_audit_0148.json](material_audit_0148.json).

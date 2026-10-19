# 0.0.15.8 — what the door IDX swap changes on screen

Question: when the ST00 door object swaps a door cell's IDX descriptor
(`0x8003AC24`, word0 & ~0x1000; state 2 `0x801126DC` writes the open word,
state 5 `0x80112968` restores it), does the terrain the cell draws change?

## Owner-local data check (cooked boot package; not committed)
Read with scratch probes linked against `legends3ds_core` over
`areas/boot.mmlpkg` (the same package the 3DS boots). Only indices and counts
are recorded here.

| Cell | Closed desc (word0) | Open desc (word0) | Collision list (alt) | Render records (slots 0–5) | Streams |
|---|---|---|---|---|---|
| 123 (Area 1 door) | 32 (0x61900) | 28 (0x60900) | 16 → 12 | 80 81 82 … → 68 69 70 … | 71–74 in both |
| 149 (Area 2 door) | 33 (0x61902) | 30 (0x60902) | 17 → 14 | 83 84 85 … → 74 75 76 … | 79–82 in both |

- The closed and open render records are different records that point at the
  **same** primitive streams, with equal flags, material count and stream
  code/shift/slot. Expanded with the native positive-terrain decoder, both
  descriptors give byte-identical primitives (positions, UVs, material words)
  in Near (28), Mid (10) and Far (10) LOD and in both banks.
- The two descriptors differ in their collision list (`alt_contact_index`),
  which is what lets Mega Man walk through.

## Consequence
- The 0.0.15.8 renderer patch (collapse the baked IDX vertices, draw the live
  descriptor from an overlay) is correct for any descriptor swap, but for
  these two doors it draws the same slab again. The swap does not open the
  doors visually in the native renderer. Since both records share the streams,
  the original terrain draw likely does not change either (CANDIDATE: assumes
  no per-record field the native decoder ignores selects primitives).
- Therefore the visible door opening must come from something else, most
  likely the door object's own draw: state 2 rotates by +11 << 8 per frame
  (`0x801126DC`). Whether the slab the 3DS shows in the doorway is terrain the
  original also draws (with the leaf model in front) or something the original
  hides is **UNKNOWN**. Next: read the door object's draw/model owner
  (0x8011C928 table neighbours, the object's model pointer) and whether ST00
  terrain records carry a primitive mask the native decoder ignores.

## Labels
- VERIFIED (owner-local data): the descriptor numbers, collision lists, and
  shared streams above; identical native expansion.
- CANDIDATE: only descriptors with word0 bit 0x1000 are swapped (the 3DS
  patch tracks only those; 85 identities per area on the replay).
- UNKNOWN: door leaf model, its rotation (`0x801126DC`) and how the original
  makes the doorway look open.

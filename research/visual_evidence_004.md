# Visual evidence checkpoint 0.0.4

This file contains derived reverse-engineering notes only. It does not include Capcom game data.

## Player composition is no longer ambiguous

The player renderer at `ROCK_NEO.EXE` `0x8002786C` iterates five PBD component slots. The executable tables used by that loop are:

- component PBD pointer table: `0x800898EC`
- per-component mesh counts: `0x80089928`
- mesh-entry -> transform lookup: `0x80089938`

Baseline/tutorial component counts are `6, 4, 3, 3, 2`, for 18 mesh entries total. The corresponding transform IDs are:

```text
body:      0, 8, 9, 10, 12, 13
head:      1, 1, 1, 1
right arm: 2, 3, 4
left arm:  5, 6, 7
feet:      11, 14
```

The baseline PBD part IDs in those slots are:

```text
body:      0, 8, 9, 10, 12, 13
head:      1, 15, 16, 17
right arm: 2, 3, 4
left arm:  5, 6, 7
feet:      11, 14
```

Therefore static head subparts `15, 16, 17` all use transformed matrix `1`, exactly like head part `1`. `L3PC` v1 records this relationship without pretending those static subparts are extra skeleton transforms.

## ST00 first descriptor triplet is distance LOD

The map renderer around `0x8002196C` selects among descriptor pointers `+0x18`, `+0x1C`, and `+0x20` using transformed depth:

```text
depth >= 0x1C21 -> far  (slot 2)
depth >= 0x0AF1 -> mid  (slot 1)
depth >= 0x0401 -> near (slot 0)
depth <  0x0401 -> near data is queued for the near/replay path
```

The later near/replay path uses descriptor `+0x18` again. `L3RG` v3 therefore names slots `0..2` as the primary near/mid/far LOD bank instead of calling all six pointers arbitrary “sides.”

The second triplet `+0x24..+0x2C` is preserved as a secondary bank because its switch condition has not been proven. On ST00, primary and secondary triplets are byte-for-byte pointer-identical for 115 of 116 descriptors. The one exception is descriptor `0x801940C0`.

## Type-4 CLT resources are real palette sources

Checkpoint 0.0.3 handled compiled type-1 textures and type-9 palette patches. ST00 also contains compiled type-4 `.CLT` palette resources. The resource `..\\SCR\\st00\\tcc100.CLT` has destination CLUT `(x=16, y=496)` and directly resolves material `0x7C010015`.

Including parser-validated type-4 CLTs changes the tutorial-relevant loaded-set result from `381/393` material references complete to `388/393` complete. Three unique material words remain unresolved (`5` references total).

A whole-disc scan finds unrelated later-stage resources occupying the same PS1 VRAM CLUT coordinates. Those are **not accepted as proof** of ST00 palette ownership because PlayStation stages reuse VRAM addresses. The remaining three palette sources require a loader/copy/runtime trace rather than a global-coordinate guess.

## Backend-neutral visual bridge

The host visual bridge now joins:

- five PBD component meshes
- executable-proven `L3PC` transform attachments
- ST00 HED descriptor pointers
- executable-proven primary LOD selection
- ST00 world/rotation expansion

Real-data validation produces 18 Mega Man mesh parts and 316 player primitives (`116` triangles + `200` quads; `1,148` vertex references).

A structural all-area ST00 bridge probe using a constant synthetic mid-range depth (not an actual camera frame) resolves all `2,892` positive cells and expands `15,512` mid-LOD terrain primitives. This probe also caught and fixed an integration bug: HED descriptors are resolved by their source pointers, not by assuming `(pointer - STG_base) / 48` is the vector index.

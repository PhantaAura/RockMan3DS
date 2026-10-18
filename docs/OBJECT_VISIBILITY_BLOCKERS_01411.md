# Imported object visibility evidence — 0.0.14.11

Status: **SOURCE_BACKED_PARTIAL**. No imported object has a proven complete placement → initializer → geometry/material → animation → native draw binding. The separate player renderer is excluded from this statement.

The five tutorial areas retain 49 source records, with counts `1,16,20,11,1`. Placement fields, first-free pool allocation and 46 generic work mirrors are reconstructed; the three class-00 records follow a separate work path. EBD class/subtype matching yields 3 unique, 32 ambiguous and 14 common/no-local candidates. Raw source byte +3 narrows some candidates, but is not a universal runtime variant selector. Neither family nor list order establishes an actor's identity or whether it should be visible.

The current blockers are concrete:

- `src/world/ebd_directory.cpp` cooks directory rows and three payload-relative resource offsets only. The loader requires the file to end after those rows. The pointed resource payloads are absent from `.l3ebd`; a unique directory row therefore cannot provide geometry or animation.
- `StageRuntimeInstance` retains source records, allocation, optional seed work and EBD candidates. Initializer ownership, resource-pointer roles and subsequent work-field updates are unresolved. The known dynamic selector routine does not establish which imported records use it.
- `tools/build_tutorial_pack.py` cooks the five player model components and player animations. Imported object presentation resources have no established cooker/binding path. The native frame submits terrain, player and projectiles, with no imported object draw binding.

A bounded owner-executable check adds two anchors. A measured pointer at `0x80089a78` targets `0x80047318`, within the consecutive pointer region `0x80089a58..0x80089a84`. Its owner/index semantics remain **UNKNOWN**; the pointer's position is not a subtype mapping. The region SHA256 is `92e11ad15b5ddf6ffda6489129bdf8707f8a874a0a82174152eaeb75f5d4aabb`.

The E0 pool callback walk at `0x80029b94..0x80029c14` uses 32 slots at `0x800bf118`, stride `0x50`. Mechanically, it tests `(work[0] & work[6] & 2) != 0`, uses byte offset `work[0] & 0x1c` from `0x800898d0`, and passes `work+0x14` to the callback. The zero-offset target is `0x80029c18`. All 35 current E0 seed mirrors have `work[0]=1`, so the fresh seed cannot pass this callback gate. These observations do not assign semantic flag names to work[0], equate its later value with a source identity, or establish visibility after initialization. The walk SHA256 is `dfb6537a80184014902a03e8501914ff2e30713ac9ed747298add9a91924fc5b`.

The next proof must connect a specific source record to its actual initializer, establish the selected resource roles and retained presentation data, and reproduce the initializer's runtime fields before enabling a native draw binding. Unknown formats or semantics remain unknown; no stand-in geometry is justified.

Reference identities: executable SHA256 `10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`; tutorial overlay SHA256 `9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`. This report contains addresses, hashes, source behavior and aggregate counts only.

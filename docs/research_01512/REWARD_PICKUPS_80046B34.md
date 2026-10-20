# Enemy-reward pickups (0.0.15.12)

Evidence: USA `ROCK_NEO.EXE`. Ghidra 12.1.4 headless decompiled the functions;
`0x80046B34` has no function in the older export. The collection test
`0x80046A0C` and every `rand` call site were checked against the R3000
instructions. Tables are cooked from the owner's executable (`L3RW/1`) and are
never compiled in.

## Recovered — SOURCE_BACKED_PARTIAL
- **Wrapper `0x80047228`:** the stat record's first word becomes the
  `0x80046B34` descriptor, for example `0x01010202` for the class 20/03
  Reaverbot.
- **`0x80046B34` descriptor:**
  - byte 0 = shard count, using size table byte 1 (`0x8008A5A8`, 16 entries);
  - byte 2 = energy count, using size table byte 3.
- **`0x80046B34` RNG and launch:**
  - The first `rand()&0xFFF` is the shard base angle, stepped by
    `0x80097EFA[count]`.
  - Energy pickups start at that angle + 0x800.
  - Each pickup draws a discarded `rand` and a size roll. A size ≥ 5 frees the
    effect.
  - Velocity is (rcos·(speed+r&3)>>12, lift+r&3, rsin·(…)>>12), with
    acceleration Y = 5.
  - Shards start 0x14 higher than the defeat point.
  - Colours are 0x02808080 (shard) and 0x02202020 (energy).
  - Model addresses are `0x8009A6C0+20·size` and `0x8009A710`.
- **States (`0x8004602C`, table `0x8008A6A8`):**
  - 0: spawn, sound 0x9B.
  - 1: flight with `0x8004B5CC` contact (`0x8004B6AC`, bounds `0x8008A618`)
    and the `0x8004AC8C` bounce. On a floor hit or 255 contacts, Y velocity is
    halved; once (vy>>1) < 2 it rests.
  - 2: rest for 150 frames on the `0x80048EA4` floor.
  - 3: blink for 30 + (rand&7) frames.
  - 4: free.
  - 5–7: on collection, rise, shrink, then sound 0x9D (shard) / 0x9E (energy).
- **Collection `0x80046A0C`:**
  - The window uses unsigned comparisons: |dx|, |dz| ≤ radius+50, and
    −radius ≤ (player y − pickup y) ≤ radius+150. Ghidra drops the Y term.
  - Shards add `0x8008A678[selector·8+size]` to money `0x800C1B2C`, capped at
    999,999 (`0x80043294`).
  - Energy adds `0x8008A680[…]` to player HP +0x6E, capped at +0xB0
    (`0x80043340`).
- **RNG:** seven original call sites are admitted on the shared
  `SourceRngService`: `0x80046BAC/BF0/C30/DD8/E18`, `0x80046480` and
  `0x800468EC`. The stream is still the CANDIDATE shared stream; its
  chronology against competing consumers is not proven.

## Not ported
- **Rendering:** the pickup model (`0x8002F9C4` resource at +0x14 from the
  stage list `0x8016C000`), the rotation/scale matrices and the energy sparkle
  sprite (`0x80097EC0`). The 3DS draws a FRONTEND_DIAGNOSTIC diamond instead.
- **Per-frame hook:** `0x80046F38`, reached through the gp+0xE6C frame check.
- **Size 4:** not ported; it holds.
- **`0x8004AC8C` with both X and Z hit:** a decompiler-literal port.

# Source Buster callback and imported-enemy damage

Checkpoint 0.0.14.50 adapts the recovered default Buster route into the
existing native runtime. The supplied Ghidra C export is research evidence, not
compiled source. No executable bytes or decompiler output are distributed here.

## Verified original boundary

- `0x80034EB8` selects a target and calls `0x80035724`.
- `0x80035724` initializes one record in the fixed 16-entry, `0x10c`-stride
  Buster pool.
- `0x80017154` dispatches active records by subtype.
- Default subtype `0x80` calls `0x800433C0`, whose normal states are
  `0x800433FC`, `0x800434F0`, and `0x800435BC`.
- State 0 establishes an 11-frame timer, releases the selection lease, requests
  cue `0x96`, and registers a two-endpoint producer through `0x800435F8`.
- That producer is `0x28000008`. Shared pair traversal writes a packet; it does
  not directly subtract enemy HP.
- The sampled tutorial class-20/subtype-03 consumer subtracts the low 24 bits,
  so this admitted packet removes eight source HP units and selects a reaction.
- Actor or map contact enters state 2 through impact helper `0x80043740`, which
  allocates effect subtype `0x84`; the terminal state clears owner `+0x198` and
  marks the shot for deferred release.

## Native integration

`ChildRuntime::tick_source_busters_80017154` uses the existing child pool,
source-hit proxy bank, effect pool, audio queue and lease owner. The normal
native source-actor path supplies current player work, pose/muzzle output and
the existing imported-actor proxy. Pair traversal stages the original packet for
the enemy's next callback. The class-20/subtype-03 hit and reaction modules are
reused rather than duplicated.

The focused regression follows this transaction:

```text
80035724 allocation
  -> subtype80 state0 / segment registration
  -> shared proxy overlap / packet 28000008
  -> class20/03 20 HP to 12 HP
  -> effect84 / state2
  -> deferred release
```

The callback path performs no heap allocation.

## Explicit limits

- The code that writes player `+0x400/+0x404` and `+0x418/+0x41c` was not
  identified. Live native source mode marks its forward endpoint as
  `CANDIDATE_FORWARD_RAY`.
- The owner and layout of shot map-collision bounds at `+0x78` remain unknown.
  Native source mode passes a clear map receipt instead of inventing bounds.
- The later animation callback that asserts fire-producer bits `0x10/0x20` at
  player `+0x174` remains unknown. A native Y-button edge exercises the bridge,
  but is not claimed as exact retail cadence.
- Zero-HP reaction reaches the pre-existing `NeedsDefeatLifecycle` hold. Exact
  despawn, reward and progression ownership are still unresolved.
- The result is `SOURCE_BACKED_PARTIAL`, not `VERIFIED_1TO1`.


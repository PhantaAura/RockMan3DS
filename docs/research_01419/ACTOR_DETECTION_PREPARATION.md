# Exact helper composition for native source detector updates

Status: **SOURCE_BACKED_PARTIAL**. The high-level preparation module removes
phase1's authored-helper dependency: it computes original normalization,
interpolation and supported contact queries from actual input/reference data.
It does not claim full actor AI, retail replay parity, sound playback or
physical Old 3DS performance.

`prepare_actor_detection_800364b0` accepts an immutable persistent detector work
block plus explicit source inputs, area map, normalization reference and bounds.
It returns a staged `before`/`next` pair, fully computed detector inputs/result,
helper status and fixed-size diagnostics. It never modifies the caller's work
or allocates. Only `Ready` may publish `next`. The caller must also preflight
behavior, pose, notification and sound queue owners before publishing the whole
actor transaction. `actor_detection_preparation_matches` compares every
persistent source detector field before publication; actor/player inputs,
map lifetime and reference lifetime remain caller-owned.

No externally supplied phase1 premise is accepted: the preparation function
clears `source.phase1` and computes its own. Source branches that do not call
helpers do not require their dependencies. An absent profile still holds;
unsigned range rejection, phase0/2 and phase1 coordinate rejection can proceed
without a map or normalization lookup when those owners are unused.

Admitted phase1 follows the recovered original order:

1. Calculate signed32 player-origin differences and preserve the source
   inclusive `[-26750,26750]` checks. A rejection runs the original scalar
   reducer's phase reset rather than attempting geometry.
2. Run `source_normalize_8007A694` using actual owner lookup data, then
   `source_detection_interpolate_80033F04` using the retained vector354.
3. Compute `(signed16(interpolated)*25)>>10`, preserving arithmetic shifts,
   step halfword narrowing and wrapping origin halfword addition.
4. Query updated origin against itself with original class argumentFFFF.
   If nonzero, skip the second query exactly as the original does.
5. Otherwise query midpoint against itself, where midpoint is oldorigin plus
   arithmetic `signed16(step)>>1`, again with wrapping halfword addition.
6. Supply those real helper results to the scalar state machine. Missing
   scratch mode/global, unproved math domains or unsupported contact families
   hold all persistent detector fields unchanged.

The first SH4 lane3 matches original packed profile/phase bytes. The second
source scratch lane3 is unowned and unused by the proved query; it is an
explicit unused zero in native diagnostics and does not drive query behavior.
There is no generic raycast, approximate normalize, manufactured clear sight
or substituted mode. Bounds must have proved ordered pairs; absent/unordered
bounds hold before querying.

## Full original composition proof

The read-only probe executes the original `8003648C/800364B0` CPU instructions
and real `8007A694`, `80033F04`, `80049C54` helper bodies. Its restricted GTE
adapter covers the source's exact reached command fields. **No normalization,
interpolation or contact return hooks are used.** The sound request producer
is excluded only to compare its intent independently of queue publication.

The authored fixture contains synthetic profile/trig/lookup/geometry values
and scalar before/after fields. The owner proof materializes994 IDX identities
and the supplied map resources, uses actual original lookup/profile/bounds,
and keeps its fixture/reference payload outside the source release.

Successful preparation-r2 evidence:

- 1,548 full authored detector cases:1,478 exact matches,70 explicit unsupported
  contact/math holds. All supported before/after fields, result/intents, query
  counts, query results and XYZ endpoints match original execution.
- 90 full materialized owner-stage detector cases:82 exact matches,8 explicit
  unsupported contact holds.
- 1,215,572 original instructions and4,524 restricted GTE commands.
- 478 original first queries and434 second queries exercised; the proof records
  exact source call arguments and continuation results.
- Native hot-loop allocations:zero. Warnings treated as errors.
- Strong holds tested for missing profile/map/bounds/lookup/scratch/global,
  unordered bounds, supplied fake phase1 premises and stale vector state.

Authored fixture SHA-256:
`4f449299d9ace59826122127d5ffd6d3acb090486cc8003880c760d9f9dfc3ee`.

Commands:

```
python3 -B docs/research_01419/actor_detection_preparation_source_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE --dat /path/to/owner/CDDATA/DAT \
  --out /new/owner-local/preparation-proof
./l3test_actor_detection_preparation \
  tests/fixtures/detection_01419/authored-preparation.tsv
```

The optional owner comparison adds owner-stage-preparation.tsv, the local
cooked pack directory, and owner EXE as arguments2..4. Owner reference records
are read only and never emitted as release source data. These comparisons are
bounded original/native field evidence, not a captured natural encounter or
measured retail-versus-native deterministic replay.

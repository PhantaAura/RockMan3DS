# Bounded original actor-detection local step query

`query_actor_detection_segment_80049c54` implements the proved subset of the
original USA helper80049C54. This is `SOURCE_BACKED_PARTIAL`; it preserves the
original helper contract rather than introducing a generic raycast.

## Exact query boundary

The original helper copies two eight-byte short vectors to scratchC0/C8. A
cross-sign X or Z endpoint with absolute start coordinate above16384 returns
80000000. A missing/zero containing cell returns40000000. Otherwise it gathers
at most four candidate cells and traverses them in reverse order.

The source8004B1E0 includezero1 call retains zero and negative entries,
deduplicates only positive descriptor IDs, and returns immediately when an X
or Z neighbor lies outside the active area. This implementation preserves that
early return. The inherited allocating map helper and actor-contact gather
currently continue across such invalid neighbors; they are not reused here.
Interior-only previous contact fixtures do not establish their edge behavior.

Descriptor bank selection uses IDX raw6 lowFFF. Family dispatch uses the first
primitive code, requires it below256, and indexes8008AA24 by `(code&F0)>>4`.
The native subset supports family0/1 primitive codes1/3/15. The original
8004E304 classifier uses the **start point**, with expanded strict X/Z bounds.
Its rectangle consumer only needs classifier truthiness. The endpoint does not
create a line intersection test. Strict expanded Y then yields packedID plus
primitive flag low four bits shifted left16. The original class argument filter
is retained; detector callers supplyFFFF.

Packed negative cells retain the containing-cell floor/ceiling checks and the
original low16-bit return. Unknown intersecting slopes or dispatch families,
missing IDX/descriptor/list data, and malformed ranges yield `Unsupported`.
They never yield an invented clear or obstructed result. No triangles are
scanned, no geometry or temporary arrays are allocated per query, and only the
selected cells' resident typed lists are traversed.

The detectorphase1's two calls currently pass updated origin→same origin and
midpoint→same midpoint. The helper's general endpoint overflow boundary is
still preserved independently of those particular callers.

## Neutral owner-local bounds package

The six original bounds at80097D0C are not embedded in native source. Optional
L3DB sidecar loading supplies their source-owned values. Missing or invalid
sidecar data must hold detector admission.

L3DB has a32-byte header followed by six little-endian signed16 values, in
scratch124..12F order: **x0,x1,z0,z1,y0,y1**. The exact44-byte format checks
magicL3DB, version1, header32, flags0, source identity80097D0C, count6,
payload12, payloadCRC32, file44 and ordered bound pairs. Truncated/extra bytes,
wrong identity/fields, changed CRC, and CRC-valid unordered pairs are rejected.
The source address is evidence metadata; only the owner-local payload carries
original bound values. Both encoder and strict loader are provided.

## Validation and reproducibility

The read-only source probe can be run with fresh output storage:

```sh
python3 -B docs/research_01419/actor_detection_segment_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT --out /new/local/proof
```

The source-safe fixture contains **2,537 authored queries**. Native comparison
admits **2,017 exact original result/candidate-count matches**, including483
nonzero outputs; **520 unknown cases are explicitly held**. The proof executes
2,371,015 interpreted instructions with no return-only helper hooks. Fixture
coverage includes strict rectangle edges, Y edges, flagged primitives, classes,
raw6 bank choice, packed Y bands, sign-cross returns, zero/outside cells and
neighbor boundaries.

A separate owner-local stage fixture materializes the994 source map
initialization identities and issues21 queries. **14 supported original/native
matches pass;7 remain held**. Original stage bounds and positions in that
private fixture are excluded from the source checkpoint. The source-safe
report includes hashes and aggregate results only.

The native test checks L3DB malformed rejection and640 allocation-free queries.
Its optional owner invocation is:

```sh
l3test_actor_detection_segment tests/fixtures/actor_detection_segment_01419.tsv \
  /local/proof/actor-detection-segment-owner-stage.tsv /local/owner-pack
```

These authored/materialized contexts are not live retail encounter traces,
complete detection parity, animation/AI acceptance, or physical Old3DS proof.
The runtime bridge must inspect status before consuming the result, keep
source-owned detector state, and independently stage every dependent query.

# Original detector normalization and vector interpolation

Status: **SOURCE_BACKED_PARTIAL**. Native phase1 preparation can now run the
original normalization and interpolation arithmetic on retained source inputs.
These are integer/GTE field translations, not replacement floating-point math.
General GTE emulation is not added to the native engine.

`source_normalize_8007A694` retains signed16 IR input narrowing, SQR MAC
products, the original leading-zero adjustment, reciprocal-square-root lookup
selection, GPF multiplication and signed arithmetic shift. The192 lookup
halfwords at original EXE address `80092D38` are owner data. A strict416-byte
`L3NRv1` sidecar carries them locally; source releases exclude the sidecar.
Missing lookup data rejects nonzero vectors. A zero vector's final outputs are
independently zero even though the original performs an irrelevant out-of-table
read; the native helper need not read undefined reference data to return zeros.

The original helper uses signed ADD to sum three squared components. Sum above
INT32_MAX is an excluded source arithmetic-trap domain and rejects. Actual
phase1 preliminary differences each satisfy abs<=26750, which bounds the sum
to2,146,687,500 below INT32_MAX. Subsequent candidate normalization applies the
same narrowing/domain validation without assuming it always fits.

`source_detection_interpolate_80033F04` retains the actual detector arguments:
turn357 and threshold0FF069DA. It compares the original wrapped dot product
shifted left4 against the signed threshold. The fast branch copies the current
normalized vector. The other branch:

1. Computes original OP cross vectors: normalized×previous, then
   previous×that result, with Q12 arithmetic shifts and signed16 GTE input
   narrowing at each helper boundary.
2. Builds the original matrix columns from secondary cross, first cross,
   and previous vector, narrowing matrix elements to halfwords.
3. Applies original MVMVA to candidate vectors **(+357,0,4080)** and
   **(-357,0,4080)**. The turn occupies X in the original, not Y.
4. Computes wrapped source dot comparisons, retains the original tie choice,
   and normalizes the selected candidate through the same owner lookup.

This was determined and corrected by original/native differential tests. A
normal linear blend, generic angle smoothing, or floating-point normalize would
change source behavior.

## Endpoint preparation contract

After storing the interpolated vector, the source computes each step as
`(signed16(interpolated)*25)>>10`, narrows step to halfwords, then advances
origin halfwords with wrapping. The original first `80049C54` call passes the
**updated origin as both start and end**. The second passes the **midpoint as
both start and end**, where midpoint=oldorigin+(signed16(step)>>1). These calls
are not a modern segment raycast from previous to new position.

Both query calls use bounds record80097D0C and source filter argumentFFFF.
Their fourth copied halfword is unused by the proved coordinate/query path;
the runtime should use typed XYZ endpoints and avoid inventing semantics for
that lane. The separate bounded segment module must preserve source candidate
order, contact filtering and unsupported-family rejection.

## Scratch mode ownership

Detector admission tests byte1F800075 only to decide whether to request sound156.
Original initializer8002CFA4 clears it to zero at8002CFD0; initialization callers
include80013640 and8001F528. Original80023728 later copies the low byte of global
800C0DE6 into it at80023758 when global80098824 is zero. The supported ST00 loader at80100528..80100560 supplies its40-byte
per-area configuration from8011AC54+area*40 to800371A0; the helper copies all40
bytes into800C0DC0. Its halfword+38 therefore owns800C0DE6 and the copied
scratch mode. Five original configuration copies and20 original render-copy
gate cases were proved separately. Store this scalar as area-keyed owner
metadata; do not embed the retail configuration table or borrow a value for
unproved areas. The wider runtime render gate/cadence is still partial.
Original sound producer8001A008 queues a bounded request; actual playback
consumer/decoded sound data remain separate owners.

## Proof

The restricted read-only original interpreter executes CPU instructions and the
reached SQR, GPF, OP and MVMVA GTE field operations. Projection, general flags
and arithmetic overflow traps are excluded. All table values in source fixtures
are authored uniform lookup scalars; no original lookup records are included.

- 959 authored normalization comparisons.
- 196 authored full interpolation comparisons, including both source branches.
- 137 further normalization and959 interpolation comparisons using the actual
 owner-local original lookup. Only authored vector contexts and scalar outputs
 are recorded locally; table records are not exported.
- 392,292 original instructions and8,582 restricted GTE commands in the combined
 successful math-r2 proof.
- All comparisons pass; helper hot-loop allocations are zero.
- Strict sidecar roundtrip and malformed magic/version/header/count/CRC/reserved/
 length/payload rejection tests pass with warnings treated as errors.

Commands:

```
python3 -B docs/research_01419/actor_detection_math_source_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE --out /new/owner-local/math-proof
./l3test_actor_detection_math \
  tests/fixtures/detection_01419/authored-normalization.tsv \
  tests/fixtures/detection_01419/authored-interpolation.tsv
```

The optional owner comparison invocation additionally supplies owner EXE,
owner-normalization.tsv and owner-interpolation.tsv as arguments3..5. This proof
does not establish retail replay parity, natural enemy acquisition, audio
playback, or physical Old 3DS performance.

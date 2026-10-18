# Actor wall/floor transaction subset

Status: **SOURCE_BACKED_PARTIAL**. The new typed actor contact API admits an
allocation-free native subset of the original wall core and floor query. It
does not establish the complete retail AI callback or encounter parity.

Read-only owner inputs remain USA `ROCK_NEO.EXE`, SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`, and ST00
PROGBIN, SHA-256
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
The reproducer exports authored scalar premises, results and hashes. Original
instructions, code bytes, geometry arrays, table payloads and assets remain
owner-local.

## API and commit boundary

`ActorContactWork` retains the complete eight-byte raw position, sixteen-byte
fixed position, both complete prior-position mirrors, all three velocity
halfwords, object class and state74. The padding raw/fixed components matter
when the original class20 wrapper restores the complete blocks.

`query_actor_wall_contact_8004b6ac` gathers at most four original cell candidates,
retains positive-cell deduplication and original list selection, applies the
class filter, and computes original scratch/result mutations for the admitted
subset. The caller commits them through
`apply_actor_contact_result_8004b414`. A separate explicitly owned original
core result can use the same wrapper boundary. Neither query nor wrapper
changes the caller on an unsupported result.

`query_actor_floor_contact_80048ea4` consumes the actual post-wall raw position
and the original previous Y. It preserves the source signed-Y-delta choice of
comparison point and the valid signed return32767 when the supported query
finds no floor. The caller owns the action's Y stores, 33-unit rollback and
transition. `NoData`, `Unsupported` or `IterationLimit` must reject the action
transaction; they cannot become current Y or a manufactured zero wall result.

The wall subset supports original rectangle codes1/3/15, the existing proved
motion-eligibility/tie branches of code1, packed negative-cell Y corrections,
class filtering, axis-result masks, and positive result state74 handling.
Code1 deep-overlap recovery, airborne upward code3, slope responses and
potentially intersecting diagonal families remain unsupported. Families2..5
can be rejected outside the original strict conservative envelope only for
symmetric, nonnegative radii and ordered, unwrapped extents. Degenerate or
wrapped extents, unknown families6/7, missing lists and malformed ranges retain
the unsupported boundary. The floor subset supports rectangle1/3/15 and packed
negative cells under the same strict guards.

Wall passes are capped at nine and floor passes at four. A still-changing last
pass reports `IterationLimit`; no incomplete floor minimum is admitted. This
is a conservative native budget, not a new retail collision rule. Malformed
group/primitive spans reject before looping over an attacker-sized count. All
candidate storage is fixed on the stack; there are no per-query allocations.

## Corrected zero-result semantics

The original `8004B414` branches to rollback only for a **negative** core
result. A zero result preserves raw/fixed positions, velocity and state74.
Earlier shared-player documentation and the player helper incorrectly grouped
zero with negative results. This source pass supplies the proof for correcting
that helper.

Positive results apply the original X/Z/Y masks to both raw and fixed mirrors,
preserving unsnapped fractional components. State74 receives the scratch byte
unless it is already2. A negative class20 result restores the complete raw and
fixed mirrors and preserves velocity/state74. A negative zero-group result
restores X/Z, clears velocity44/48, and restores Y/clears velocity46/state74
only when state74 is nonzero.

The actor wrapper and corrected shared C player wrapper both agree with176
original instruction cases across classes00/10/20/A3, state74 values0/1/2/255,
negative extrema, zero, positive low-bit returns and all axis masks. The core
return/scratch input is explicitly authored in these wrapper-only cases.

## Full core and owner-stage differential evidence

The reproducer also executes complete original wall and floor calls on2,736
authored geometry cases. These runs have no call hooks. The native admitted
subset agrees on2,190 full wall calls, including642 nonzero corrections;
546 wall cases remain excluded. Independently supplying the original post-wall
mirrors admits2,268 full floor comparisons. The native test accepts a case only
when its query status is `SourceBackedPartial`, and requires exact returned
result and every wrapper-mutated field.

The three original area1 spawn positions each have six additional no-hook
owner-stage calls after the separately proved994-identity IDX materialization:
zero motion, +/-4 X, +/-4 Z, and +4 X/+4 Z. All18 full original wall calls
return0 and preserve the requested fixed fractions; all18 subsequent original
floor calls return-1. The native queries match all18 using the matching cooked
L3MP/L3RG/L3IN, without changing those input files. These are actual stage
queries proving a supported zero wall result, rather than a placeholder zero.

The owner-local scalar/evidence outputs are under
`work/pass-20261006-worldalive-01418/actor-contact/`. The source-only test also
checks strict diagonal edges, code>=256 rejection, invalid spans, preserved
transactions, negative-class restoration and floor convergence rejection.

## Slope floor contract discrepancy

Full original floor code8 exposed a mismatch in the retained legacy
`runtime_contact_floor_interval_8004a060` contract. The older scalar transform
probe writes horizontal bounds to opposite scratch offsets from those written
by the actual `8004B3C0` wrapper. For the authored rectangle[-100,100] in X/Z,
Y[-128,128], actor bounds[-60,60,-60,60,-128,0] and query(-120,-60,0), the full
original floor wrapper returns-48 while the retained legacy helper returns48.
Scalar transform agreement on the earlier reversed premises therefore does
not prove wrapper integration. The new actor API conservatively rejects
slope8..11. This discrepancy remains a separate shared-player/floor-sampler
repair gate; no approximate slope interval is supplied by this actor module.

## Reproduction

Choose a new output name; earlier owner evidence is preserved:

```sh
python3 -B docs/research_01418/contact_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261006-worldalive-01418/actor-contact/new-contact-source.json

test_actor_contact \
  ../pass-20261006-worldalive-01418/actor-contact/new-contact-source \
  ../pass-20261005-worldalive-01417/pack-runtime
```

The optional second native argument identifies a pack directory containing the
matching `st00.l3map`, `st00.l3render` and `st00.l3init`. With no arguments the
test uses authored fixtures only and needs no owner inputs. The full callback
still requires owned BIOS RNG, persistent detection, complete request handling,
packet/proxy ownership, allocator/effect behavior and scheduling admission.

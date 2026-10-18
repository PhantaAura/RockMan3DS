# Original family80 subtype2 scalar callback

Status: **SOURCE_BACKED_PARTIAL**. The neutral native callback is
`tick_source_effect2_80116e68()`. It preserves the supplied overlay's actual
scalar operations; it does not turn an arbitrary room-specific particle into
a new enemy implementation.

## Source ownership and behavior

Original lower-family80 dispatcher800172F4 selects the active subtype2 callback
80116E68 from an owner-admitted overlay table. Unlike the built-in128+ sweep,
this route runs before E0 in the original caller. Ascending live record order
matters: a newly allocated child later in the pool can update during that same
sweep. This file implements one transaction; the separate native dispatcher
must preserve that behavior and admit the real table identity.

The original callback routes work0D=0 through unsigned parameter03. Parameter0
advances clock11 and allocates a child every fourth invocation; the new child
consumes three BIOS result receipts for positional offsets. Parameter1 advances
clock11, allocates every eighth invocation, and marks the parent for deferred
release when its incremented unsigned clock reaches17 or above. Parameter2 is
an original no-op. Other parent dispatches remain unknown.

For work0D nonzero, state08=0 updates the child primitive color/extent and
unsigned countdown; a countdown reaching zero selects state08=1. State1 marks
the record for deferred release. Child parameters0 and1 have different proved
fade/extent operations; other child parameters follow the original countdown
without those modifications. Unknown state table entries hold.

80031618 selects the first free bit across both32-bit masks. Reuse clears only
record bytes2..19 and installs06=3; other bytes survive until their actual
writers run. Source initializer stores and inherited primitive words are
preserved exactly. Deferred release clears work00 and updates800988C8/CC; it
keeps the allocation until the separately proved cleanup consumes the mask.
The original parent initializer blindly dereferences a null allocation result.
The native safety boundary holds on exhaustion without writing address zero.
This limit is explicit, rather than a claim of full failure-behavior parity.

## Required inputs and guarded publication

- Source identity must select a72-byte record in the64-record pool. The caller's
  active flags00/06 and subtype02 must be known; relevant dispatch/state/clock
  and primitive lanes are validated as each original branch needs them.
- Allocation needs both original allocation masks and a source-installed class
  byte at the selected child. Release additionally needs both release masks.
  Inconsistent masks and malformed identities hold atomically.
- Pulse initialization and the child fade operations need the eight scalar
  words from original table8011CD74, retained locally in a neutral package.
  `SourceEffect2ColorReceipt` binds that table token to an admitted owner lease.
  No palette or fallback color table is embedded in the source.
- Smoke initialization needs three typed receipts at original return addresses
  80116F60,80116F98,80116FE4. Each binds generation, consecutive global ordinal,
  parent identity/clock/parameter/mode/tag/XYZ, newly allocated child identity,
  callback identity, and V0. These receipts are external BIOS observations,
  not a generated/default seed. The callback itself consumes no RNG owner.
  The caller must reserve and publish receipt-owner advancement only with a
  successful callback commit. The shared64-bit auditor sequence must start
  above zero and its three consecutive ordinals cannot wrap. Zero/wrapping
  triples hold atomically; the final nonwrapping triple remains admissible.
  Missing/stale receipts leave all pool lanes and
  masks unchanged.

The existing global PS1 capture has corresponding three-call groups, but lacks
actual parent-work records. Those values alone do not establish this callback's
live receipt context. Deterministic authored consumer tests do not prove retail
RNG chronology.

All publication uses bounded local record copies. Untouched raw and known lanes
survive. The single callback allocates no C++ heap memory; actual ARMv6K GCC
reports416bytes static stack for the callback and8bytes for small helpers.
This is a compiler observation, not a measurement of whole-frame stack peak.

## Verification

`source_effect2_probe.py` reads the immutable supplied PS-X executable and the
already retained overlay. It executes402 authored scalar cases using26,806
unchanged MIPS instructions. The original allocator, dispatch, field updates
and deferred release execute directly. Smoke cases stop at the real BIOSA0/2F
boundary, accept120 explicitly authored external return premises across40
spawns, and resume at the original saved return addresses. There are **zero
instruction patches and zero helper hooks**. The eight authored data-table
words and work records are test inputs, not original texture/palette payloads.
The full original BIOS RNG implementation was not executed.

`test_source_effect2.cpp` compares original parent/child bytes, allocation and
release masks, and exact known write lanes for all402 cases. It also passes
2,341 unknown-lane/context/exhaustion atomic guards and verifies zero C++ heap
allocations during each callback. Clang and actual devkitARM ARMv6K GCC compile
with warnings as errors. No source asset data, decompiled listing or original
instructions are included in this checkpoint.

Commands (from the source root, output must be a new private directory):

```sh
python3 -B docs/research_01438/source_effect2_probe.py \
  --exe /absolute/path/to/already-supplied/ROCK_NEO.EXE \
  --overlay /absolute/path/to/already-retained/owner-overlay.bin \
  --out /absolute/path/to/new-private-oracle-output
ctest --test-dir /absolute/path/to/host-build -R source_effect2 --output-on-failure
```

Source proof hashes and limits are in `source-effect2-proof.json`. Native render
submission, complete lower80 dispatch, cadence, live RNG, tutorial progression
and physical Old3DS parity require their separate evidence.

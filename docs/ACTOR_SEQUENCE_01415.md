# Original actor sequencing subset

The source class20/subtype03 initializer at `0x80102410` selects request0
through `0x800322A8`. Its callback `0x8010226C` snapshots requestAC intoAD,
dispatches the actor state, then compares old and current requests. A changed
request calls the original reset helper `0x800322A8`; an unchanged request
calls `0x80032488` once. These are direct source call sites, not animation
semantics inferred from numeric order.

The native scalar helper `actor_sequence_step()` implements the sequence
portion of `0x80032488`. It decrements the unsigned byte countdown with wrap.
A countdown0 becomes255 on this invocation. When the decremented countdown
is0, current record byte3 controls the next action:

- Nonnegative signed control advances the pointer by one8-byte record.
- Control-1 reloads only the current record's countdown byte.
- Other negative controls select the absolute low-seven-bit record index.

Selecting a new record reloads its first four bytes into frameA4,
countdownA5, opaqueA6 and opaqueA7. The branch reads control from current
record byte3; opaqueA7 is not substituted for this memory read. The helper
does not allocate. Missing records or caches exceeding256 records reject the
step and preserve the previous cursor.

The owner-local class03 row0/request0 sequence has exactly eight reachable
records, indices0 through7, then returns to0. Each record's original
countdown is1. The cache can decode those eight imported poses at area load
and share them among the three imported instances. Public evidence stores
the chain/count/hash, not the original record or pose bytes. Class20/35's
diagnostic pose0 is not source-selected animation and is not eligible.

Source dispatcher `0x80016FD4` iterates sixteen class20 pool slots at
`0x8009F108`, stride960. It admits a callback only when
`(signed work0 & unsigned work6 &1) !=0`. The gameplay update at
`0x80015DB4` calls the work80 distance owner before this dispatcher. Another
dispatcher caller exists at `0x800137F4`. The overall game-state/pause
ownership and frame cadence remain separate unresolved gates. A diagnostic
native playback gate driven by GameplayClock therefore remains default-off
and **CANDIDATE** for scheduling, even though the sequencing branches are
**SOURCE_BACKED_PARTIAL**.

Allocation-frame timing also remains explicit: the initializer resets the
request0 record, and if the prior requestAC was0 the surrounding callback
then advances it on that same callback. Decoding pose0 at native load and
advancing on the first native gameplay step is diagnostic timing until the
allocator's prior request state and complete callback scheduler are proved.

`actor_sequence_source_probe.py` executes240 original sequence cases with
five checked scalar outputs. Only pose and matrix consumers are hooked; the
original sequencing branches execute. It also executes36 original callback
admission cases with an authored counter callback. Both passed. Synthetic
records are used for these fixtures; they are not retail replay parity.

The standalone native helper test was compiled with AppleClang warnings as
errors and passed all240 sequence cases plus null, over-budget and missing
next-record guards:

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I include \
  src/world/actor_sequence.cpp tests/test_actor_sequence.cpp \
  -o /tmp/test_actor_sequence
/tmp/test_actor_sequence docs/validation_01415/actor-sequence-source-cases.tsv
```

Full AI request changes, event pause/cutscene ownership, complete animation
reset behavior, GTE rounding and hardware cadence are not verified by this
scalar helper. The source callback does more work than this sequencer.

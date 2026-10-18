# RNG chronology and admission, 0.0.14.21

Status: **SOURCE_BACKED_PARTIAL**. The original dispatcher and callback call
order are measured on authored inputs. Original reference input/update
chronology and the complete native consumer schedule remain **UNKNOWN**.

The public decomp is useful navigation. Its pinned symbol file labels resident
`8007FE50` as `rand`; the sampled game source still uses an assembly include for
the gameplay update. It does not supply a complete portable enemy scheduler or
an established native seed. Research copies remain outside this source tree.
No public implementation was adopted.
[Pinned symbols](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/config/syms.us.rock_neo.txt),
[pinned game source](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/src/rock_neo/game.c).

## Concrete source result and a real validator fix

`rng_callback_chronology_source_probe.py` reads only the hash-guarded owner EXE
and tutorial overlay. It executes the original 16-slot dispatcher `80016FD4`,
original class03 callback `8010226C` and unchanged resident random trampoline.
The A0 gateway logs an explicitly authored ordinal scalar. Pose/effect/proxy
consumers and the dynamic dispatch binding are declared external premises.

All **72 authored contexts**, with **101,040 original plus instrumentation
instructions**, admit `(work0 & work6 & 1) != 0` slots in ascending original
pool order. Each admitted class03 callback makes one random-service call, even
when its action does not use the sample's low bits. Original work+32A receives
that callback's scalar. This is not a live encounter, a BIOS implementation or
a complete source consumer schedule.

The original direct call at `801022F0` returns to **`801022F8`**. Its sample
store is later at **`801022FC`**. The inherited validator conflated these two
addresses and would reject genuine class03 returns. The validator now checks
the actual return address, requires the observed return before the later
store, rejects duplicate/stale ordinal stores, and validates all captured
actor context domains. A regression explicitly rejects a trace whose call
and return both incorrectly use the store address. The observer's original
store breakpoint stays at `801022FC`.

Proof: `docs/validation_01421/rng-callback-chronology-source-proof.json`.
The initial reproducer diagnostics also exposed two authored instrumentation
encoding mistakes: a signed16 scratch displacement and an SLL source register.
They were fixed before the 72-case result. They were not original game defects.

## Immutable tape implementation

`retail_rng_tape.h/.cpp` stores no generator and allocates nothing. It borrows a
bounded immutable capture owned by the loader. Binding requires whole-trace,
supported-source and continuous installation evidence, no pending return or
reseed, contiguous global ordinals and the installed OpenBIOS recurrence.
Failed rebinding revokes the old view. This validates the supported reference
profile; it does not prove a Sony retail BIOS profile or seed.

Peeking requires independent measured input/update and callback-schedule
binding to the same trace, global ordinal and reference-vsync boundary.
Class03 also requires exact original pool slot/work, state/action/phase,
statistics/context bytes, raw XYZ and unsigned heading. Other global consumers
remain in the tape and cannot be skipped. An unbound current runtime continues
to hold; captured positions or native frame counters alone cannot establish
the chronology binding.

Peeking does not consume. A one-use ticket advances a staged cursor only after
the complete actor transaction commits. A failed body/pose/audio/lifecycle
transaction discards the staged cursor. Exhaustion never repeats or wraps a
sample. Unloading/rebinding invalidates cursors and tickets, even if the same
immutable memory address is reused.

`test_retail_rng_tape.cpp` exercises authored proof/malformed-data/context/
competing-consumer/ticket/rebind/rollback/exhaustion cases and 4,096 allocation-
free held peeks. These authored tests are not retail parity or a claim that the
current live gameplay is driven by measured samples.

## Remaining real blocker

The original full callback draws unconditionally on every admitted update.
The native bridge currently implements a supported state1 subset and does not
own all outer callback/effect/proxy/clock consumers. Supplying a scalar only
when the state1 tail needs its low bits would use the wrong chronology. The
observer captures original actor context and global BIOS calls; it does not
yet establish matching player/input/update history in the native scene.

The next admission step is a guarded actual tutorial trace, followed by an
independently matched update boundary and complete consumer transaction. Keep
`PHYSICS_1TO1=false`, preserve `NeedsRandomSample` or `NeedsChronologyBinding`
when evidence is missing, and do not introduce a private random seed per enemy.

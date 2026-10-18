# Shared candidate RNG contract

The existing owner-local observed OpenBIOS profile supplies one global seed and
ordinal. Class 03, lower effects 2/3/4, lift/dust and scene event D6C share it.
Existing original-source scalar planners select the actual calls, child slots,
call-site addresses and arguments. No per-object random seed, invented clock or
replacement cutscene was added. The new stage is explicitly CANDIDATE.

Consumers bind frame, overlay generation, work address, slot, call-site and
required count. Late holds restore touched scene records and abort the RNG
transaction. Explicit supplied receipts take priority; missing explicit receipts
remain missing. Strict recorded RNG mode never admits generated candidate rows.
Zero-call/full-pool source branches do not require an RNG owner.

Fixed capacity 120 follows the supported source effect-4 maximum: 24 children,
five calls each. Class 03 still admits 16 distinct actor slots. A burst is fully
committed or held; partial commits and stale generations are rejected. Only used
audit rows are copied; a shorter transaction clears an older longer audit tail.

For non-class03 records the legacy actor.work/slot/caller_ra fields identify the
typed consumer. actor.state08/action09/phase0a are unused there; their defaults
are not evidence that original source actor fields were zero. source_generation
identifies the actual overlay owner. The service is not retained as a borrowed
pointer on RuntimeSession, avoiding stale ownership after copies/transitions.

There are no new hot-loop dynamic allocations. Largest compiler per-function
estimates: main 124192 bytes, session copy assignment 71056 bytes, effect-34
candidate wrapper 19216 bytes; reserved stack 262144 bytes. These are not a
call-chain sum or physical high-water measurement. Physical validation is open.

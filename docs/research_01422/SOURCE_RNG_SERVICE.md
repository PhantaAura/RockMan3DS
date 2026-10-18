# Optional shared RNG service, 0.0.14.22

The generator equation is **SOURCE_BACKED for the selected installed OpenBIOS**:
`next = (state * 1103515245 + 12345) mod 2^32`, return
`(next >> 16) & 0x7fff`. The unchanged selected BIOS instructions were already
executed by `research_01420/openbios_rng_profile_probe.py`: 256 authored seeds,
16 consecutive calls each, 4,096 calls and 45,824 original instructions. The
native service matches all 512 original scalar checkpoints across those 4,096
calls. These authored algebra cases do not establish a game seed.

The observed game-entry origin is **MATCHES_MEASURED_FIELDS** for this selected
reference run: entry count 1, observer vsync 1,062, random ordinal 0 and seed 0.
No seed is defaulted, invented or borrowed from BIOS BSS initialization. Native
consumer chronology remains **CANDIDATE**. The captured returns belong to nine
other original callers; there are zero admitted class03 samples. Candidate
freeplay omits those competing consumers and any unmodeled reseeding. Its
generated actor samples are not measured actor samples, and it proves neither
a retail BIOS profile nor retail parity.

## Capture audit and private sidecar

`tools/cook_source_rng_profile.py` reuses the existing strict reference validator
and adds exact row schemas, scalar domains, monotonic reference time, source
identity guards and an observed ordinal-zero game-entry boundary. It checks
the supplied BIOS/EXE and selected tutorial overlay SHA-256 before creating a
new private output directory outside source. `--st00` hashes the selected
overlay in memory; it writes no extracted overlay or instruction bytes.

Default cooking requires a completed bounded observer capture and rejects any
guard failure, pending return or partial line. Explicit `--capture-prefix`
audits every complete supplied row before allowing only an exact terminal
pending call, terminal observer failure and/or incomplete final JSONL line to
be excluded. Malformed calls/returns, interior failures, unknown row kinds,
state gaps, reseeds, missing/repeated origins and unsupported identities still
fail. There is no search for an earlier usable portion of a bad trace.

The retained live-r2 RNG snapshot has 23,280 complete rows and 11,638 completed
returns. It ends with entry for ordinal 11,639 and a final `lua_callback_error`.
The explicit prefix contains 23,278 rows; those two terminal rows are excluded.
Its whole trace is unaccepted. Snapshot SHA-256:
`df118de389b835380b04b58f8a73bc9a72a2b280fb210aa915adee141bb378b4`.
Validated completed-prefix SHA-256:
`f1f3df4a3ee11a838d24bca9113b212aec1761f1a798caad8df4476616a650d6`.

The accompanying input snapshot has 15,342 complete rows, including 15,341
original update observations. Its fields, original player/pool identities,
scalar domains and explicit false native chronology assertions are audited.
Input SHA-256:
`5eb4c143f01f42b9cd46b24d059c578f03a5fff32ac9ff9d85a7019d4ef88537`.
These observations supply provenance only; they do not bind a native replay.

The resulting `source_rng.l3rg` is exactly 256 bytes with SHA-256
`8db657383f9aff097e119361e877b30046d6478db39ab97381636a5fe2dddd54`.
The sidecar and its cooker evidence stay outside source/release archives under
the new private local output directory. No BIOS image, retail asset,
instruction bytes or random-return tape is serialized. Source SHA metadata,
origin scalars, capture scope, warning flags and counts are the whole payload.

## Native transaction boundary

`SourceRngService` has one shared state across all class03 actors and areas.
Installation requires `SourceRngMode::CandidateOpenBios` explicitly and a
validated profile. Disabled/missing profiles supply no sample. There is no
per-enemy PRNG and no area-based reseeding. `RetailRngTape` is unchanged and
remains the independent strict measured-context/input-chronology replay gate.

Use `begin_source_rng_update(service, transaction, update_context)` before a
callback. Stage exactly one call with `stage_source_rng_class03(transaction,
actor_context)` for every admitted original class03 callback, even when its
body does not use sample32A. Context includes original caller RA `801022F8`,
pool slot/work, actor state/action/phase, native frame, native update ordinal
and explicit callback phase. The later sample-store PC `801022FC` is rejected
as a caller. Object record `source_index` is not an original actor pool slot.

Commit with `commit_source_rng_update` only when the complete corresponding
actor transaction succeeds. A held/rejected callback uses
`rollback_source_rng_update`. Earlier committed actors and shared ordinals
remain published. Per-callback native hold rollback is explicitly CANDIDATE
diagnostic scheduling; it does not claim that the original BIOS call waits
for the native implementation's missing downstream dependency.

A transaction can instead span a complete fixed update only if all associated
actor state is also staged. Rolling back a multi-actor RNG batch after earlier
actors have been published would be invalid. No actor-vector copying or heap
allocation is required for the per-callback integration.

Committed records are available in `service.last_calls()` and
`service.last_call_count()`. Staged records remain in `transaction.calls()`
after rollback so the caller may log a held attempt without publishing its
seed/ordinal. A native global ordinal labels generated candidate calls; it is
not an assertion that the captured reference's competing consumers match.
Transactions from older commits or installations fail as stale. Duplicate
pool slots in one staged batch and malformed caller/work contexts are rejected.

## L3RG v1 format

All scalar fields are little endian. The header is 32 bytes: magic `L3RG`,
u16 version 1, u16 header size 32, then u32 algorithm 1, total bytes 256,
payload bytes 224, payload CRC-32 and two reserved zero words. Algorithm 1
means only this selected OpenBIOS recurrence.

The first 160 payload bytes are five SHA-256 digests in order: BIOS, EXE,
overlay, whole supplied snapshot and validated completed prefix. The remaining
64 bytes are 16 u32 words: scope, warnings, observed origin seed, origin
ordinal, origin vsync, origin entry count, completed returns, reseeds,
complete snapshot rows, validated prefix rows, excluded complete rows,
snapshot bytes, validated prefix bytes, final completed ordinal, distinct
observed caller count and reserved zero. Reseeds must be zero for this profile.

Scope 1 is a whole bounded capture; scope 2 is an explicit completed prefix.
Required warnings are bit 0 (unmodeled consumers) and bit 1 (selected reference
is not proved retail). Prefixes also set bit 2 (capture incomplete), with bit
3 for final observer failure, bit 4 for excluded pending call and bit 5 for
excluded partial line. The retained live-r2 prefix has warning mask 31.
Runtime loading checks canonical size/CRC, fixed source identities, mandatory
warnings, counts, scope consistency and nonzero provenance hashes without
allocating. CRC and metadata checks provide bounded local integrity; they do
not independently authenticate an arbitrary hand-authored sidecar.

## Validation

- `tests/test_cook_source_rng_profile.py`: 12 authored guard tests pass.
- `tests/test_source_rng_service.cpp`: selected original BIOS scalar cases,
  optional measured private-profile decode, malformed headers/profiles/origins,
  explicit selection, caller/slot context, stale commit, per-callback hold and
  entire staged-update rollback pass with warnings as errors.
- 4,096 fixed updates / 65,536 staged calls allocate zero heap blocks.
- Existing `tests/test_rng_reference.py`: 28 tests pass unchanged.

The source-safe fixture `tests/fixtures/source_rng_openbios_01422.tsv` contains
only the 512 authored scalar checkpoints from the prior original-instruction
proof. It contains no live actor sample tape or BIOS instruction bytes.

Combined native integration, complete competing consumer recovery and retail
or hardware gameplay parity remain outside this evidence.

## Authored Lua listener harness: UNEXECUTED

`tests/test_rng_observer_lifecycle.lua` loads the actual repaired observer with
fake PCSX, read-only memory and IO bindings, authored synthetic headers and
guard bytes. It accepts the observer source as CLI argument or global
`MML_OBSERVER_HARNESS_SOURCE`; it uses no real owner files. The intended checks
are 12,000 completed synthetic returns using nine persistent caller listeners,
unrelated cached-address visits both while idle and during another pending
return, capacity failure at the 33rd distinct caller, and complete listener,
event and memory-handle cleanup on close. Capacity failure is explicitly
reported as UNKNOWN with whole trace unaccepted. This harness checks listener
lifecycle; it does not implement a second BIOS recurrence or reproduce the
real emulator binding failure.

At authoring time it is **UNEXECUTED**: no `lua`, `lua5.4`, `lua5.3` or `luajit`
executable is on PATH; no standalone Lua, liblua or fengari was found in the
installed Homebrew/local, PCSX application or bundled runtime files; system
and bundled Python have no `lupa`. No interpreter was installed and no
emulator was launched. The harness must pass in an actual Lua runtime before
its intended results are treated as verification. A dedicated future Redux
test process may run it with the fake bindings; this document does not claim
that such a run has occurred.

The authored Lua lifecycle harness was subsequently executed through the installed PCSX-Redux Lua runtime: 12,000 returns, nine cached return listeners, 12 total registrations, explicit 33rd-caller capacity hold, and complete close cleanup passed. See `docs/validation_01422/rng-observer-lifecycle.json`. This tests the actual observer against fake bindings; it does not prove a long original-game capture is crash-free or establish actor RNG chronology.

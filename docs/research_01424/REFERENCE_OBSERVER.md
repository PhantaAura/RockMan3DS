# Read-only original-reference observers, 0.0.14.24

The RNG observer now retains a bounded cache of exact caller addresses, with a
source-informed capacity of128. A separate observer captures the
original player lease/GTE boundary. Neither change establishes complete enemy
RNG chronology, target selection, a native Buster hit or retail parity.

## Why the previous RNG capture stopped

The frozen 0.0.14.23 reference snapshot contains 5,705 complete rows: 2,850
completed random returns, the entry for random ordinal 2,851 and a terminal
`return_listener_capacity` failure. Its 32 completed distinct caller return
addresses exhausted the previous bounded cache. The 33rd caller, `80100E70`,
is an ordinary address in original 2 MiB RAM. It fits the final bounded
exact-caller cache. No admitted class03 enemy sample appears in that failed capture.
The whole trace remains **UNKNOWN** and unaccepted; this code change cannot
repair its missing terminal return or establish its missing enemy chronology.

Frozen input path:
`work/pass-20261008-worldalive-01423/reference-observation-r1/rng-reference.jsonl`.
SHA-256: `b0222b91c7602b148ebf98e522658494dc66df9af2fb7a01851bef89d301a389`.
The original process later reported an unknown C++ exception. The retained
capture proves the observer capacity failure; it does not prove that this
failure caused the emulator exception.

## Bounded exact-caller RNG ownership

`research_01420/redux_rng_reference_observer.lua` begins with three narrow
breakpoints: game entry, A0 service admission and tutorial sample store. It
lazily retains one exact-address breakpoint for each discovered caller, up to
128. The pending caller alone is enabled; all cached returns are disabled
while idle. Capturing a return requires its exact virtual execution address,
including when other virtual aliases share the same physical bytes. BIOS code
is checked again before retaining actual V0 and shared state.

A bounded owner-data scan found41 aligned direct-call candidates to the exact
`8007FE50` rand wrapper in the owner EXE payload and52 in the exact141056-byte
ST00 PROGBIN resource. These93 addresses and the32 completed prior live callers
support128 as a bounded engineering capacity; they are **CANDIDATE** instruction
sites, not a proof that every scanned word belongs to a function or that all
competing consumers are covered. No arbitrary new allocation is permitted at
the129th distinct caller: it stops with `return_listener_capacity` and retains
an explicit **UNKNOWN** failed capture. Source identity/admission requirements
for tutorial actor samples remain independent.

Listeners are created once per distinct admitted caller, never once per random
call. Unsupported return domains/aliases, nested calls, changed BIOS and
execution-width failures remain explicit. The row budget stops before starting
a new service, with no pending return. Default strict trace validation rejects
failed captures; an explicit audited completed prefix is a limited analysis
mode. Raising this bounded research capacity cannot recover a failed trace or
establish complete native consumer chronology.

The initial wide-range experiment was rejected for the final observer. In the
[pinned official debugger](https://github.com/grumpycoders/pcsx-redux/blob/e7cff5efe8c487131d97833c56f79814d5fa35f7/src/core/debug.cc),
`checkBP` queues intersecting objects and invokes `triggerBP` before
[the breakpoint object's enabled check](https://github.com/grumpycoders/pcsx-redux/blob/e7cff5efe8c487131d97833c56f79814d5fa35f7/src/core/debug.h).
Disabled RAM/ROM ranges therefore still incur C++ dispatch and name construction
on almost every interpreted instruction. Constant listener count alone did
not demonstrate low overhead. The first live01424 profile progressed only to
game entry at the frozen observation, without admitted enemy/GTE rows; the root
reported2.92 UI FPS and0 guest FPS. CPU contention and a missing guest progress
measurement prevent treating that single screenshot as a controlled performance
benchmark, but the source confirms the range dispatch cost. The wide strategy
is absent from the final module. Its source/proof and private capture remain
outside the release under `observer-wide-experiment`/`reference-live-r1`.
Wide module SHA-256:
`b91a2ef2f2da769ce2723ee26c39b44ee21125635f4f10e6da4b8235bc27ad1d`.

The exact installed BIOS rand's JR is`BFC06108`; its delay-slot mask executes
at`BFC0610C`. Srand's JR is`BFC06114`; its delay-slot seed store is`BFC06118`.
An execution breakpoint before those instructions cannot observe the actual
post-delay V0/state. The final exact-return observer preserves actual returned
values rather than replacing them with an inferred epilogue result.

The [official breakpoint API](https://pcsx-redux.consoledev.net/Lua/breakpoints/)
documents enable/disable/remove ownership and actual execution addresses. The
installed API smoke check originally exercised range shape too, but it did
not benchmark dispatch. That experiment does not justify a wide default.

## Original player lease/GTE boundary

`redux_player_lease_observer.lua` hooks `80035508`. It compares the first
32 live instruction bytes with the already hash-verified owner EXE, without
retaining those instructions in the output. At entry it captures:

- Eight scratch camera words at `1F800008`.
- Raw mask `80098798` and all 24 raw pointers at `800BE478`.
- Selected owners' two inline words at `+BC/+C0`, in original slot order.
- All 32 raw `CP2C.r` and 32 raw `CP2D.r` words.

The original routine consumes high mask bits from bit31 to bit8, corresponding
to slots0 through23. Only selected pointers are dereferenced. Each selected
pointer must be nonzero, word-aligned and have its entire `+BC/+C0` span inside
original 2 MiB RAM through physical/KSEG0/KSEG1 aliases. Scratch memory, ROM,
hardware, extra RAM mirrors and KSEG2 pointers fail explicitly. Aliased owners
retain separate slot observations and a shared physical `alias_first_slot`;
no invented independent owner copies are introduced.

One retained exact-RA return listener captures the same admitted owner points
after the original stores and both raw GTE banks after execution. It also
records the current mask/pointer table, so changes are visible. A second RA
stops with `return_listener_capacity` and **UNKNOWN**, rather than allocating
a new callback per call. Entry and return identities, pending state, bounded
rows and memory-file cleanup are guarded. BC/C0 words are points, never
pointers to follow. The [official register API](https://pcsx-redux.consoledev.net/Lua/memory-and-registers/)
provides the raw CP2 bank arrays. These are raw backing snapshots: this observer
does not replace unused data15 storage with architectural SXYP mirroring or
normalize IR sign extension. A comparator must document any such adapter.

Set these launcher globals before `dofile`:

```lua
-- Reuse the RNG bootstrap's hash-verified MML_OWNER_EXE_PATH/SHA256.
MML_PLAYER_LEASE_TRACE_PATH = '/absolute/new/private/player-lease.jsonl'
MML_PLAYER_LEASE_MAX_ROWS = 8192 -- allowed range16..8192
```

The observer refuses to overwrite a trace. Its global is
`MML_PLAYER_LEASE_OBSERVER_01424`; `status_readonly()` and `close()` are bounded
inspection/cleanup APIs. JSONL entry/return kinds are `player_lease_enter` and
`player_lease_return`, matched by `ordinal` and `caller_ra`. Scalar arrays are
unsigned 32-bit decimal words. This original function boundary can be compared
without assuming that native gameplay and the reference have the same input
history.

## Executed tests and limits

The installed PCSX-Redux LuaJIT 2.1.1785598229 runtime ran the actual Lua modules
against authored read-only fake bindings, with no owner game loaded:

- RNG: 100,000 completed calls in four bounded25000-call authored captures;
  128 retained return listeners,131 total registrations, at most four enabled.
  The129th distinct caller is rejected explicitly without new registration.
  RAM/ROM boundaries, reseed-service ordinal ownership, unrelated visits,
  unsupported domains, budget stopping, nested calls, changed BIOS, execution
  width and close with a pending return pass.
- Lease: 4,000 completed pairs, 192,000 selected-owner snapshots, two total
  listeners. Alias ordering, invalid active pointers, ignored inactive
  pointers, changed caller/code, nested calls, missing CP2 state, read
  exceptions, execution width, budget stop and pending-close cleanup pass.
- Installed binding capacity check:131 real narrow listeners (three base
  plus128 retained returns) support enable/disable/removal;64 raw CP2 words
  are readable. No wide range is registered in this final test.
  This checks installed API shape, not callback execution during gameplay.

Proofs and full runtime logs are under `docs/validation_01424`. Each authored
harness reports synthetic guards, no real owner files, no emulated-state
mutation and `retail_parity=false`. The first isolated runtime attempt inside
the filesystem sandbox exited `-6` with empty stdout before a test result;
the approved isolated-runtime retry passed. No test failure was suppressed.

Exact private executed command records:

```text
work/pass-20261008-worldalive-01424/lua-observer-harness-r6/argv.json  RNG
work/pass-20261008-worldalive-01424/lua-observer-harness-r3/argv.json  lease
work/pass-20261008-worldalive-01424/lua-observer-harness-r7/argv.json  installed API
```

To rerun an authored test with a standalone compatible Lua runtime:

```sh
lua tests/test_rng_observer_lifecycle.lua /absolute/source/docs/research_01420/redux_rng_reference_observer.lua
lua tests/test_player_lease_observer_lifecycle.lua /absolute/source/docs/research_01424/redux_player_lease_observer.lua
```

The current machine has no standalone Lua on PATH. Its isolated Redux
bootstraps set the corresponding `MML_*_HARNESS_SOURCE`, invoke the test with
`pcall(dofile,...)`, then request exit0/1 with the documented host runtime quit
API. The observers themselves contain no quit, pause, resume or reset calls.
The isolated launcher disables memory cards, GDB, PCDRV and web servers and
loads no ROM; new unused host test card paths do not touch owner save data.

For a fresh live tutorial capture, install the hash-verified observer bootstrap
before game entry, use interpreter/debugger mode and the existing disabled
memory-card configuration, and keep HTTP status disabled. Use the owner-supplied
reference game and ordinary controls to reach the tutorial. The exact class03
code and pool guards must admit the desired actor before treating any stored
sample as its measurement. A mid-session install must keep its explicit
`MID_SESSION` scope; it cannot recover earlier competing RNG consumers. Keep
all traces private, immutable and outside code-only release archives. Native
RNG scheduling remains **CANDIDATE** until the captured consumer/input history
is independently matched.

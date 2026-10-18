# Shared RNG reference boundary, 0.0.14.20

The selected installed OpenBIOS random service is now **MATCHES_MEASURED_FIELDS**
on authored inputs. The game's seed, complete consumer scheduling and retail
parity remain **UNKNOWN**. This research changes no native gameplay source.

## Concrete result

The installed BIOS SHA-256 is
`713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d`.
Its original random routine at `BFC060E4` updates the shared word at physical
`00008448` (cached alias `80008448`); its seed setter starts at `BFC06110`.
`openbios_rng_profile_probe.py` executed the unchanged installed instructions
for **256 authored seed contexts**, each with **16 consecutive random calls**:
**4,096 random calls, 45,824 instructions, no hooks**. Both stored state and
returned scalars match the public recurrence and mask. The proof is
`docs/validation_01420/openbios-profile-proof-r1.json`.

The installed boot has a BSS-clear interval `00004AA0..0000B05C`, containing
this shared word. The public boot explicitly clears its BSS. These facts
support a cold OpenBIOS initialization boundary; they do not establish the
word's value when Mega Man Legends reaches its entry or an enemy encounter.
Earlier BIOS/game consumers, reseeding, warm resets and save states still
matter.

## Primary references checked

The current official PCSX-Redux main commit checked was
`e7cff5efe8c487131d97833c56f79814d5fa35f7`. Its MIPS sources are now the nugget
submodule at `e9f70ebc993d2a4323368ca88d5eabfe1233e0a0`; inherited links to
`pcsx-redux/src/mips/openbios` may point to older cached content.

- [Pinned OpenBIOS RNG](https://github.com/pcsx-redux/nugget/blob/e9f70ebc993d2a4323368ca88d5eabfe1233e0a0/openbios/kernel/libcmisc.c)
  owns one unsigned state and the two services.
- [Pinned handler table](https://github.com/pcsx-redux/nugget/blob/e9f70ebc993d2a4323368ca88d5eabfe1233e0a0/openbios/kernel/handlers.c)
  maps A0 functions `2F/30`.
- [Pinned boot](https://github.com/pcsx-redux/nugget/blob/e9f70ebc993d2a4323368ca88d5eabfe1233e0a0/openbios/boot/psx.s)
  and [linker layout](https://github.com/pcsx-redux/nugget/blob/e9f70ebc993d2a4323368ca88d5eabfe1233e0a0/openbios/psx-bios.ld)
  explain zero initialization.
- [Official breakpoint API](https://pcsx-redux.consoledev.net/Lua/breakpoints/)
  requires debugger/interpreter and supports observation callbacks.
  [Events](https://pcsx-redux.consoledev.net/Lua/events/) documents listener
  lifetime and removal.
- [Pinned debugger](https://github.com/grumpycoders/pcsx-redux/blob/e7cff5efe8c487131d97833c56f79814d5fa35f7/src/core/debug.cc)
  normalizes memory aliases for breakpoints, so the A0 observation does not
  need duplicate cached/uncached registrations.

Reference copies and retrieval hashes remain outside the source checkpoint
under `work/pass-20261007-worldalive-01420/rng-reference/external`. No public
BIOS implementation or foreign engine code was copied into the native engine.

## Live observer prepared

`prepare_rng_reference_observer.py` hash-checks the installed BIOS, owner
EXE and the already supplied tutorial overlay. It writes a new bootstrap
outside source and refuses existing output names. It does not launch an
emulator. `redux_rng_reference_observer.lua` additionally compares bounded
live ROM/game code with those input files.

The observer records exact shared state before each A0 random/seed call,
actual V0 and shared state after its caller returns, global random ordinal,
game-entry boundary and original class03 stores. Actor records include raw
slot, index/context, position, heading and state. It assumes no seed, writes
no emulated memory, controls no pads and accesses no saves. Logs are bounded
to 16,384 rows by default, with an absolute maximum of 65,536.

Preferred prepared bootstrap:
`work/pass-20261007-worldalive-01420/rng-reference/live-r4/observer.lua`.
Future trace: the same directory's `rng-reference.jsonl`. The status-only
HTTP endpoint is disabled by default; ordinary file/event capture requires
no HTTP bridge. The preparer has an explicit `--enable-http-status` opt-in.

The initial process inventory found no running reference process. The root
task then launched the selected interpreter/debugger with the preferred
bootstrap. The observer is now **exercised**: live ROM and original game-entry
guards passed. Its installation read shared state **0**, then the original
game entry at observer vsync **1,062** also read state **0**. The status-only
GET subsequently reported **4,185 vsyncs**, one game entry, two recorded rows,
no guard failure and no pending return.

These are actual observations of this installed OpenBIOS reference case.
At the frozen snapshot, **zero random returns and zero class03 samples** had
been captured; New Game/encounter input had not occurred. The source-safe
proof is `docs/validation_01420/live-startup-proof-r1.json`; the original
frozen scalar capture stays outside source under
`rng-reference/live-r2/trace-startup-snapshot-r1.jsonl`.
Installing mid-session can measure that encounter's values; it cannot
recover earlier random consumers. A zero state in this run does not establish
a retail BIOS default or the full native consumer schedule.

The first reference process subsequently terminated. Its retained stdout
ends with the status HTTP request, then an unknown C++ exception and
`libc++abi: terminating`. This temporal association makes the HTTP bridge
a **CANDIDATE** trigger; the C++ root cause remains **UNKNOWN**. The earlier
two scalar rows and code guards are preserved. The new r3 bootstrap explicitly
sets `MML_RNG_ENABLE_HTTP_STATUS=false`, avoiding registration and requests
while retaining file-only observation. The failure and mitigation record is
`docs/validation_01420/reference-http-failure-r1.json`.
The [official HTTP Lua API](https://pcsx-redux.consoledev.net/Lua/web-server/)
requires a string return, which the handler supplied; the actual request also
returned valid JSON. No return-type violation is established. The disabled
bridge isolates a possible emulator/transport failure without assuming its
cause.

`validate_rng_reference_trace.py` checks actual call/return identities,
state recurrence, continuity and class03 stores without filling gaps.
Fourteen authored acceptance/rejection cases passed; they are explicitly
marked authored in `docs/validation_01420/validator-authored-cases-r1.json`.

The file-only r3 run subsequently captured **20 complete original BIOS random
call/return pairs**. All stored/returned fields and consecutive state values
match the installed profile. They share caller return address `801047AC`;
they are **not accepted class03 enemy samples**. The run stopped on a callback
code-identity failure when the address still held a different overlay.
A bounded supplied-data lookup found one non-tutorial overlay with the
aligned direct random call at that address; this is a **CANDIDATE** source
identity, not proof of the live overlay.

`docs/validation_01420/live-r3-failed-prefix-proof-r1.json` validates only
the 42-row prefix before the final guard failure, explicitly marks the whole
trace unaccepted, and admits zero native samples. The full failed trace is
retained outside source. Default validation still rejects any guard failure;
`--prefix-before-failure` is an explicit evidence-analysis mode.

The prepared r4 observer skips nonmatching callback code **before admission**
without reading actor fields. It admits only the exact owner tutorial code
and original pool identity. Any code mismatch **after admission** still stops
the observer. Four additional authored overlay/failure guards were added:
`tests/test_rng_reference.py` now passes **18 tests**. Root CMake registration
does not change. The r4 bootstrap disables HTTP and labels its intended
in-console installation **MID_SESSION**; it has **not been exercised**.
Reinstalling it into the existing original tutorial session is the next
concrete reference step. RNG ownership and native consumer cadence stay
**UNKNOWN**.

## Smallest remaining dependency

A guarded live trace can supply immutable original-reference samples for an
explicitly matched encounter. Native use must also match imported actor
identity, callback order and input/update chronology, and commit a sample
cursor only with the complete actor transaction. Such a replay would be
**MATCHES_MEASURED_FIELDS** for the captured fields and context.

It would not establish full retail parity or justify a private actor PRNG.
For a shared native generator, recover the source-owned start-state boundary
and connect all competing consumers first. Keep the existing optional sample
input and `NeedsRandomSample` hold until that evidence is available.

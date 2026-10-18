# Original random-service ownership

Status: **SOURCE_BACKED_PARTIAL** for the original gateway, callback consumption
and scalar-store boundary. Game seed, boot chronology and native random owner:
**UNKNOWN**. This pass adds a reproducible source probe and a bounded reference
observer; it deliberately does not instantiate a native seeded PRNG.

The owner-local inputs are the established `ROCK_NEO.EXE`, SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`, and
tutorial PROGBIN SHA-256
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
The source release contains no executable bytes, disassembly, tables, BIOS
payload or retail assets.

## Recovered boundary

Original trampoline `8007FE50` executes three instructions to jump to `A0`,
with `t1=2F` in its delay slot. It preserves the caller's return address and
input `a0`. Four authored register cases execute that original trampoline.

Class20/03 callback `8010226C` snapshots positions, angles and animation
request, stores `16912` at work15E, then calls the same trampoline. The result's
low halfword is written to work32A at `801022FC`, **before** state08 dispatch.
Twenty combinations of state08 `{0,1,2,255}` and explicitly supplied return
scalars verify one gateway call and the halfword store. State255 is a prelude
case only; no unsupported state body is executed or admitted.

Three ordered authored callback preludes, A→B→A, use one instrumented gateway.
Its count advances once per callback and each actor receives the supplied
scalar in call order. These are not generated random samples. The state body
is stopped immediately after the store; later consumers are excluded.

The bounded aligned static scan finds **41** direct rand-call candidates in
the EXE and **52** in the tutorial overlay. Other systems therefore reference
the same BIOS service. These addresses are candidate control-flow references;
the scan does not prove every aligned word is live code or recover all
indirect callers. A private random stream for each actor cannot be justified
from this source boundary.

The scan finds no A0 direct call or observed three-instruction `srand`
trampoline shape in those two supplied images. Four EXE loads of literal30
into t1 remain unrelated register-use candidates. **This is not proof that the
game never seeds randomness**: indirect calls, other overlays, runtime
patches, BIOS startup, warm restart and save restoration are not covered.

## Public BIOS implementation versus game ownership

The official MIT-licensed [OpenBIOS implementation](https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/kernel/libcmisc.c)
uses one unsigned32 state, the recurrence
`state = state * 1103515245 + 12345` modulo2³², and returns
`(state >>16) &0x7FFF`; its seed setter stores the supplied word. The official
[A0 handler table](https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/kernel/handlers.c)
binds those services to function2F/30. These public implementations explain
the API family; they do **not** prove the current game's initial seed or the
state of the user's running reference emulator. No OpenBIOS source code is
copied into the native engine by this pass.

The mathematical generator alone is insufficient for parity. Every earlier
rand consumer, allocation, effect, pause/update cadence and reseed affects the
global sequence. A boot default, a current timestamp, per-actor hashing or a
convenient fixed sample would be an unsupported choice.

## Reference observer and next capture

`redux_rng_observer.lua` uses the official
[breakpoint API](https://pcsx-redux.consoledev.net/Lua/breakpoints/).
It requires the debugger and interpreter, an explicitly configured new
absolute `MML_RNG_TRACE_PATH`, and an already selected owner reference run.
It adds observation breakpoints only; it does not boot/reset/resume the game,
write RAM, override pads or access save data. It stops after4096 scalar rows.

The A0 observation records service2F entries and service30 seed arguments.
The original `801022FC` observation records fullV0/low16 and actual actor
slot/state just before the store. The latter checks the proved class20 pool
range, exact record alignment and class/subtype before reading fields.
The observer is **not yet exercised against a matched New Game reference
capture**, so it is a proposed measurement tool, not evidence of retail
samples or source-owned seed. Its observations begin at the current reference
state; installing it mid-game cannot reconstruct earlier consumers. It does
not presume a BIOS state address. Retain emulator/BIOS hashes, input trace and
start-state provenance alongside any eventual trace.

The next useful capture is a fresh owner-reference New Game with observation
installed before the game entry, continuing through the first class03 spawn.
Record all A0 rand/reseed calls, callback returns, caller order, actor work,
update cadence and pause state. A BIOS-specific dynamic symbol/address may
then permit a read-only state observation; it must be established from the
selected BIOS, never guessed or carried over from another build.

## Admissible native interface

Keep `Class20_03CallbackInputs::source_random_sample` explicit and optional.
Missing ownership remains `NeedsRandomSample`; a narrower state1 path may
run only when its actual branch does not consume a sample. The whole original
callback consumes a sample unconditionally, including initializer/release.

A future gateway can supply an immutable reference sample record containing
global ordinal, caller identity, actor source identity, value and provenance.
It should prepare/reserve the next matching sample before admitting the whole
callback and commit the sample cursor only with the complete callback
transaction. That avoids losing or duplicating supplied samples during a
native evidence hold. It does not reproduce the retail global lifecycle
until all competing consumers and scheduling are connected. Do not advance a
native PRNG speculatively after a partially implemented callback.

The existing native reducer needs no RNG algorithm change for this interface.
When a real shared lifecycle is proven, seed/state belongs to the session's
global random service, not an actor instance or area package. Save/load
persistence remains unknown until independently recovered.

## Reproduction

From the project root:

```sh
python3 -B work/Legends3DS_0.0.14.19_WorldComesAlive/docs/research_01419/rng_source_probe.py \
  --exe owner-data/extracted/ROCK_NEO.EXE \
  --dat owner-data/extracted/CDDATA/DAT \
  --out work/pass-20261006-worldalive-01419/rng/source-evidence.json
```

The probe refuses unsupported input hashes and an existing output. It ran
successfully: **4** original trampoline cases, **20** callback preludes,
**3** ordered gateway cases, **1,162** interpreted probe instructions. Only
the A0 service itself is an authored return hook. Source hashes, call candidate
addresses and scalar outcomes are serialized; source payloads are not.
These results are original-instruction differential evidence on authored
inputs, **not** a retail encounter replay or hardware parity measurement.

## Current original-reference setup check

A later bounded read-only setup audit found that the inherited isolated
PCSX-Redux sessions are configured for the interpreter, enabled debugger and
WebServer ports18080/18081. Their GDB servers are disabled. Both configured
ports refused current connections; a fresh process inventory contained no
PCSX-Redux process. `rng/live-reference-setup-evidence.json` preserves the
config hashes, GET attempt outcomes and exact absent-process observation.

The task did not launch, resume, reset or control the original game, access
saves, write RAM, install a live observer or retain a raw RAM response. Thus
there are **no new measured values** for detection globals800BBD96/800BBD84,
scratch byte1F800075, actual stats selector800C1B6B or BIOS seed. The older
intro/tutorial snapshots are historical observations and cannot establish
those globals' current values or a universal initial value.

When the original reference session is available again, the documented
existing read-only RAM endpoint can capture main-RAM globals and actor
context with owner-code hash checks. Scratch must be read through a supported
mapped-memory Lua/debugger API rather than masked into the2MiB RAM response.
Capture scene/update context and actor detection phase alongside these
values. One snapshot can narrow a concrete reference case; it cannot prove
their writers, initial lifetime or all pause/detection transitions.

`redux_detection_globals_observer.lua` prepares that bounded measurement as
a separate GET handler, `/api/v1/lua/detection_globals_01419`. It reads the two
globals, mapped scratch byte, raw stats selector/pointer, player context and
all16 class20 slots (masking slot bits in the raw marker), including each
class03 detection profile/phase/countdown. It compares four bounded live
code ranges with the read-only owner EXE supplied by a hash-verified launcher;
a mismatch emits only a guard-failure record. Optional logging uses a new
explicit absolute path outside source, every120vsync, at most256 scalar rows,
and refuses an existing file. It changes no controls, RAM, registers or saves.
It remains a proposed observer until loaded and exercised in the explicit
owner reference. The root task attempted a new isolated launch; the first
follow-up GETs still refused connections and both launch logs were empty, so
`rng/live-detection-globals-context-r1.json` records no measured globals.
This agent did not launch or operate it. Rendered scene identity still must
accompany a later successful snapshot; a main-code match alone does not prove
the current scene is a matched encounter.

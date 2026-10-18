# Source fade callback and packet ownership

This slice consumes the previously retained `80012E10(slot2,callback80017E94)`
intent with a bounded native callback owner. The unchanged original dispatch
at `80017E94` selects `80017ED0` for type2's mode0. It advances the original
scalar state and exposes the original fullscreen subtract presentation.
Readiness remains **SOURCE_BACKED_PARTIAL**. A native scheduler acknowledgment
is not a PS1 BIOS thread creation, handle, deletion or completion receipt.

The owner executable identity is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
No executable instructions, source tables, retail packets or assets are copied
into the source tree. Differential fixtures contain authored work and packets.
The original owner-local executable is read only.

## Callback state and exact boundaries

At entry, signed halfword work`80098A68+0A` greater than0 initializes phase+01
as0 and intensity+0E as255. Otherwise phase becomes1 and intensity becomes0.
Byte+04 becomes0 and active halfword+08 becomes1. All other bytes and their
validity are retained. The callback immediately executes its first iteration;
acknowledgment must not defer this first update or initialize twice.

Phase0 subtracts step halfword+0C from intensity with16-bit wrap. A positive
signed halfword result is retained. A zero/negative result writes intensity0,
phase2 and the explicitly owned byte`1F800001` into step+0C. Phase1 adds step
with16-bit wrap, sign-extends the result, and retains it while less than256.
A result at least256 writes intensity255, phase2 and that same endpoint byte.
This preserves the original signed/wrapped edge behavior rather than replacing
it with a generic float fade or saturating animation.

Phase2 decrements every nonzero step halfword once. When step is already0,
the source clears active+08 only if the prior busy+0A equals1, then clears
busy+0A for every value. It calls `80012ECC`, which closes the current callback
before the subsequent yield site. The bounded consumer publishes a terminated
callback state at this source boundary. It does not fabricate BIOS teardown.
Other signed phase byte values preserve work and reach the yield unchanged.

Every nonterminal iteration reaches `80012E98(1)`. Original yield publishes
current thread state1 and wait halfword1 before BIOS `ChangeThread` at
`8007FF30`. The original scheduler's next pass decrements the wait to0 and
reaches its `ChangeThread` call. Native `advance_source_fade_callback` accepts
one explicitly admitted scheduler pass per call; it never advances by a guessed
wall-clock duration. Original rendering/scheduler cadence and native hardware
cadence have not been measured for parity.

The initial callback is transactionally admitted by
`acknowledge_source_fade_scheduler_intent`. A compatible FIFO entry is consumed
only after the complete initial iteration has its required owners. All unknown
inputs, malformed queue counts, incompatible callback/slot requests and missing
endpoint owners preserve work, packet bytes, validity, queue and callback state.
An already running callback rejects a second acknowledgment. The fixed8-entry
queue and callback updates allocate no memory.

## Endpoint frame-budget owner

Byte`1F800001` is an independent source owner. Unchanged blocks at
`80012004..8001201C`, `80013430..80013440` and `800155B4..800155CC` write2.
Other control blocks at `800167D4..800167DC` and `800169E8..800169F0` write4,
with subsequent restore blocks writing2. Seven exact writer probes retain
these observed boundaries. This prohibits treating2 as a universal fallback.
An explicitly retained fresh initialization premise may own2 until an
unsupported control/scene consumer invalidates that premise. Restored or
continued gameplay must supply its own known owner or hold at the endpoint.

## Rendering and packet ownership

Original `80017B24` returns without touching packets if active+08 is0.
Otherwise it samples the selected double buffer from byte`1F800000`, writes
intensity's low byte into the selected TILE RGB bytes+04/+05/+06, inserts that
TILE into current ordering-table word+116, then inserts the selected DR_MODE at
the same head. The resulting order is DR_MODE → TILE → previous tail.
Tag length high bytes survive; link low24 bits change. The other buffer and all
other packet bytes remain retained.

`bind_source_fade_presentation_packets` implements those exact byte/tag writes
only with explicit buffer, ordering-table and tag owners. It supplies no fake
framebuffer or pointer fallback. Buffer values outside0/1 are bounded failures.
Unknown selected packet tags hold atomically. Inactive presentation needs none
of those extra owners, exactly as in the original early return.

For a native backend, `describe_source_fade_presentation` verifies both owned
TILE constructors (command62, length3, origin0, extent320×240) and DR_MODE
constructors (length2, `E1000140`, second word0), then exposes their semantic
presentation. Its grayscale is the original intensity low byte. Draw-mode
bits5..6 equal2 and command62 enables semi-transparency: the operation is
**destination minus source**, clamped by the GPU, rather than alpha black.
The technical bit definitions are documented in the
[PSX-SPX rendering attributes](https://psx-spx.consoledev.net/ps1/gpu/rendering-attributes/)
and [rectangle command specification](https://psx-spx.consoledev.net/ps1/gpu/render-rectangle-commands/).
This description does not establish PS1/3DS framebuffer precision, dither,
quantization, draw ordering against unported UI, or pixel parity.

## Evidence and validation

`source_fade_callback_probe.py` executes **42,774 unchanged original
instructions**, **1,632 authored callback cases**, **192 authored packet cases**,
a yield publication/scheduler resume case and seven frame-budget writer cases.
There are **zero instruction patches and zero helper hooks**. It stops before
BIOS/GPU ownership boundaries and emits only authored fixtures plus scalar/hash
metadata. `fade-callback-source-proof.json` retains owner/source/fixture hashes.

`test_source_fade_callback.cpp` compares every72-byte context against those
fixtures, including both packet buffers, and checks validity preservation,
unsupported input rollback, queue FIFO admission, exact initial/terminal
lifecycle, known zero owner values and **4,096 allocation-free updates**.
The focused compile and test passed using:

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I include \
  src/flow/source_fade.cpp src/world/l3map.cpp \
  tests/test_source_fade_callback.cpp -o /path/out/test_source_fade_callback
/path/out/test_source_fade_callback \
  tests/fixtures/source_fade_callback_01428.tsv \
  tests/fixtures/source_fade_presentation_01428.tsv
```

This slice advances one exact source callback and its presentation description.
It does not claim natural tutorial completion, complete cutscenes, a live retail
trace, full gameplay timing, PS1 BIOS thread parity or physical Old 3DS proof.

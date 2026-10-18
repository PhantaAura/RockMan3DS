# Source main-loop owners and pad producer, 0.0.14.32

The new `source_main_loop` module ports original controller decode/repeat stores
and exposes exact phase/cadence gates. It supplies the missing producer for the
completion predicate's `800C0C2A` word when a retained controller owner and an
explicit packet source are admitted. It does not infer the initial scratch
halfword `1F800002`, invent main-phase progression, or execute the complete
original cooperative scheduler.

## Pad producer

The top-level caller invokes `80012298` at `80011DA4`, before the buffer flip and
before cooperative jobs resume through `80012C80`. `80012298` decodes owners
`800C0C00` and `800C0D68`, using `800121AC` and its six `80012350` repeat branches.
Their bounded first72 bytes are now retained with independent known bits.

The original packet prefix is status byte0, type byte1, active-low button halfword
2, and extra raw word4. A status of255 follows the source unavailable-pad clear.
For available packets, the consumer stores decoded type at36, held buttons at38,
prior held buttons at40, pressed buttons at42, released buttons at44, and extra
word at48. The six repeat masks are processed in original call order. Source
mode byte37 changes byte54 only in the branches that write it; other fields
retain their existing owner bytes. `source_main_loop_pad_pressed_800c0c2a()`
reads only the known pressed halfword at offset42.

The explicit executable-boot initializer applies the bounded subset of original
`80011F64/80011F78 -> 800177CC`. Those actual calls zero1228 bytes per pad owner,
including all72 retained bytes. Default construction leaves known bits unset.
Warm scene/save/reset operations do not repeat this cold clear. Unknown packet
or retained repeat state holds before publishing either port's changes. This
two-port atomic guard is a native bounded transaction, not a claim about original
failure behavior; the original source has no unknown-byte metadata.

The module accepts an explicit eight-byte packet. A native digital-control
adapter is a separately labelled platform translation; it is not an observation
of original BIOS controller chronology. The scalar decoder itself follows the
owner instructions for the supplied bytes. No native Start press, native frame
count, or pad bit seeds scratch halfword2.

## Phase and E0 gate

The selected cooperative main-phase dispatch at `800155D4` reads signed byte
`800C1B10`. Phase3 selects `800158F0`, whose subsequent dispatch reads signed
subphase byte `800C1B11` at `8001591C`. Subphases1,3,4,5,7 have unconditional calls
to update pass `80015DB4`; their paths to that call are proved separately.
Subphase2 also admits one exact store-free gate: signed byte `800C356C` is0 and
event active byte `800C4C48` is nonzero. The source jumps from `800159F0` through
`80015B08` to `80015C08 -> 80015DB4`. Missing fields, subphases0/6, and other
subphase2 branches remain explicit holds. Out-of-range signed subphases select
the source no-update return. Other main phases require their own consumer.

Within `80015DB4`, the original E0 sweep `80017528` follows other actor-family
and service calls, including `80017478`, `80017094`, `80016FD4`, `80017210` and
`800172F4`. Its fixed pool is32 records of80 bytes at `800BF118`; the source
tests `record[0] & record[6] & 1` and invokes the selected subtype callback using
the separate table owner at `1F80006C`. The existing E0 callback consumer already
uses that eligibility predicate. The new phase gate can constrain native E0
scheduling, but it does not acknowledge the preceding source consumers or fill
missing subtype-table ownership. Active resource residency is still required.

## Cadence, suffix order and scratch initialization

The main wait at `80011CE8` compares unsigned word `80098888` with byte
`1F800001`. The original callback `800123FC` increments that shared word modulo
2^32. Both optional owners must be available; host frame counts are not defaults.
The original initializer writes scratch byte0=0 and byte1=2 at `80012014/1C`.
Its bounded execution preserves arbitrary authored values of halfword2.

After buffer preparation, the unchanged caller suffix invokes, in order,
`80012C80`, `8001A110`, `8001C95C`, `800203A8`, `80053B40`, `80064060`,
`8007D02C(a0=0)`, `80017B24`, and `800178FC`. It then checks the separate debug
word at `gp+0`; the zero branch reaches the halfword increment and next-loop
jump. These are call identities, not invented names for unported services.
The suffix proof stops at each actual callee and resumes at its original return
address using a declared external boundary. It proves ordering, not those
opaque callees' internal side effects.

The inherited owner executable BSS clear excludes scratch memory. The inspected
selected installed OpenBIOS BSS interval `00004AA0..0000B05C` also excludes it.
The existing public boot reference and bounded direct-access scans do not prove
a complete BIOS/shell initialization producer for `1F800002`. Its initial owner
therefore remains UNKNOWN in this module. A future guarded original-reference
snapshot must retain its BIOS, executable and boundary identity; an observed
OpenBIOS case would not establish a retail BIOS default.

## Evidence and validation

`main-loop-source-proof.json` records256 complete authored two-pad comparisons
(94,274 unchanged original instructions), two original cold-clear loops, eight
scratch-prefix preservation cases,50 unsigned-gate cases,11 phase-selector
cases, five active-event phase-gate cases, nine vblank increment cases, and nine
original suffix call boundaries. Helper hooks and instruction patches are zero.
The fixtures contain authored scalar/pad states, not original code, retail
tables/assets or saves. The probe reads only the hash-guarded owner executable.

The new host test verifies every full pad fixture, packet-to-Start predicate,
held/released Start behavior, two-port atomic missing-owner guards, unknown
state holds, phase-gate holds, and integer wraparound. Host C++17 compilation with
`-Wall -Wextra -Werror` passes. An independent GCC16.1 ARMv6K compile with the same
warnings-as-errors passes; the decoder's reported stack use is336 bytes. All
consumer paths are fixed-size and the differential tests check zero allocations.
Root validation records connected native build and emulator results separately.

Remaining owners include initial shared scratch halfword, observed phase/control
state admission, other conditional phase paths, complete cooperative/callee
chronology, competing RNG/effect services, and physical Old3DS behavior. Retail
parity and PHYSICS_1TO1 remain false.

# Source event1 phase2 consumer

Status: **SOURCE_BACKED_PARTIAL**. `PHYSICS_1TO1=false`, retail parity false,
audible playback unproved, hardware untested. This pass implements the original
phase2 logical consumers and command producer; command execution and natural
scene completion remain held.

`source_event_phase2.cpp` consumes ST00 event1 phase2 as one transaction. It
advances phase04 from2 to3 only after the four descriptor branches, each selected
sound-state call, its matching voice stops, and the command packet producer have
actual native owners and receipts. Missing data or capacity preserves every
event/sound/voice/command/receipt field. The hot path allocates no memory.

## Original sound chain

`8001B33C` reads four8-byte slots at `80098AF8`, in order. Signed halfword+00
equal to-1 skips the slot. Otherwise signed byte+02 equal to-1 also skips.
Byte+02 zero calls `80070114(id)`; every other value calls
`8007013C(id,unsigned_byte04)`. The wrappers sign-extend their halfword
arguments and invoke `8006FF90`. No gameplay meaning is assigned to these IDs
or selectors.

The selected sequence owner corresponds to the original dereference
`*(800C4CF0+id*4)+program*176`. The native owner is explicitly keyed by signed
ID and unsigned program; it contains176 raw bytes and176 byte-validity markers.
Pointers into the original executable are never used at runtime. Multiple
descriptors selecting the same key share one native owner and execute their
mutations in source order.

`8006FF90` clears bits0/1/3/10 of word+98 and sets bit2. `80075314` scans
unsigned byte `800BC820` voices, comparing signed halfword+10 in each54-byte
voice slot to signed halfword `(id | (program<<8))`. Every match writes current
voice `800C0BFA` and executes `800742C8`. That direct call does **not** use
the phase1 wrapper's `800BC050` lock. It clears the voice's+00/+04 halfwords
and+1D byte, ORs the appropriate pending mask, then clears active bits using
the complement of each **complete pending mask**. Direct742C8 leaves v0 as
the final active high-half value; receipts retain that register result.

`800725BC` writes halfword `800B5168=0`. The remaining source stores reset
the documented sequence bytes, copy +84/+8C to+90/+94 and+56 to+54, copy
word+04 to+00/+08, initialize sixteen channel indices and64/127 fields, and
preserve all other bytes. The C++ differential compares all176 sequence bytes
and all24×54 voice bytes, including retained tails. Validity expands only
where the original stores occur.

The native driver receipt queue holds24 records. When repeated descriptors
would produce more stops than fit, the complete event transaction holds. This
is an explicit bounded native policy; the original source has no equivalent
queue capacity check. Sound-call receipts hold four records and describe
logical execution only. No sample bank, SPU playback, sound duration, or CD
timing is established.

## Command producer and initialization

The event next calls `8001D58C(20000003)`. Request bits28..29 select a bank,
bits16..27 a table entry, and low16 an8-byte pair. The owner-local profile
supplies the measured scalar ID and pair words. The reusable C++ source
contains no original lookup table payload or hardcoded request-selected data.

The producer writes the16-byte record `[4,id,pair0,pair1]` at pointer
`800989A4`, stores request `800989B8`, then copies current record
`80098A84` words+08/+0C into `800989BC/C0`, and advances the write pointer.
Those current-record reads happen **after** packet stores. Fresh current/write
aliasing therefore captures the newly written pair, rather than the old slot.
Thirty-two original differential cases exercise that alias.

`8001B44C..8001B4B4` proves the logical queue initialization slice: sixteen
type words at `800A3A40` become zero; current and write pointers equal the
queue base; `80098828`, `80098968`, `800989C8`, `800989C4` and
byte `800988D0` become zero; byte `80098964` becomes1. Record words+04,
+08 and+0C, as well as the last-request/capture globals, are retained.

`80019F0C..80019F40` writes only descriptor byte+02=FF in all four slots.
The hash-guarded executable LOAD image independently proves all32 descriptor
bytes and all256 queue bytes zero. `initialize_source_event_phase2_from_executable_boot`
admits those owner-package LOAD bytes once, before the two exact logical
initialization slices. It never implies a scene-reset or continued/load-game
zero fallback. Reset APIs preserve retained bytes and validity.

The native queue reserves one of its sixteen records as the source type0
sentinel. Count15 and beyond holds rather than overflowing the bounded owner.
Current indices outside the owner, unknown current words when they do not
alias the producer, absent profiles, and unsupported requests hold atomically.

## Command execution boundary

The original `8001C95C` dispatcher uses the current record's **type word**;
type4 selects `8001B89C` through table `80087680`. `source_event_command_front_request`
returns that function and the actual four packet words for the owning consumer.
It does not remove a record, synthesize CD status, execute a movie/message, or
write `80098AB8` completion bit20.

The type4 handler falls through from its phase0 prefix to phase1 and calls
`8001D324(200)`; later phases invoke CD/control consumers such as
`80077E88`, `8001D2BC` and `80077F8C`. Their callbacks/resource ownership
remain unported. The event's phase3 predicate still requires an explicitly
owned raw `80098AB8` byte. Neither enqueuing a packet nor counting native
ticks proves its completion.

A read-only live original input trace independently observed ST00 event1
phase1/2/3/4 on consecutive updates10380..10383, phase4 for55 updates,
phase6 for3003 updates, and phase7 for19 updates before deactivation. This
trace contains prior stage1/stage7 intro events; its retained command status
cannot be substituted for a fresh native boot or used to claim whole native
chronology.

## Focused validation

`source_event_phase2_probe.py` executes **128 authored source cases** and
**16 initializer cases** using **202,032 unchanged original instructions**,
without helper hooks. The source executes320 sequence calls and1020 voice
stops. Authored table pointers/data select safe lookup values; instruction
bytes are untouched. The fixture exports authored inputs and measured changes,
not original code/table/audio/asset/save payload.

`test_source_event_phase2.cpp` passes **120 complete source differentials**,
**8 bounded atomic holds**, **16 source initializer differentials**, missing
owner/field/capacity guards, exact validity-mask checks, current/write alias
ordering, pipeline receipts, type4 dispatch requests and4096 allocation-free
hot-path cases. Its standalone compile uses C++17,
`-Wall -Wextra -Wpedantic -Werror`.

Focused standalone command from the project directory:

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I include \
  src/flow/source_event_phase2.cpp src/flow/source_event_consumer.cpp \
  src/flow/source_event_pipeline.cpp src/world/l3map.cpp \
  tests/test_source_event_phase2.cpp \
  -o ../pass-20261008-worldalive-01423/test_source_event_phase2
../pass-20261008-worldalive-01423/test_source_event_phase2 \
  tests/fixtures/source_event_phase2_01423.tsv
```

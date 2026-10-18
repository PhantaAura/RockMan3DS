# First scripted-scene callback

Status: **SOURCE_BACKED_PARTIAL**. PHYSICS_1TO1 remains false. No hardware,
complete cutscene, natural progression or full retail frame parity is asserted.

The definitive later01428 Azahar trace advances event1 through phase4 at frame13,
phase5 at frame68 and phase6 at frame69. The constructor commits17 records and
15 owner resource references. The fade executes19 iterations and terminates at
frame77 with grayscale0 and inactive presentation. Thus the next event hold is
the missing8010102C consumer; the independent CD-service queue's phase value is
not the event phase. That observation does not by itself explain a black render.

## Native operation

The native pipeline now executes the complete8010102C callee and its first
table callback80100738. The exact supported overlay's8011B2E8 table+04 selects
that callback. A different table token holds explicitly. Event+08 is only an
enable gate against255; it is not an index into this callback table. The source
loads the callback directly from the current event+A4 record+04. Function addresses are
source identities; native code never jumps through a PS1 pointer.

The optional raw80122424==1 branch adjusts the camera word800BC744 by plus or
minus0x20000, using scratch input1F800002 bit02. Both owners must be explicitly
known. The supported overlay LOAD contains byte80122424=0; the helper admits
that premise only during fresh ST00 overlay initialization. No controller input
is synthesized when that branch needs an input word.

With event+08!=255, the first callback reads event+10. At value0 it writes the
raw scene bytes80122428/2C/30/34/38 to1/64/64/64/2, requested family40 actor+15E
to16912 and the event+30 familyE0 actor's+40 word to00808080. At425 it writes
family40 action+09=6 and+0A=0; at465 it writes familyE0 action+09=2 and+0A=0.
These are unrenamed source request lanes, not invented animation semantics.

All identities resolve to occupied, correctly tagged bounded pools. The actual
resident EBD resource witness is required while a callback is enabled. Preflight
finishes before any camera, actor or global store. A completed prefix retains a
native continuation at8001EC0C, so a missing downstream owner does not repeat
the callback. The callback itself leaves event work, phase and counters unchanged.

The new1180-byte owner-local L3P6 profile retains231 camera-program words, the
complete17 nonterminal timeline records plus the terminal18th record, and raw
area-config field800BC81C. The actual owner-data cook caught an initial record
count error before accepted output; no retail record was dropped or replaced
with an invented terminator. EXE/overlay identities, fixed coverage and CRC are
validated. This payload stays outside the source release.

The native pipeline additionally executes the complete bounded8001EC0C camera
data interpreter, including vector stores, source waits, vector integration and
the three original projection-control helpers. Raw800BC81C is obtained from the
original40-byte area config's+24 field through the proved800371A0 writer. Its
broader visual meaning is not renamed to a modern FOV or distance. Proven shared
source projection setters preserve signed-short DQA behavior and produce DQB.
Neither a PICA camera application nor projection parity is claimed here.

Original cold executable BSS clear80068000 supplies the retained188-byte camera
owner before fresh ST00 construction. Separate instruction evidence proves it
clears this range; the native operation is explicitly a fresh executable-boot
subset. Warm transitions and captured scenes do not gain zero-valued fallback
owners. Unknown inputs and unsupported commands preserve the whole camera call.

The following8001F070 timeline consumer increments the original four counters,
handles signed16 deadlines and selector changes, advances the actual eight-byte
record cursor and resets the exact counter lanes. The current bounded pipeline
executes one admitted scene update, then retains its caller continuation. It
does not repeatedly execute the callback or counters while the caller is held.

Profile rebinding invalidates its resource witness and scripted logical owners.
Accepted area transitions and save restoration clear both owners and the pending
continuation. Failed transitions preserve them. Tick admission verifies that the
resource witness still belongs to the active EBD directory and payload.

## Evidence

`source_event_phase6_probe.py` executes unchanged original8010102C/80100738 with
authored actor pools, retained camera/global bytes, input words and event fields.
Its54 cases execute2046 instructions, with zero patches and zero helper hooks.
The fixture tests compare every selected actor/camera byte and raw control
store, including retained lanes, disabled callbacks, wraparound camera arithmetic
and values immediately around the three callback conditions.

The additional camera/timeline oracle executes64 camera cases and64 timeline
cases, totaling34748 unchanged instructions with zero patches/helper hooks.
It covers every supported camera command, source wait conditions, retained vector
addition, all three projection shifts and signed deadline/counter rollover.

`test_source_event_phase6` checks these182 differential cases,16 atomic owner
guards,14 malformed L3P6 packages, shared callback→camera→timeline wiring, fresh
initialization, expired/replaced resource rejection during a continuation and4096
allocation-free continuation holds.
Standalone C++17 `-Wall -Wextra -Wpedantic -Werror` compilation and the test pass.
Cumulative host/native/emulator results are recorded in the pass report.

## Next exact owner

The next caller branch is801011E8. With a busy source fade it proceeds to the
color consumer80100614; with busy0 it can require predicate8001DE84, or the
separate type10 fade on its exact source state condition. These remain explicit
holds. No scene-complete receipt, scripted rendering, retail frame chronology,
later table callback, original actor request playback or normal transition is
fabricated. The current logical one-update advance does not finish the scene or
the World Comes Alive pass. Owner instruction listings and retail inputs stay in private work;
the source checkpoint contains only authored code, probe, fixtures and scalar/hash
evidence.

# Original event and natural progression audit, 0.0.14.22

This work continues the supplied runtime. It does not reinterpret EBD as bytecode,
use developer area skipping as progression, or replace original scenes with
authored scripts. Its current evidence label is `SOURCE_BACKED_PARTIAL`.

## Concrete connection and evidence correction

The inherited native area switch was SELECT-driven diagnostic cycling. Existing
`RuntimeSession` auxiliary trigger matching queued references; native consumers
did not execute the original ST00 stage event poll/handler. The P2 event VM is a
separate authored instruction API, not a decoder for the original event code.
The inherited 01415 correction already establishes that C0/03 is palette
animation and EBD is a resource directory.

The new `source_event_pipeline` provides a bounded portable implementation of
the original dispatcher, hash-guarded ST00 poll records, initializer, same-tick
event1 phase0, and phase4's pure non-expiry decrement. It returns exact original
consumer addresses when it cannot continue. Runtime integration is tracked by
the parent checkpoint; presence of this API alone does not establish gameplay.

The original event handler reads busy halfword **80098A72**. The inherited 01415
probe wrote **800A8A72**, which is incorrect. The new differential instrumented
actual halfword reads and tested zero and nonzero signed values. It does not use
the earlier wrong-address branch as supporting evidence.

## Original state ownership and initialization

| Field | Source | Supported boundary |
|---|---|---|
| Stage signed byte | 800C1B60 | Original stage poll selection; capture or explicit source lifecycle input |
| Transition signed byte | 800C356C | Original game initializer80015674 stores zero; future transitions need their real writers |
| Player signed HP | 800B521E, player+6E | Event initializer accepts HP>=0, including zero; negative HP refuses |
| Busy signed halfword | 80098A72 | Phase0 tests zero; coldboot0 measured in loaded EXE data, never a scene-reset fallback |
| Event work | 800C4C48,168bytes | Reset clears only00..03; retained bytes remain unknown unless captured/written |
| Source flag bank | 800BE378,256bytes | MSB-first source ID masks; native semantic snapshots map to LSB-first |

`initialize_source_story_flags_8001d8c0()` implements the explicit fresh branch:
64 source words are cleared, then80067E18 stores bank-resident words at offsets
B0=16128, B4=24, B8=128, BC=0;80067E4C sets flag1216. This bank is **not** simply
all-zero logical flags: the original metadata overlaps the flag reader's bank.
The continuation branch copies owner state from800B6130 through800176DC and is
not implemented as a fresh-state fallback.

`reset_source_event_work_8001d928()` reproduces the source word store at8001D950.
Only00..03 become known. `SourceEventWork.known` preserves unknown retained
bytes; full original captures use `source_event_work_from_original()`. The event
initializer's original stores mark their own destinations known. Unproved
active/event/phase/countdown reads hold the complete transaction.

`apply_source_st00_area_event_prefix()` supports only the proven area0/1/2
branch80100460..801004F0:

- Area0 clears player+08..0B, clears low two player+00 bits, sets source flag1540,
  and returns the opaque original store8009888C=80000000.
- Area1 writes player+44E=1 and sets its low two active bits.
- Area2 writes player+44E=1.

It does not acknowledge the surrounding original geometry, actor, camera or
audio initializers. Unsupported stages/areas hold. This is a shared source
handler prefix selected by source area, not a replacement room or exit script.

## Dispatch behavior and next consumers

The original8001D990 selects poll80100598 via80088FA8 when event work is inactive
and transition status is zero. Poll table8011ADF8 has four-byte records
`u16flag,u8event,u8opaque03`, terminated byFFFF. It selects the first enabled
flag; refusing initialization for negative HP does not try a later flag.
8001DDE4 initializes the event, and the handler runs in the **same** dispatcher
update through80089024. Event1 handler801010C4 uses state table80100018.

| Phase | First remaining original consumer | Status |
|---|---|---|
| 0 | Busy80098A72 read | Implemented with explicit input; zero advances to1 |
| 1 | 80042238(playerwork) | Source readiness/player/logical driver stores; two actual native receipts required |
| 2 | 8001B33C | Held |
| 3 | 8001D878 | Pure raw80098AB8 bit20 predicate; native status writer remains unbound |
| 4 | 80017C30 at countdown expiry | Pure non-expiry decrement implemented; expiry held |
| 5 | 80100EFC | Held |
| 6/7 | 8010102C | Held |

80042238 is now boundedly inspected. It always writes player+449=FF and ORs
player+BD with40. Its readiness branches depend on active bit00, state08,
action09, source byte800C1B93 for action14, and helper8004324C, which reads
player+174 bitsC0 and player+177. A ready branch clears player+B4 and invokes
8001B2F0 with22 and23; that wrapper calls original80071220. Those downstream
driver calls now execute through the bounded logical driver described below.
The native queue contains scalar source intents; audible playback and SPU
hardware timing remain unported. Missing fields or insufficient room for both
receipts hold the complete player/event/driver transaction.

Later actor commands are interpreted by8001EC0C (high-byte opcode), timing by
8001F070 (eight-byte records), messages by80063BC8→80063C30→80053A08 with original
message payload80153000. These are the next useful source anchors. Unknown
operations remain unknown. The historical final phase7 destination branch was
tested only under a completion premise with helper hooks; it is not permission
to transition before the original consumers finish.

## Bounded source player and logical driver consumer

`source_event_consumer` implements80042238, its8004324C readiness predicate,
the signed16 wrapper8001B2F0,80071220 and800742C8 for source calls22/23.
Lock800BC050 rejects only value1 with return-1; the original player ignores
both driver returns. Native receipts preserve rejection rather than inventing
a ready driver acknowledgement.

An accepted call clears voice800A33E0+index*54 halfwords+00/+04 and byte+1D.
The relevant pending bit is ORed into800C5680/+2. Both active halfwords
8009AE40/+2 are ANDed with the complement of the **whole resulting pending
masks**, not only the current voice bit. Current voice800C0BFA is written and
lock0 restored. Voice raw/known lanes outside the five source stores remain
retained. Both calls must receive native logical receipts before player00&FC
and event phase2 publish. Notready source branches publish only their player
stores and keep phase1. Receipt queue admission is an explicit native policy;
queue saturation holds atomically and does not claim original audio timing.

Logical initialization applies exactly lock store8006B580 and complete mask
reset800729D0..EF; voice tails/currentvoice remain unknown. Sixteen unchanged
reset cases measure only these slices, excluding sound hardware consumers.
The source sound startup80019DE0 calls8006B594→8006B4AC→80072D48; logical driver
update800725E0 has the mask reset tail. This does not establish whole original
sound initialization or audible playback.

Fresh player header ownership is explicit: the original NewGame store80013C7C
writes800C1B92=0 before callback800156A4. Then8003BE6C writes00/06=3 and clears
02..05/08..0F. A separate always-taken initializer prefix8003BEE8..8003BF10
clearsBD,74..77,184word,110/111,449. The native API applies only these stores
and marks only their destinations known. Player01/07 and all other unwritten
fields stay unknown. Forty unchanged cases prove these prefixes. Together
with the source area0 prefix, this minimal known player state can consume
phase1 without promoting the whole player mirror to source evidence.

Busy80098A72 is fade work80098A68+0A. Its coldboot0 is measured in the exact
hash-guarded executable loaded range80010000..800D9000, not inferred from BSS.
`initialize_source_event_busy_from_executable_boot()` names that boot-only
premise; event reset8001D928 never clears it. Three completion prefixes clear
it at80018010,800183F0,80018518 before80012ECC. Twelve authored contexts execute
those prefixes unchanged and stop before that scheduling consumer. Whole
native fade completion and NewGame tick ordering remain unproved.

`source_event_consumer_probe.py` measures324 full original event1 phase1 cases,
16 driver reset slices and12 busy completion prefixes with40,577 instructions
and no helper hooks. `source_event_next_phase_probe.py` covers all256 raw phase3
status bytes using8,576 original instructions: bit20 advances to phase4 and
stores countdown55, otherwise work remains unchanged. The native input must
explicitly own that byte; camera/command completion is never inferred from
a countdown or a presentation state.

The next phase2 anchor8001B33C scans four8-byte descriptors at80098AF8.
Signed halfword+00==-1 or signed byte+02==-1 skips a call. Byte+02==0 selects
80070114; other values select8007013C with unsigned byte+04. The event then
calls8001D58C(20000003), a16-byte command packet producer that dereferences
request-selected original tables. These sound descriptor/table/command
consumers remain held. Later message/table anchors above remain navigation
evidence and do not establish scene completion.

## Validation and private owner profile

`source_event_pipeline_probe.py` executes **336** authored contexts using
**21,591 unchanged original instructions**, with no helper hooks. Only the poll
table data is replaced by an explicitly authored three-record table. It compares
the complete168-byte work mutation for dispatcher/poll/init/phase0 and pure
phase4 branches. These are source differentials, not native-vs-retail gameplay.

`source_event_initialization_probe.py` executes **58** contexts and **6,810
unchanged original instructions**, with no helper hooks:16 fresh-bank cases,
24 reset/retained-tail cases, and18 area0/1/2 prefix cases. Before-work and flag
bytes are authored; source instructions supply the measured mutations.

The standalone C++ test passed with C++17, `-Wall -Wextra -Wpedantic -Werror`:
all394 original cases,11 malformed package cases, missing-premise/consumer
holds,2048 source-to-native flag mappings, and4096 allocation-free busy holds.
Fixture files contain authored data and measured mutations, not retail payload.

Exact standalone validation from the project workspace root:

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -I work/Legends3DS_0.0.14.22_WorldComesAlive/include \
  work/Legends3DS_0.0.14.22_WorldComesAlive/src/flow/source_event_pipeline.cpp \
  work/Legends3DS_0.0.14.22_WorldComesAlive/src/flow/source_event_consumer.cpp \
  work/Legends3DS_0.0.14.22_WorldComesAlive/src/world/l3map.cpp \
  work/Legends3DS_0.0.14.22_WorldComesAlive/tests/test_source_event_pipeline.cpp \
  -o work/pass-20261007-worldalive-01422/test_source_event_pipeline_r2
work/pass-20261007-worldalive-01422/test_source_event_pipeline_r2 \
  work/Legends3DS_0.0.14.22_WorldComesAlive/tests/fixtures/source_event_pipeline_01422.tsv
```

The owner-local hash-guarded `cook_source_event_poll.py` produced four records,
80bytes at `work/pass-20261007-worldalive-01422/source-events/st00.l3ep`, SHA256
`2e50c920b9da51b1dfbb928c5f1bfbde7444cac1ce0a30042679cc2b499e4eba`.
The format is bounded L3EPv1: canonical coverage, source identity, CRC, decoder,
stage and flag domains are validated before allocation. The private profile is
excluded from code-only checkpoints.

The pinned public decomp is navigation only, not copied implementation. Its
scene functions remain assembly includes, so it does not provide a completed
native event implementation. See the [pinned scene source](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/src/rock_neo/scene.c)
and [symbol declarations](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/config/syms.us.rock_neo.txt).

## Explicit unfinished boundary

Original event prefix and explicit partial initialization are source-backed.
Whole NewGame ordering, native fade lifecycle, phase2 sound descriptor/command
consumers, camera/messages, actual normal transition and package handoff remain
uncompleted. Phase1 has a bounded logical driver receipt path; playback remains
unknown. Native wiring/build/emulator results belong to the parent checkpoint.
No physical Old3DS validation, full cutscene parity or natural tutorial
completion is asserted by these tests.

A candidate shared RNG service was discussed but **not implemented here**.
The independent live-r2 capture measured installed-OpenBIOS game-entry seed0
at ordinal0. That is a measured reference boundary, not a Sony-BIOS claim or
proof that native global consumer scheduling matches. Existing strict tape
admission remains separate. No invented seed or private per-enemy generator was
introduced by this work.

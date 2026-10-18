# Original event command/CD control owner — 0.0.14.24

Status: **SOURCE_BACKED_PARTIAL**. The bounded native logical consumer is implemented; actual CD/audio transport, media timing, continued-session ownership, reset recovery and end-of-stream replay remain unverified. `PHYSICS_1TO1=false`. This work does not establish natural tutorial progression or hardware parity.

The prior event-phase3 predicate at `8001D878` reads raw `80098AB8 & 20`. The real producers are now traced: the type4 caller copies command27's response byte at `8001BA7C`; the outer command owner copies command1's status response at `8001C9E8` on every source mode2 update. `8001CBF0` clears the same status during the original pause initializer. No native tick count or packet receipt produces this bit.

The primary [PSn00bSDK CD header](https://github.com/Lameguy64/PSn00bSDK/blob/master/libpsn00b/include/psxcd.h) identifies bit `20` as the drive-reading/XA flag and supplies the command numbers below. This supports interpreting the source predicate as drive state; interpreting it as generic cutscene completion would be an unsupported inference. No SDK implementation code was imported.

## Reused owners and exact ordering

`SourceEventCommandQueue` from 0.0.14.23 retains the actual sixteen 16-byte records and scalar source labels. `source_event_command.h/.cpp` adds a bounded, heap-free command context, explicit CD requests/receipts, logical mixer intents and two consumers:

- `consume_source_event_command_dispatch_8001c95c()` owns the outer poll-before-dispatch ordering.
- `consume_source_event_command4_8001b89c()` owns the type4 state machine.

The outer owner first checks error byte `8009896C`. Nonzero errors require the original recovery path and `CdInit` wrapper `800778C0`; they hold with that exact address. In source mode2, command1 polls status and copies its actual response[0] into `80098AB8`. A reading response requires command17; its response[5..7] is converted by `80077F8C` into source word `800989D8`. The original unsigned comparison against end sector `800989B0` remains intact. Reaching the end holds at loop producer `8001D58C` when source `800989BC & 40000000` is set; otherwise it holds at pause owner `8001CB7C`. It never declares playback finished from elapsed native frames.

Before-end, the original table dispatch selects the current command. Type0 selects the complete no-op callback `8001B4C4`; the outer status-poll writes still commit when the queue is empty. That source path was executed unchanged for28 instructions with no helper hooks. Other unported command types remain explicit holds. Type4 selects `8001B89C`:

1. Phase0 checks readiness `80098964`. Already-ready input sets command mode `800988D0=2`, increments phase, and falls through. Otherwise the pause initializer stages mixer values127..1, flag `800AD142|8000`, callback installation and actual command9. Only an explicit transport-owned callback can change readiness.
2. Phase1, when ready, writes halfword `800AD14E=127` and requests mode200 through `8001D324`. An existing identical mode makes no CD call. Changing mode's bit80 establishes countdown2; the phase2 fallthrough decrements it once and returns. The next source update decrements again and returns.
3. Phase2 at countdown0 clears callback/error/readiness fields. Special source byte `80098B42==2` bypasses the filter/location/read branch exactly. Otherwise it reads the current packet, keyed file-sector metadata at `80082CD0 + id*12`, computes start/end with each packet offset masked to24 bits, and emits commands13(filter),2(location),27(ReadS) in source order. Filter bytes are1 and `packet_byte11 & 1F`. Original signed sector↔BCD functions are implemented and tested, including overflow/narrowing behavior. Only command27's supplied response[0] is copied into `80098AB8`.
4. Phase3 with bit20 clear increments source counter; at150 or greater it sets error bit10, without setting drive status. Bit20 set stages source mixer values0..126, clears source special mode and flag8000, then invokes the exact queue compaction from `8001CAAC`.

The queue pop copies all four words from each following record, including retained sentinel tail bytes, and clears only the final type word. Missing validity, inconsistent sentinel/count, or insufficient mixer capacity holds the complete transaction. The native queue reserves its existing type0 sentinel; it does not permit the original unbounded write to escape sixteen records.

## CD transport boundaries

A `SourceEventCdReceipt` names the original wrapper, command and exact parameter bytes and provides the original nonzero poll/control return conditions plus eight explicitly valid response bytes. Command receipt acceptance alone never implies bit20. The original transport wrappers retry until their lower-level results become nonzero; a missing or zero result is an explicit native hold. The current 3DS integration has no backend that can honestly furnish these receipts.

`source_event_cd_sync_callback_8001cc08()` executes the logical callback stores. Code2 unregisters the sync callback, sets readiness1 and callback state0. Other codes unregister it, set callback state128 and OR error8. Missing retained error fields are not fabricated. Unsupported callback owners remain held. Logical mixer receipts are source register values, not audible playback or a new hardware emulator.

## Cold LOAD versus source initialization

The owner-local executable LOAD snapshot has zero values for the listed context fields. These zeros are valid only for that measured cold image; the native code has no blanket BSS or scene-reset assumption. The optional neutral package supplies their raw values and validity. Source queue initializer `8001B44C` explicitly writes readiness1, command mode0, phase/counter0 and type words0; it does not initialize all retained CD context fields.

| Context address | Width | Role retained from source |
| --- | ---: | --- |
| `800988EC` | 1 | CD mode |
| `8009896C` | 1 | Error bits |
| `800988C0` | 1 | Callback state |
| `80098B42` | 1 | Special branch selector |
| `80098AB8` | 1 | Last source drive-status byte |
| `800AD142` | 2 | Mix flags |
| `800AD14E` | 2 | Source volume scalar |
| `800989B0` | 4 | End sector |
| `800989D8` | 4 | Actual polled sector |
| `80091FFC`, `80092000` | 4 each | Source callback owner labels |
| `800AD154` | 4 | Last mix bytes, individually valid |
| `80098A98` | 8 | Last response bytes, individually valid |

File id245's sector metadata is read from the hash-guarded owner executable by the package cooker; no retail table or sector constant is baked into the runtime. Fresh diagnostic cold-load admission is separate from continued sessions and save restore.

## Evidence and verification

`source_event_command_probe.py` verifies the supported EXE and overlay hashes, then executes only the EXE functions needed here. It avoids allocating unused overlay bytes per case. Authored inputs and output scalars/diffs are the only fixtures exported; source instructions/tables/media are not exported.

- 335 complete dispatch cases execute original instructions, actual mixer callees and queue compaction without helper hooks. They intentionally avoid the unported CD transport body through already-mode200/countdown/specialmode2 branches or source phase3.
- 512 signed sector→BCD and512 arbitrary BCD→sector cases cover original arithmetic and narrowing.
- 256 unchanged source callback cases cover every callback code.
- 6 transport-boundary observations capture the real mode/filter/location/read arguments, source response-tail stores and pause prefix. Their transport responses are authored boundaries, not a driver parity claim.
- 32 additional outer-caller cases execute the original poll/status/location/type4 ordering around48 explicit authored CD-response boundaries. Error recovery and reached-end branches are native guarded holds.

The C++ test compares full queue bytes, source scalars, mixer call sequences, all arithmetic cases and the outer ordering. Additional tests also ensure empty no-op dispatch retains actual outer status receipts and cover mismatched/zero/unknown receipts, unknown fields, capacity, retained sentinel validity, reset/loop/pause holds and4096 no-allocation ticks for each consumer. Authored complete transport-owner tests are explicitly separate from retail chronology.

An initial proof-observer error counted a channel value32 on CD register bank2 as the mixer-apply value32 on bank3. The differential caught native127/source128 at seed68. The observer was corrected to the actual bank3 apply store; failed observations were preserved outside the source tree in the pass directory. The implementation was not changed to agree with an incorrect fixture.

No Old 3DS, audio output, real CD backend or full progression test is claimed here. The next concrete source boundary is recovery `800778C0` and the reached-end `8001D58C`/`8001CB7C` owner, together with a measured neutral audio-stream backend that supplies actual status/location receipts.

## Actual stream identity and retained-data gap

The current packet's file id245 resolves through the original file table to LBA91731. Read-only ISO9660 directory metadata maps that exact sector to owner file `XA37S00E.XA`; the adjacent id246 maps to `XA37S01E.XA` at LBA147491. These are numeric asset labels; no story or audio-content label is assigned.

For id245 the source file record reports130255360 bytes:55760 sectors of2336 bytes. The directory and existing extraction report114196480 bytes:the same55760 sectors projected to2048 bytes. Id246 shows the identical2336↔2048 relationship over59504 sectors. The directory XA attribute is2555 and file number1. This is a substantial importer fidelity gap: the existing2048-byte XA views do not retain complete raw sector/subheader/audio groups and cannot honestly be fed into a native XA decoder as full streams.

The packet's exact24-bit-masked span is91731..107315 (end exclusive),15584 physical sectors, source file filter1/channel3. A targeted read of only24-byte headers for the first64 sectors confirms audio/Form2 submode64, duplicated subheaders and channels0..7. Channel3 occurs at relative sectors3,11,19,27,35,43,51,59. Coding byte01 maps to stereo/37800Hz in the primary [FFmpeg PlayStation stream parser](https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/psxstr.c); no implementation code was imported and no original audio payload was read, decoded or exported by this check.

The inherited audio catalog has39 identity records and `routing_proven=false`; no cooked PCM or NDSP playback consumer is present in the supplied runtime. Therefore a truthful native transport needs a targeted owner-local full-sector stream importer/decoder and an exercised bounded audio backend. Merely acknowledging Setmode/filter/read commands or advancing a synthetic sector timer would still invent drive state. This pass records the exact source request, asset identity and data-loss blocker for that work; it does not fake playback to make event phase3 pass.

The metadata-only proof is `docs/validation_01424/source-stream245-directory-proof.json`. The owner CUE/BIN/XA files were read only; no original payload is included in the source checkpoint.

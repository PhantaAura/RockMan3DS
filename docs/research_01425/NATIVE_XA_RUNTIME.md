# Native XA runtime — 0.0.14.25

The original startup stream has a real full-sector host importer and a native
indexed PCM reader/NDSP backend. This is a source-backed partial implementation,
with a candidate native protocol adapter. It is not PS1 emulation, proof of CD
drive timing, a complete audio engine, or confirmation of physical Old 3DS sound.

`XaStreamReader` validates the L3XA v1 header, supported source identity,
canonical dimensions, complete interleaved sector span, index sequence, exact
file length, and header/index/PCM CRCs. Opening scans the PCM with a fixed 16 KiB
scratch buffer; the entire stream is never resident. The index has at most 4,096
entries, each 24 bytes. Later reads validate each PCM sector again. SHA fields
are provenance metadata; on-device CRC checks do not authenticate their SHA
values. The host importer and private SD manifest provide full SHA checks.

The measured startup stream retains a 46,752-byte index and uses two 8,064-byte
linear buffers. NDSP channel 0 plays 37,800 Hz stereo PCM16. Buffers are refilled
only after the SDK marks them DONE. Cache flush precedes queue submission; clear
and `ndspExit()` precede freeing linear memory. The refill loop has no dynamic
allocation. Stack/heap totals and physical SD latency still require measurement.

The independent gate is `sdmc:/3ds/Legends3DS/source_audio.cfg` containing
`SOURCE_XA_AUDIO=1`. It defaults OFF. The current source startup command subset
also requires `source_event.ini` with `SOURCE_EVENT_BOOT=1`; it remains a fresh
diagnostic subset, not a faithful complete New Game sequence. The private stream
lives at `data/legends3ds/tutorial/audio/stream_245.l3xa`. Missing/invalid data or
an actual `ndspInit()` error leaves transport unavailable while the renderer
continues. No DSP firmware or cooked audio is distributed in source releases.

The adapter accepts only the measured mode, filter and start location. ReadS
first submits PCM, then waits for actual `ndspChnIsPlaying()` before acknowledging
reading. It never synthesizes successful playback from elapsed game ticks.
Getstat uses actual backend state. GetlocP exposes absolute position from DONE
buffers only; its track/index/relative prefix remains UNKNOWN. Position is
coarse (2,016 frames per buffer), not a measured hardware head position. Unknown
pause/recovery/seek/loop requests explicitly hold. Source diagonal mixer intents
are removed only after actual NDSP application; unsupported cross-mixing holds.
Initial unity gain is a native diagnostic setting, not an inferred retail value.

The outer source consumer reads response byte 0 for Getstat and bytes 5–7 for
GetlocP. Only those fields must be known for that consumer; the type 4 consumer
still retains its stricter complete-response guard. Unknown prefix fields are
not filled with invented values. Native CD state/timing translation is
**CANDIDATE**, even when actual PCM playback is observed.

Accepted area replacements invalidate outgoing stage metadata, command queue,
pending receipts, context and mixer intents, then stop/free the native audio.
Rejected preparation preserves active owners. This lifetime rule is exercised
by authored regression cases; developer area switching is not natural tutorial
progression. Final session exit also stops audio explicitly.

Validation includes always-on Release checks, malformed/truncated/spanned
package rejection, changed-after-open CRC failure, 4,096 allocation-free reads,
explicit close, unavailable/queued/playing/failed transport states, and a
RuntimeSession integration case that waits for an authored observed backend.
That case is not NDSP or retail parity evidence. The owner-local reader checks
all 1,948 PCM sector CRCs; the decoding history is explicitly COLD_ZERO. Retail
seek history, independent hardware decoding parity, accurate event cadence,
end-of-stream ownership and physical Old 3DS performance remain unverified.

The actual r4 binary was exercised in Azahar 2126.0 with original-model settings,
100% CPU and 1x rendering. The initial safe-renderer test without a component
returned `0xD880A7FA`, with no false playback receipts. The owner supplied an
existing local DSP dump; it was copied only into the private test SD tree. In
the later audio-enabled run, initialization returned zero, actual PLAYING and
DONE buffer states advanced to 437,472 completed frames in the sampled log,
with zero sampled underruns/dropped DSP frames. The stream command popped and
source event phase 3 advanced to phase 4 at native frame 13. At frame 68 the next
source consumer, `80017C30`, held on missing context. This does not prove full
original scene completion. A developer area switch stopped playback and logged
zero owned PCM/index bytes. The stream was intentionally interrupted; complete
end-of-stream playback, audible quality and hardware parity were not tested.

The observed audio open took about 5.35 seconds in that emulator run; the maximum
recorded PCM sector read was 2,312 microseconds. These are emulated file/DSP
measurements, not Old 3DS SD or timing guarantees. Total SDK audio allocations,
nested stack peaks and true worst-case refill latency remain hardware pending.

libctru's [2.7.0 component loader](https://github.com/devkitPro/libctru/blob/v2.7.0/libctru/source/ndsp/ndsp.c)
looks for `/3ds/dspfirm.cdc` or the `hb:ndsp` handle and returns the observed
missing-component result if neither supplies it. The source release contains
neither this component nor the owner-derived audio package.

Primary SDK references: [native streaming example](https://github.com/devkitPro/3ds-examples/blob/master/audio/streaming/source/main.c)
and [libctru NDSP channel implementation](https://github.com/devkitPro/libctru/blob/master/libctru/source/ndsp/ndsp-channel.c).

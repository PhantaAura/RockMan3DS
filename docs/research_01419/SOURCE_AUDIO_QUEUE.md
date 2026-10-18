# Source audio request producer ownership

Status: **SOURCE_BACKED_PARTIAL** for queue production and retained fields.
Audible playback, original sound bank decoding and native driver timing remain
unported. Sound requests are not a fabricated audio playback implementation.

Original scalar producer8001A008 writes to a20-byte slot: type byte0=0, low16
request bits at+2, rawa1 word at+4 and rawa2 word at+8. Byte1 and bytes12..19 are
retained. For the detector's request156 the original arguments are156,0,0.

Original pointed producer8001A050 writes type byte0=1 and request+2, then copies
raw words from a1+0 and a1+4 into slot+4/+8. It does not consume a2. These words
must come from the actual source pointer/retained raw actor fields; the API does
not rename them as guessed volume/pan/coordinates or repack an approximate
render position. Actor action2/3 use this source producer for their recovered
sound requests; source class/actor ownership still controls admission.

The write pointer80098938 begins at800BE4F8 and stops at800BE6D8:24 slots of
20bytes. Both producers silently drop when the pointer equals the end. This is
a legitimate source producer result and must not stall actor gameplay. A
malformed native count above24 rejects safely because the source pointer
invariant would have been broken. Initialization80019E14..24 installs the base
pointer; it does not clear slot bytes. Reset8001A1D0..1DC likewise retains them.

The source consumer8001A110 runs from the main loop at80011E78 after main
update dispatch80012C80 at80011E70. It first tests global80098958 and skips both
consumption and reset when that global is zero. For admitted consumption it
looks up the request's owner table/dispatch handler, iterates all queued slots,
then resets the write pointer. Those table/driver/sample consumers are not
proved by this new module. A native diagnostic drain must be labelled as such;
retaining requests to full capacity and dropping later requests also preserves
the producer contract while playback is absent.

`SourceAudioQueue` uses a fixed array and accepts explicit raw source arguments.
A caller can copy/stage it, enqueue preflight requests, and publish it together
with the actor's successfully prepared behavior/pose. Failure before publication
leaves the shared queue untouched. No hot-loop allocation occurs.

Read-only original proof:

```
python3 -B docs/research_01419/source_audio_queue_source_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE --out /new/owner-local/audio-proof
./l3test_source_audio_queue \
  tests/fixtures/detection_01419/authored-audio-queue.tsv
```

360 original/native producer comparisons cover both producers, counts0/1/22/23/
24, request156/176/180 and wrapped request bits, raw signed-looking words,
retained slot bytes and full-buffer drop.25 original pointer resets and the
initializer pointer assignment pass. The probe executes6,441 original
instructions. Native test additionally stages an unpublished request, rejects
uninitialized/malformed queues, and runs480 batches×24 requests with zero
allocations. All source fixtures contain authored byte patterns/scalars, not
commercial sound or executable payload.

Fixture SHA-256:
`6dc26f9e296e16245bef62784432b04ea9477c7ac0a54ec640753fe50e859066`.

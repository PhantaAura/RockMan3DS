# Original prop attachment controllers

Status: **SOURCE_BACKED_PARTIAL**. `update_source_prop_attach_80112d14()`
implements parameter0 states1,2 and3 using the original class40 pose bank.
It consumes the existing imported model and source integer pose. There is no
new model, authored hand motion or floating-point attachment approximation.

## Source transforms

The source actor pointer is event+`0x24`, original global`800C4C6C`. A bounded
native owner resolves that token against the actual allocated class40 pool,
checks class/subtype, and requires the consumed pose bytes to be known.
The source pose owner must run before the corresponding E0 update; this leaf
supplies no scheduling or new pose generation.

- State1, `80112D14`: rotate by source angles1024/0/0, compose actor bone3's
  matrix at byte672 with that quarter-turn matrix through saturated GTE IR
  products, and scale the submitted matrix by the source32768/Q12 factor.
  Separately transform local offset[80,0,-320] through the **unscaled** composed
  matrix, then add the bone3 world halfwords at408/410/412 with16-bit wrapping.
- State2, `80112E80`: submit the same composed/scaled matrix. The position is
  the signed average, rounded toward zero, of bone4 at416/418/420 and bone7
  at440/442/444. It does not reuse the state1 local offset.
- State3, `80112F70`: reset the original32-byte matrix block, install the
  source quarter-turn rotation, and use bone0 position at384/386/388 with20
  subtracted from X. It does not retain the state1 scale.

Each original handler writes modulation`02808080`. This is a source material
word requiring the separate semitransparency renderer implementation; this
module does not invent a modern alpha value or claim blend parity.

The original state1 copies32 temporary stack bytes although its matrix helper
produces only20. The remaining12 bytes have no admitted owner. Native code
preserves their raw storage and clears their known markers at prop44..55.
They are not filled with an invented zero. States2/3 have different, proved
write sets and do not inherit this state1 invalidation.

## The linked release branch

When state1's signed byte12 is nonzero, the original calls`80031988` and then
**continues** the attachment operations. The helper assumes a family80 record,
but here receives an E0 address. Its exact unsigned multiply/shift calculation
therefore selects foreign global words rather than the E0 allocation mask.
For example, E0 slot14 at`800BF578` uses pending`8009898C`, used`800988FC`, and
bit`08000000`. It stores `(pending|bit)&used` and clears the prop's byte00.

`SourcePropForeignReleaseOwner` requires the exact prop identity, computed
addresses, and known admitted word values before any mutation. Unknown or
mismatched ownership holds the entire callback atomically. The native code
never substitutes an E0 free or silently writes unrelated native memory.
Only that typed pending owner is updated on success; its used word survives.
The current runtime can leave this auxiliary representation explicitly held
until its external words are genuinely owned.

## Differential evidence and limits

`source_prop_attach_probe.py` executes **56,772 unchanged MIPS instructions**
for180 authored cases across all three handlers. The source rotation, matrix
composition, scaling, offset, midpoint arithmetic and release helper execute;
there are zero instruction patches and zero helper hooks. The established
restricted GTE interpreter executes504 MVMVA instructions, including64 clipped
IR components in32 cases. Unused GTE flags and timing are outside that model.

The fixture covers identity, negative, dense saturated and fractional bone
matrices; standard and adversarial authored trig inputs; signed halfword
position bounds; positive/negative odd midpoint sums; prop slots0/14/31;
actor slots0/3; and36 foreign-release branches. It contains authored scalar
data only. State1's unowned12 original stack-copy bytes are redacted from the
public expected output and replaced with the unchanged authored input bytes,
matching the explicit native unknown-storage contract. Those12 bytes are not
claimed to match original stack contents.

The native test compares source-known output bytes and exact validity behavior,
foreign mask effects, and unchanged storage. It passes180 cases plus6,156
unknown-field, stale-owner and foreign-release atomic guards. C++ heap
allocation counters remain unchanged during every callback. Clang and actual
devkitARM ARMv6K GCC compile with warnings as errors; GCC reports304 bytes of
static callback stack. This is not a measurement of whole-frame stack peak.

The additional `test_scene_e0.cpp` authored RuntimeSession integration passes
through the actual resource lease, occupied E0 slot, wrapper and attachment
leaf into the E0 draw adapter. Bone3 scale512/XYZ[100,200,300] produces source
position[110,240,300], the exact quarter-turn submitted matrix and modulation
02808080. Missing parent pose and a detached copy of the prop both hold without
mutation; callback/wrapper/draw allocate no heap memory. The targeted CTest
`scene_e0` and `source_prop_attach` tests both pass after rebuilding their actual
host targets. This verifies publication to the draw adapter, not GPU blend
parity or a live scene-to-gameplay handoff.

No live original grab-frame capture was collected by this probe. Original
scene cadence, full event completion, semitransparency rendering, source-vs-live
prop alignment and physical Old3DS performance require separate evidence.

```sh
python3 -B docs/research_01439/source_prop_attach_probe.py \
  --exe /absolute/path/to/already-supplied/ROCK_NEO.EXE \
  --overlay /absolute/path/to/already-retained/owner-overlay.bin \
  --out /absolute/path/to/new-private-output
ctest --test-dir /absolute/path/to/host-build -R source_prop_attach --output-on-failure
```

The proof JSON records immutable owner hashes, public fixture digest, counts
and limitations. Original instructions, model/texture payloads and decompiled
listings remain outside the cumulative source checkpoint.

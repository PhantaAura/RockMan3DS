# Phase5 retained EBD resource admission — 0.0.14.29

Status: SOURCE_BACKED_PARTIAL. PHYSICS_1TO1 remains false. Hardware is pending.

The 01428 L3P5 loader checked the EBD SHA-256 named in its header but discarded
that identity. The constructor then accepted resource tokens without comparing
them with the active area's retained EBD. Presence of two individually valid
packages did not prove they belonged together.

The loader now preserves the header digest. A load-time bounded owner admission
hashes the actual immutable source bytes retained by L3EBv2 and compares them
with that digest. It compares all source directory keys and pointer-derived
offsets with the neutral directory, then checks each selected record against its
source family, subtype, variant, directory index, first-match order, resource
slot0, and in-payload offset. Pointer identity does not establish resource format
or an inferred resource extent. Original virtual addresses remain identity tokens.

`bind_source_event_phase5_resources(profile, directory)` returns a fixed-size
witness. The constructor requires that witness and rejects it if the profile has
changed or its immutable payload lease expired. `resident_in(directory)` also
checks active immutable storage identity and unchanged neutral directory metadata, so a replacement fails even if another
registered area still owns the old shared bytes. A caller must clear or replace
the witness on an accepted area transition, save reset or profile rebind. A failed
transition keeps the existing admission. Fresh event-state reset can retain a
valid profile/resource admission; it does not manufacture new resources.

The witness holds a weak lease and a bounded profile copy, not a payload copy or
raw lifetime-dependent resource pointer. Host witness size is 816 bytes, including a 14-row neutral metadata snapshot. The
supported profile requires the already-proved original load base and 14 rows;
other profiles need separate evidence. A 256KiB admission cap bounds SHA work.
Admission and constructor checks allocate no memory. SHA uses a fixed 64-word
schedule. Native stack usage and Old3DS frame/load cost require the native build
and device measurements; host timings do not establish device performance.

The existing L3P5v1 file already carries the necessary digest, so no format
migration or texture/model/area recook is required. L3EBv1, which has no retained
payload, cannot gain admission. The source cooker now records the expected
runtime admission rule in its evidence output.

## Verification

The new authored test uses seven independently computed Python `hashlib` digest
vectors, including SHA padding boundaries and a 113808-byte payload. It checks
12 malformed profile/directory/digest/index/variant/offset joins, first-match order,
profile mutation, immutable replacement, shared lifetime, complete unload, and
4096 allocation-free expired-witness holds. Existing unchanged-instruction
constructor differential cases retain their authored inputs and now supply a
consistent authored EBD witness.

An owner-local test loads the unchanged 01428 sidecar with the retained corrected
area_00 L3EBv2. The actual bytes match the header digest and all 15 selected
resource joins; the source constructor commits. No original bytes, model, image,
audio, executable, or owner-local package is included in these source fixtures.

This proves package ownership admission for the supported constructor. It does
not prove scripted actor rendering, animation semantics, callback execution,
scene completion, retail parity, or physical Old3DS behavior.

Exact focused commands (source directory):

```sh
cmake -S . -B ../pass-20261011-worldalive-01429/host -DCMAKE_BUILD_TYPE=Release
cmake --build ../pass-20261011-worldalive-01429/host --target test_source_event_phase5 test_source_event_phase5_resources -j 4
ctest --test-dir ../pass-20261011-worldalive-01429/host --output-on-failure -R '^source_event_phase5'
```

Result: 3/3 CTest cases pass (0.14 seconds); scoped C++ targets use warnings as
errors. The cooker case includes 9 authored Python tests. Owner verification is
separate from CTest and uses the test executable's optional two-path mode.

# L3EB source resource preservation

L3EB version 2 retains the complete original EBD byte payload alongside the
existing normalized lookup rows. This preserves exact resource pointers,
aliases, overlaps and pointed bytes without assigning a resource type or
inferring a resource's extent from the next pointer. Resource semantics and
render bindings remain UNKNOWN. Owner-derived cooked data belongs only in an
owner-local install; source ZIPs contain code and synthetic fixtures only.

All integers are little endian. Common header fields retain their v1 offsets:

| Offset | Type | Field |
| --- | --- | --- |
| 0 | bytes[4] | `L3EB` magic |
| 4 | u16 | version: 1 or 2 |
| 6 | u16 | header bytes: 32 for v1, 40 for v2 |
| 8 | u32 | flags: exactly 3 for v1, 7 for v2 |
| 12 | u32 | entry count |
| 16 | u32 | original source load base |
| 20 | u32 | entry table offset, equal to header bytes |
| 24 | u32 | exact complete cooked file size |
| 28 | u32 | CRC32 of every byte after the header |
| 32 | u32 | v2 original source payload offset |
| 36 | u32 | v2 original source payload byte count |

Flag bit 0 means resource pointers are normalized to source-relative offsets;
bit 1 means `UINT32_MAX` represents original pointer zero; v2 bit 2 means
complete original source bytes are present. Unknown flags/versions are rejected.
Each cooked row remains 16 bytes: four key bytes, then three u32 offsets.
For v2, the unchanged original EBD immediately follows the cooked rows. It
includes its original u32 row count, original 16-byte rows with absolute
pointers, and all following bytes. The appended bytes are not relocated or
rewritten. The normalized offsets always refer to this original source payload,
never to the cooked header/table.

`parse_ebd_directory` owns a complete copy of the source in
`EbdDirectory::source_payload`. `cook_l3ebd_directory` emits v2 when that vector
is nonempty. Empty payloads emit v1 so constructed/loaded legacy directories
remain usable without fabricated bytes. `load_l3ebd_directory` accepts both
versions; v1 retains its original offset-only behavior and has no payload.
`TutorialRuntimeArea` already owns an `EbdDirectory`, so v2 bytes survive its
existing runtime loading path without a renderer change.

Parsing checks the count/table bounds, source size and source address range
using widened arithmetic. Non-null absolute pointers must lie inside the
source payload. Cooking rejects count/size truncation, and validates its output.
Loading checks exact sizes, contiguous table/payload layout, allowed flags,
source address range and optional CRC. For v2 it also checks that the original
source count and all key/pointer fields agree with the normalized cooked rows.
CRC covers the normalized rows and opaque source bytes; it excludes the header,
whose structural fields are validated separately. Disabling CRC does not
disable any structural or pointer validation. V1 cannot validate an offset
against absent original bytes, so it does not claim resource availability.

The lookup API is:

```cpp
EbdResourceSpan ebd_resource_span(const EbdDirectory& directory,
    size_t entry_index, size_t resource_slot, size_t requested_bytes);
```

The caller supplies a known byte length. It returns a borrowed exact window
with `AVAILABLE`, `NULL_POINTER`, or `PAYLOAD_UNAVAILABLE` status, pointer,
size and source offset. Invalid row/slot and an out-of-range available-payload
window throw. Bounds use subtraction rather than an overflowing offset-plus-
length calculation. Zero-length windows at a valid non-null pointer are allowed;
one-past-end pointers are rejected. Original null pointers and missing legacy
payloads return no data/size. Aliased pointers yield the same borrowed address;
overlapping windows are preserved. No allocation occurs in the lookup.
Borrowed spans expire on payload mutation, directory move or destruction.
Callers must not infer object/model/animation lengths from this API.

`tests/test_ebd_resources.cpp` checks exact v2 source-byte/row roundtrips,
byte-stable v1 roundtrips, alias/overlap/null states, source-directory pointers,
empty directories, last-byte and overflowing window bounds, truncated headers,
unsupported flags/versions, malformed counts/ranges, row/source mismatch and
opaque-byte CRC failure. Fixtures are entirely synthetic. The CMake target
`test_ebd_resources` enables assertions and warning-as-error compilation.
Source-backed owner EBD coverage is recorded separately by the owner-local audit;
these synthetic tests do not establish object semantics or retail GPU parity.

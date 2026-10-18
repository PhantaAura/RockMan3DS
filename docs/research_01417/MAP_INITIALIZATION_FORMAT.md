# Neutral stage0 startup materialization

Status: `SOURCE_BACKED_PARTIAL`. `map_initialization.h/.cpp` translate the
proved original `0x8003A3B4` materializer into bounded neutral metadata. The
only admitted startup profile is stage0, whose next original variant call
exits before its separate mutation path. This format does not reconstruct
later saved state or stages5/6 variant mutations.

The host cooker scans exact source HED pointer rows, retaining the first
masked descriptor match before a zero terminator or the512-row cap. Unmatched
IDX records retain their fields. A populated identity0 is eligible. Each
admitted record stores its original unsigned raw6 field, neutral descriptor
identity, primary-near render-record identity and unsigned record+2 seed.
There is no blanket alternate-list or geometry-bank override.

The existing cooked L3MP/L3RG files remain unchanged. The new owner-local
`.l3init` file binds to both complete cooked files with CRC32. Derivation
checks original IDX positions/flags/descriptor identity, source descriptor
fields and render offsets, then compares the complete rebuilt neutral L3RG
representation in memory. Original HED admission is not inferred from the
parser's descriptor union.

## L3MI version1

All integers are little endian. Header length is48 bytes; each entry occupies
16 bytes. The encoded upper bound is32816 bytes and2048 entries. There are
no optional trailing sections or accepted nonzero reserved fields.

| Header offset | Field |
| --- | --- |
|0 | Four-byte magic `L3MI` |
|4 | Version u16=1 |
|6 | Header length u16=48 |
|8 | Startup profile u32=1 (stage0) |
|12 | Complete cooked IDX count u32, at most2048 |
|16 | Complete neutral descriptor count u32, at most65535 |
|20 | Complete neutral render-record count u32, at most65535 |
|24 | Entry count u32, no greater than IDX count |
|28 | Original complete L3MP CRC32 u32 |
|32 | Original complete L3RG CRC32 u32 |
|36 | Entry offset u32=48 |
|40 | Exact encoded file length u32 |
|44 | Entry payload CRC32 u32 |

| Entry offset | Field |
| --- | --- |
|0 | IDX identity u16 |
|2 | Expected neutral descriptor identity u16 |
|4 | Expected primary-near render-record identity u16 |
|6 | Expected original raw6 u16 |
|8 | Derived seed u16 |
|10 | Reserved u16=0 |
|12 | Reserved u32=0 |

Entries must have strictly ascending IDX identities. Inspection validates
format, bounds, exact length, checksum, every index, order and reserved field
before allocating a vector. Its `decoded_bytes` estimate includes the local
`MapInitialization` object and its decoded entry array. The estimate adapts
to the host/3DS structure sizes.

Runtime validation checks both original file bindings, all header counts,
every existing IDX raw6/descriptor field, the primary-near record mapping,
and the retained render header seed. It completes all checks before changing
any field. Apply writes only raw6; valid inspection, validation and apply
allocate no heap memory. Reapplying to the changed owner tutorial map fails
the original-field preconditions. Absent metadata remains a caller-controlled
compatibility path; no missing record is filled by guessing.

## Validation and owner fixture

`test_map_initialization.cpp` checks synthetic source admission, populated
identity0, early HED termination, masked pointer identities, the last IDX
identity2047, the last searched HED slot511 and excluded slot512, unmatched
retention, high-bit unsigned seeds, malformed metadata, mismatched source and
neutral ordering, CRC bindings, and rejection before partial mutation. Valid
preflight and runtime application also have an allocation counter check.

The strict C++17 host test passes against the verified baseline core. The
actual owner cooker produced994 entries for995 IDX identities,117 neutral
descriptors and321 render records. The file is15952 bytes; the host decoded
estimate is9996 bytes. Its SHA256 is
`d84f875e120fb5260a8687b68a9c11141973aa8d8fa1484a5eb5fa647ed227e5`.

`idx_map_initialization_owner_compare.py` executes the original integer
region again, then passes its995 unsigned field scalars through the local
native helper's standard input. The helper compares every result directly
after applying the checked metadata. All995 fields match;994 writes occur;
all XYZ/descriptor fields remain unchanged. The expected scalar array is
discarded. Saved hashes/counts are in
`idx-map-initialization-owner-comparison.json`.

The original source-region synthetic ledger also passes137 cases, executing
2,827,519 instructions across the authored edge premises. The actual owner
fixture executes1,543,539 instructions independently. These are bounded
integer/source and native-host checks, with no complete gameplay parity claim.
Live load timing remains a separate original-reference observation; Old3DS
execution remains `HARDWARE_PENDING`.

## Cold owner pack hook

`build_tutorial_pack.py` invokes `l3cook_map_init` after its existing map/render
cookers with explicit `--stage0-startup`, preserves optional HD2 input, and
records metadata provenance plus the usual output hash. Its fresh staging
directory and exclusive publication policy remain in effect. The initializer
cooker also uses exclusive file creation. Existing code-only install/update
flows continue to consume verified cooked inputs and do not decode owner data.
The generated `.l3init` file belongs with owner-local cooked assets and must
be excluded from source-only releases.

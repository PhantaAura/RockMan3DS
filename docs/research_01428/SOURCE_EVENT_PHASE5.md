# ST00 event1 phase5 native constructor

Status: **SOURCE_BACKED_PARTIAL**. Full gameplay/camera/cutscene parity is not
asserted. PHYSICS_1TO1 remains false; physical Old3DS testing is pending.

The exact supported owner executable is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`;
ST00 overlay is
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`;
area0 EBD is
`ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15`.

## Recovered operation

The phase5 dispatch at801011B4 calls80100EFC, then the original caller increments
phase to6. Constructor80100EFC first calls8001E6C0(0,0,0), which writes the
logical camera mode and original fixed-point retained fields. The signed-offset
camera block is **800BC738**, with directory word800BC7F0. Earlier local
investigation incorrectly used800C4738; the unchanged instruction oracle caught
that address error before accepted evidence and integration.

Six explicit source input words are800A39A0/A4/A8 and800A3964/68/6C. The original
writer prefix80038674..800387B8 establishes eye words from signed player
position halfwords+14/+16/+18, raw rotation words512/0/48 and raw mode1.
The native helper requires all six player position bytes known. This helper is
an initialization prefix; current native coordinates and camera timing are not
claimed to match a retail frame. The constructor rotates the raw angle lane
order to3968,396C,3964 before its12-bit masks and16-bit shifts.

The constructor processes16 owner-derived20-byte records through8001EAE8 and
8001E968: one requested family40 object and15 familyE0 objects. The exact cold
pool-reset prefixes are8002E88C,8002E924,8002E9CC, including the original
inclusive-word clear800177CC and `family | slot` byte01 tags. The requested
family60 allocation uses the original physical family80 pool. Native allocation
scans the same MSB-first bitmaps, with explicit bounded capacity checks before
any store. Original code does not protect a failed allocation; native failure
holds the entire transaction rather than writing through a null identity.

For indices1..15,8002F9C4/8002FA38 resolves familyE0/subtype3 plus the original
variant byte. Each source lookup writes selected EBD resource identity to+14;
it does not invoke the model decoders in that constructor. An initial local
inspection incorrectly described the absence of nested decoder calls as a
resource miss; selected output pointers prove all15 are resource hits.
The owner-local cooker retains each actual selected directory index/variant
and resource pointer as an identity token. Native code never dereferences a
PS-X address. The imported EBD payload and actual rendering remain separate
owners, and drawing the constructed records is not claimed here.

The complete port preserves retained bytes and performs original color stores,
the additional physical-family80 object construction, all event+24..64 binding
tokens, event+A4 callback-table token and raw80122428 reset. Publication then
sets event phase6. Preflight makes missing camera/pool/header/profile/capacity
conditions atomic. The constructor has no hot-loop allocation and avoids a
copy of the26332-byte host context. ARM size/stack measurements belong to the
native build report.

## Evidence and tests

`source_event_phase5_probe.py` executes unchanged original instructions with
authored retained pools, spawn records, EBD directory entries, bitmap occupancy
and source input words. It does not patch an instruction or hook a helper.
Accepted `phase5-source-proof.json` records:

- 24 full constructor/caller cases, 125346 instructions, maximum 6494 per case.
- 4 cold pool-reset cases, 42204 instructions.
- 24 player-position camera prefix cases, 1968 instructions.
- Zero instruction patches and helper hooks.

The two fixtures under `tests/fixtures/source-event-phase5*.tsv` contain authored
input-derived results, not original textures, geometry, scripts or records.
`test_source_event_phase5` compares every event/camera/pool byte and bitmap,
including retained nonzero lanes. It also tests16 malformed package domains,
12 rollback cases, same-pipeline phase5->6, explicit phase6 hold and4096
allocation-free held steps. Standalone `c++ -std=c++17 -Wall -Wextra -Werror`
compilation and the test passed. Root's cumulative host/native results are
reported separately.

Nine dedicated cooker unit cases additionally pass: authored record/CRC roundtrip,
first-match lookup order, executable and overlay/EBD rejection, malformed
record/count/resource cases and refusal to overwrite an existing output.

The owner-local package is `source_event_phase5.l3p5`, 620 bytes. Its96-byte
header covers both supported overlay/EBD identities, count16, record size32,
metadata size12, total size and payload CRC. The cooker independently validates
original first-match EBD resource identities. The runtime loader validates
profile identities/domains but does not independently hash the currently loaded
EBD payload; root must restrict native admission to the explicit matching fresh
area0 binding. Before these identity tokens drive rendering in broader areas,
cross-package payload identity validation remains required.

## Next exact boundary

Phase6 starts8010102C. Its optional raw80122424==1 branch adjusts800BC744 by
plus/minus0x20000 using source scratch-pad input1F800002 bit02. Its callback
reads event+08 and table(event+A4)+04. The first callback is80100738. That
callback checks event+10 against0,425 and465 and writes original actor fields
and raw80122428/2C/30/34/38 words. The subsequent source handler calls
8001EC0C and8001F070, then checks busy80098A72, callback state and predicate
8001DE84 before another fade operation. Those callback/actor-command/message
operations remain explicit holds. No elapsed-time scene completion, command
acknowledgment, natural tutorial transition or dialogue is fabricated.

Original instruction listings and retail sidecar outputs remain only in private
work. Source archives include authored implementation, probes and scalar/hash
evidence. They exclude the original owner files and cooked payloads.

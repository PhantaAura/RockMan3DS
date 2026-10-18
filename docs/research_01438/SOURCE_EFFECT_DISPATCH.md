# Original lower family80 dispatcher and scalar control package

Native .38 adds an explicitly owned neutral L3E2 profile and the original ascending lower-family80 sweep800172F4. It connects the separately proved scalar callbacks2/3/4 without embedding original color/control tables in source. Other active lower callbacks remain visible holds. This does not complete World Comes Alive or prove retail chronology.

## Source order and gates

The original world caller executes class40 at80015DEC→80017094, lower family80 at**80015E04→800172F4**, then E0 at80015E0C→80017528. Builtin effects≥128 run later at80015E2C→800173B4, after the Buster pass. Root must preserve those original boundaries.

The lower sweep visits all64raw records at800A3B40,72bytes each, in ascending order. It checks raw00/raw06 and invokes only subtype02<128 through scratch1F800064. It adds no source allocation-bitmap eligibility test. A child allocated in a later slot can update in the same sweep; one placed in an already visited earlier slot waits. The original overlay's four instructions80100354..80100360 store table8011A604 into that scratch owner. A default native lease provides no equivalent ownership.

## Neutral L3E2v1

All multibyte fields are little endian; the package is exactly384bytes.

| Bytes | Field |
|---|---|
|0..3|L3E2 magic|
|4..7|u16 version1/headerbytes160|
|8..23|u32 scope1/tablecount16/colorcount8/filebytes384|
|24..31|CRC32 over32..383, then reserved zero word|
|32..127|EXE,overlay,EBD SHA256 digests,32bytes each|
|128..159|u32 identities:dispatch table8011A604;color8011CD74;sequence3 8011CDA8;rows4 8011CDCC;scratch1F800064;dispatcher800172F4;stride72;capacity64|
|160..223|16 source callback identity words|
|224..255|8 source scalar color words|
|256..277|11 source subtype3 sequence halfwords, including original FFFF terminator|
|278..279|zero alignment bytes|
|280..299|5 four-byte subtype4 control rows, including original zero termination row|
|300..383|zero reserved tail|

The loader checks supported source digests, every metadata field, exact framing, CRC, reserved/alignment bytes, and **SHA256 of bytes160..299** against6015208c30d7c82fa3ce30147d74697b65842be15a220278178565aea2295ee3. Mutating color/table/control words and recomputing CRC still fails. The source contains only this derived digest and callback/source identities. The owner-local cooker retains the actual scalar words in local installs, which source releases exclude. It does not borrow an unrelated palette.

## Ownership and publication

`SourceEffectDispatchLease::bind_overlay_load_80100354()` is the explicit source load lifecycle. It requires an admitted L3E2 profile, matching admitted P5 resource binding, the actual immutable EBD storage owner, and nonzero current scene owner/generation. It binds exact profile/P5/binding identities and a weak payload lifetime. Rebind failure clears a previous lease. Profile/binding replacement, EBD reload/unload/directory change, scene change and generation mismatch hold before the sweep. Root is responsible for invoking this only at the actual native overlay load and invalidating it with area/resource lifecycle.

The sweep passes guarded color/control receipts to the leaf consumers. External input providers use fixed function pointers and borrowed finite receipt storage. Completion receives each fully published callback or a held result; this module never advances a generator, cursor or seed. Callbacks publish individually. A later held or unsupported slot does not roll back earlier pool changes after a provider has committed its matching receipt. Unsupported/unknown slots are reported in a bounded64-entry hold array. Continuing other ready slots after a hold is native diagnostic scheduling and remains CANDIDATE for complete original phase chronology.

## Verification

`dispatch-caller-proof.json` records24 unchanged call boundaries,6 original overlay table-store cases,90 scalar dispatcher cases, and71,680 original instructions. There are no helper hooks or instruction patches. Dispatch cases cover first/last/cross-bank slots, signed flag bytes, false gates, known lower callback targets, and high subtypes skipped by this sweep. Deliberately zero allocation masks still permit the original raw-gated callback. Callbacks stop at their real entry; their bodies have independent subtype2/3/4 proofs.

Public host checks reject384 malformed/truncated framing cases and unowned execution. Optional owner-local arguments additionally verify the actual package/EBD and384 independent one-byte mutations with recomputed CRC; profile/binding/scene/payload lifetime guards; both child visitation orders; unsupported callback reporting; subtype3/4 control/lifetime delivery; preservation of earlier publication during a later RNG hold; finite provider ordering; and zero C++ heap allocations during every exercised hot sweep. No source fixture includes original scalar color words.

Standalone host and devkitARM GCC16.1 ARMv6K compilation pass with warnings as errors. A misleading-indentation accessor was braced before the passing ARM compile; no warning was suppressed. Reported per-function static stack is672bytes for load,40 for bind,32 for lease validation,704 for dispatch. These are not nested-call RAM/stack peaks or physical Old3DS measurements. Root reports the actual linked native build and emulator/hardware results separately.

Test commands: `test_source_effect_dispatch` for public guards; `test_source_effect_dispatch /private/source_effect_dispatch.l3e2 /private/source_event_phase5.l3p5 /private/owner.ebd` for owner-local positive cases. Reproduce original boundary proof with `python3 -B docs/research_01438/source_effect_dispatch_probe.py --exe /private/ROCK_NEO.EXE --overlay /private/owner-overlay.bin --out /private/new-proof-directory`.

Remaining owners include an actual runtime RNG provider, complete E0/other scalar sibling behavior, cooperative cadence, natural scene handoff/tutorial progression, and physical Old3DS validation. PHYSICS_1TO1 and retail parity remain false.

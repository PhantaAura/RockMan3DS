# Sparse original combat-profile metadata

Status: **SOURCE_BACKED_PARTIAL**. New L3CB connects the proved fresh class03
record fields to a bounded native parser. Owner-derived values stay in local
sidecars; the source release includes only code and authored test values.

Original selector80064718 selects the first table when raw800C1B6B is not2,
and the second when it is2. The current sidecar retains **exact raw selectors1
and2 only**, with source variants0/1, class20/subtype03, index0C0/context0F0.
Missing selector/identity/context is unknown and lookup returns null. Even
though other raw selectors select the same original table, this package does
not silently alias those contexts to the proved fresh profile.

For those identities, the original subtype03 pointer slots are8008EDD0 and
8008F06C and select records8008F35C/8008F6BC. The original fresh initializer
reads unsigned byte08 for HP (scaled by16) and unsigned byte0F for detector
profile. Original action3 phase0 reads unsigned byte0A into work330. These
three fields are retained; no broader combat-record semantics are invented.
The stats selector's fresh lifecycle contract is documented separately in
`STATS_SELECTOR_LIFECYCLE.md`. Restore of unknown persistent state invalidates
the fresh selector; area loading does not choose it.

## Native ownership and canonical format

`SourceCombatProfiles` owns a fixed16-entry array and count. Parsing and lookup
allocate no memory. `load_l3source_combat_profiles` checks exact magic/version,
flags, sizes, entry width/count, CRC, zero reserved fields, proved identity and
strict ordering. Duplicate identities, contradictory source variant, unknown
contexts and malformed/trailing payloads reject. The serializer performs the
same identity/order checks. A sparse missing row is never replaced with an
unrelated row.

`find_source_combat_profile(profiles,selector,class,subtype,index0C,context0F)`
returns a pointer into the caller-owned bank. Its lifetime ends when that bank
is destroyed or replaced. Accessors expose `initial_hp()`, `action3_work330()`
and `detection_profile()`; none chooses session state or executes combat.

L3CB uses a32-byte little-endian header:

|Offset|Field|
|---:|---|
|0|Magic L3CB|
|4|Version u16=1|
|6|Header bytes u16=32|
|8|Contract flags u32=1|
|12|Sparse count u32,1..16|
|16|Entry bytes u32=12|
|20|Exact file bytes u32|
|24|Payload CRC32 u32|
|28|Reserved u32=0|

Each12-byte row stores source class, subtype, raw selector, source variant,
record index0C, context high0F, original unsigned bytes08/A/F, then three zero
reserved bytes. Identity order uses the first six bytes. The current cooked
two-row file is56 bytes; no original combat table is embedded wholesale.

## Owner-local cooking

```sh
python3 -B tools/cook_actor_combat.py --exe /path/to/ROCK_NEO.EXE \
  --out /new/owner-local/actor-metadata
```

The cooker verifies the exact supported owner SHA-256, bounds every EXE
read, resolves the proved subtype pointers, validates alignment and refuses
existing/inside-source output directories. All packages are prepared before
creating the exclusive new directory. It produces:

- `actor_combat.l3cb`:56 bytes, two exact sparse identities.
- `actor_detection.l3dp`:36 bytes, the existing proved profile4 layout.
- `actor_normalization.l3nr`:416 bytes, the original384-byte signed16 lookup
  at80092D38 in the existing format.
- `actor_detection_bounds.l3db`:44 bytes, original12-byte ordered bounds
  at80097D0C in the existing format.
- `cook-evidence.json`:selection addresses, identities, sizes and hashes;
  no extracted field values or table bytes.

The output directory is owner-local data and is excluded from source ZIPs.
No existing detector cooker is changed. Compiling runtime source alone does
not require recooking these unchanged sidecars.

The cumulative `build_tutorial_pack.py` now calls the same strict shared
builder with the existing owner `ROCK_NEO.EXE` before the pack's manifest
output inventory. It writes the four sidecars directly under
`legends3ds/tutorial/` in unpublished staging and embeds source identities,
selection addresses, hashes and limits in the existing manifest's
`actor_source_metadata` field. It emits no extra runtime JSON and copies no
executable. The existing exclusive publication workflow remains responsible
for publishing a complete new pack. Unknown owner SHA, unsupported source
record/profile, occupied target or provenance key aborts; no session selector
is chosen by this helper. Authored helper tests verify all four payloads,
embedded provenance, rejection and preservation without recooking retail data.

## Validation and limits

Authored C++ tests check every truncated length, malformed headers/CRC,
reserved fields, contradictory variant/context, duplicate/unordered records,
sparse lookup, unsigned field limits and1,200 allocation-free lookups. Owner
sidecars are additionally loaded/round-tripped separately. Authored Python
tests check all four formats and source-bound metadata reads, unsupported
profiles, invalid bounds, exact owner hash, inside-source output rejection,
invalid-input noncreation and preservation of existing output.

`actor_combat_source_probe.py` executes original80064718 record selection,
original80102410 initialization and unhooked original80102C9C action3 phase0
for both selected records. Two owner cases match the cooked fields; ten
authored unsigned boundary cases cover0,1,127,128,255. The probe executes
14,076 instructions. Initializer floor80048E6C and matrix/pose8003346C are
explicit return-only hooks; action3 phase0 has no helper calls. The original
record loads use unsigned byte operations. An owner-local expected TSV then
feeds `test_source_combat_profile PACKAGE EXPECTED.tsv` to compare the
original measured HP/profile/work330 with native accessor results.

```sh
python3 -B docs/research_01419/actor_combat_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --package /owner-local/actor-metadata/actor_combat.l3cb \
  --out /new/owner-local/combat-field-evidence
```

The evidence and expected TSV are excluded from source releases. This is
field-level source agreement in synthetic memory; it does not establish a
running enemy encounter, later action3 phases or hardware behavior.

This metadata boundary does not establish actual hit delivery, audio, RNG
ownership, complete enemy AI, restored-save profile selection or retail parity.
The root runtime must bind the appropriate optional session selector and
actor identity, then stage behavior/contact/pose transactions before claiming
the fields drive native gameplay.

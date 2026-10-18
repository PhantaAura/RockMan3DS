# Selected original byteC in L3CB v2

Status: **SOURCE_BACKED_PARTIAL**. Original Action4 phase4 expiry selects a
subtype pointer through `80098844`, adds signed actor work32E times16, reads
unsigned byteC and stores it in work35C. No broader gameplay name is assigned
to this field. Source anchors are `80103158` (unsigned subtype), `8010316C`
(signed work32E), `80103180` (unsigned byteC) and `80103190` (work35C store).

The current sparse identity still admits class20/subtype03, raw selector1/2,
record index0C0 and context high0F0 only. It retains the selected base record.
Consequently, **actor variant32E must equal0** to use retained byteC. The legacy
identity field `source_variant` tags the statistics table selection; it is not
actor work32E. Actor variant1 selects the next16-byte record and remains outside
the current package contract. Native consumers must hold that case.

Original instruction execution verifies14 cases and924 instructions: both
owner base records match the transient v2 cooker, ten authored boundary cases
verify unsigned values0/1/127/128/255, and two alternate actor-variant cases
prove the different selected address. Actor phase/timer/context are authored;
this is not a live retail encounter or hardware parity proof. Original inputs
are read only and no owner field values or package payload are exported.

L3CB header/version2 preserves the32-byte header,12-byte sparse rows, ordering,
CRC and bounded16-entry ownership. Row9 is byteC, row10 is explicit presence0/1,
and row11 remains zero. Absence requires row9=0. A present value of0 is distinct
from absence. v1 keeps all three bytes reserved-zero and parses byteC as
`std::nullopt`. `SourceCombatProfiles::format_version` retains the parsed version
so old packages roundtrip exactly. Encoding v1 with a known byteC rejects;
it cannot silently discard evidence. New banks and the owner cooker emitv2.

Authored C++ tests cover v1 compatibility, explicit zero, unsigned maximum,
mixed missing/present v2 rows, malformed presence/reserved bytes, invalid
versions and prohibited evidence loss. Parsing/lookup remains allocation-free.
Python tests verify the cooker copies byteC from the same bounded16-byte record,
retains source identity/provenance and preserves all inherited path/input guards.

```sh
python3 -B docs/research_01421/combat_byte_c_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/owner-local/combat-byte-c-source

python3 -B tools/cook_actor_combat.py --exe /path/to/ROCK_NEO.EXE \
  --out /new/owner-local/actor-metadata-v2

python3 -B tests/test_cook_actor_combat.py
```

The cooker creates a new directory and leaves existing packs untouched. Its
four generated sidecars remain local and are excluded from source releases.
Only `actor_combat.l3cb` changes format; detector/normalization/bounds formats
are unchanged. Package replacement and manifest updates belong to the main
integration workflow, after verifying identities and output hashes.

The existing owner-EBD animation probe independently decodes requests21 and22:
19 and17 reachable original pose records respectively. The combined eight
proved source requests consume47,532 decoded bank bytes, and mesh plus bank
consume50,784 bytes under the512KiB actor cap. These are decoded-memory counts,
not Old3DS runtime frame-time observations or completed enemy attack evidence.

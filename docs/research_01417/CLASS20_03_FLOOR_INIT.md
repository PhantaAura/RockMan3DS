# Class20/03 initializer floor after IDX materialization

Status: **SOURCE_BACKED_PARTIAL**. All three original area1 initializer floor
queries now agree with the original executable when the proven shared-stage
IDX initialization is applied. This does not establish actor wall movement,
combat, BIOS RNG, detection, or live encounter parity.

The static cooked map retains original raw IDX fields. A new temporary host
probe loads that map and its matching L3RG, then derives each of the 994 admitted
ST00 IDX halfwords from the corresponding primary render record's retained
byte2/byte3. Admission is the independently proved owner ST00 HED match; this
is a diagnostic loaded-RAM premise, not a contact-list override or raw pack edit.
The probe and every owner output remain outside the source tree.

After this initializer, selectors94 and123 each have low12 bits1 and choose
their alternate lists. This fixes the two previously unavailable positive
contact bindings:

| Original source position | Important positive candidate | Selected list | Original floor | Native floor after fix |
| --- | --- | --- | --- | --- |
| `(367,-1,-6946)` | IDX94 / descriptor41 | 21 | -1 | -1 |
| `(-300,-1,-5654)` | IDX123 / descriptor32 | 16 | -1 | -1 |
| `(-200,-1,-5654)` | IDX123 / descriptor32 | 16 | -1 | -1 |

Corrected bank selection alone gives supported floor -1 at the last two
positions. The first still reported Unsupported with eight unsupported visits.
Its selected IDX94 list contains groups whose first primitive codes are
0x51/0x41/0x31/0x21. Every such group is outside the original actor query's
conservative X/Z envelope. The native sampler counted them unsupported before
testing whether the query could intersect them. The original classifiers reject
them, so those unknown visits were unrelated to this query.

The narrow sampler fix excludes family2..5 groups outside that envelope only
when all supporting premises are available: symmetric nonnegative X/Z radii,
strictly ordered translated raw extents, and an envelope that does not wrap
signed16 coordinates. The original classifier's shifted rectangle lies inside
this envelope, retains strict edges, and has positive division denominators on
those premises. Classification and interval handling for a group inside the
envelope remain Unsupported. Other families, wrapped coordinates, degenerate
extents, asymmetric bounds and missing contact lists also remain Unsupported.
No diagonal floor or movement response has been manufactured.

The floor core's selected list uses the established low12-bit raw6 predicate
at `80048FCC`; class20 flags still apply the original 0x8000 primitive filter.
Group code0x301 is rejected before family classification by the original
code>=256 gate. Supported rectangle primitives1/3/15 and slope interval
primitives8..11 retain their earlier source-backed kernels. The six initializer
bounds remain `(-60,60,-60,60,-128,0)`, and source coordinates are unchanged.
Successful sampling exposes a validated floor result; the caller owns binding
it to the actor.

`actor_floor_initialization_source_probe.py` loads the canonical owner
MDT/IDX/STG/HED into a fresh synthetic memory image, supplies the verified
materialized halfwords and actual area1 grid context, then executes original
wrapper `80048E6C`, core `80048EA4`, candidate gather, class filter, selectors,
classifiers and interval helpers. It uses **no call hooks**. All three calls
return -1 and preserve their query position, executing 10,952 instructions.
Another 128 authored outside-envelope/edge cases execute the original four
diagonal classifiers with both verified radii 0/60 and codes
0x21/0x25/0x31/0x35/0x41/0x45/0x51/0x55,
and strict edge points. All return false without hooks, executing 10,352
instructions. The complete owner-local evidence SHA-256 is
`cfb0eee626fa8009fb8a30c769cebd2dc7827aeefba130523517200d77d62566`.

The source-only unit test `test_actor_floor_init.cpp` checks the exclusion,
strict edges, query preservation, both known bounds profiles, code>=256 rejection,
and all retained unsupported guards. Focused assertions-enabled Clang compilation
with `-Wall -Wextra -Werror` passed. The separate temporary owner map probe
reports SourceBackedPartial / floor -1 / unsupported0 at all three original
positions. The old cooked map/render files are read-only inputs to that probe.
The existing assertions-enabled actor-resource suite also passed against the
updated sampler, including its prior bounds, packet, pose, hierarchy, shared
lifetime, reload isolation and allocation checks. The source probe syntax check
passed.

The owner outputs are under
`work/pass-20261005-worldalive-01417/enemy-source/`. Reproduction requires a new
output path:

```sh
python3 -B docs/research_01417/actor_floor_initialization_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261005-worldalive-01417/enemy-source/new-floor-initialization.json
```

No original code bytes, instruction listings, table payloads, contact arrays,
new assets or original function tables are distributed with this proof.

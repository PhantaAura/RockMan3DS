# Source-backed detector occupancy extension

The 0.0.14.19 segment helper held every contact group in dispatch families2..7
before asking whether the point was in its footprint. The original80049C54
calls the family's classifier first. A rejected footprint skips the group;
it does not require interpreting that group's Y primitives. That distinction
blocked valid imported detector queries even when the troublesome groups were
far from the detector point.

The extension executes original classifier truthiness for rectangle families0/1,
four diagonal families2..5, and diamond family6. Diagonal predicates retain
original signed16 coordinate stores, their code25/35/45/55 inverted bound branches,
the strict inequalities, source tolerance4, signed32 MULT low word and signed
DIV truncation. Division traps and unknown dispatch families remain held.
Family7's original rectangle rejection is also admitted; its interior remains
unknown. These are source-specific occupancy predicates, rather than triangle
raycasting or approximated collision surfaces.

Once admitted by the X/Z classifier, the original8004A060 helper dispatches on
the primitive code's low byte. Most codes retain the plain Y interval, including
the diagonal and diamond plain intervals. The native extension admits this
unchanged interval and preserves the original flag8000 filter and result
packed-cell/low-four-flag encoding. Every slope-specific Y branch remains held.
It therefore also skips an unknown slope when the proven classifier rejects
its footprint or the original class filter excludes the primitive.

The query result now records the first held resident cell index, group index,
primitive index and code. Each is UINT32_MAX when no held primitive is known.
This costs four scalar words and no heap allocation; native runtime logs can
name the actual gate rather than reporting a generic unsupported query.

## Reproduction

```sh
python3 -B docs/research_01420/actor_detection_segment_extended_source_probe.py \
  --exe /owner/ROCK_NEO.EXE --dat /owner/CDDATA/DAT --out /new/owner-local/proof
```

The source probe verifies the supported USA executable SHA, tutorial overlay
SHA, and the994 source-materialized stage identities. It runs complete original
80049C54, including original classifiers and Y helper, without return-only
replacement hooks. The source checkpoint includes authored scalar fixtures
and evidence hashes; the actual stage fixture with owner coordinates/bounds
stays owner-local.

The new native regression is registered separately from the inherited segment
regression. Its optional private invocation is:

```sh
test_actor_detection_segment_extended \
  docs/research_01420/actor-segment-extended-authored.tsv \
  /owner-local/proof/actor-segment-extended-owner-stage.tsv /owner-local/pack
```

The unchanged inherited fixture remains useful and still exercises unsupported
slopes. Its old scalar unknown-family guard now uses family7 interior, since
family2 code21 has newly proved semantics. Malformed formats and ranges still
fail. The new regression additionally checks first-held diagnostics, exact
outside-family7 rejection, flag-filtered unknown Y, and1536 allocation-free
supported diamond queries.

The complete original/native differential contains11690 authored queries:
11113 exact admitted results/candidate counts,577 explicit holds, and444 nonzero
results. It also compares294 owner-stage queries around the three imported
actor source records: all294 match, none hold, and82 are nonzero. The original
probe executes11133302 instructions. The prior detector's stage sample held
7 of21 queries; the extended supported predicates resolve those holds within
this materialized context. These counts are regression coverage, not a game
completion measure.

This is SOURCE_BACKED_PARTIAL, not live retail encounter parity or physical
Old3DS validation. It does not establish enemy AI, attacks, whole collision
physics, or gameplay correctness by itself. Remaining unknown Y branches and
family7 interior are explicit blockers wherever they are encountered.

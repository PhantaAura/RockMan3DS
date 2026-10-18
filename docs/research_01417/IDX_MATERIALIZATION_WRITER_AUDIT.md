# Original IDX materialization writer

Status: `SOURCE_BACKED_PARTIAL`. The missing static0-to-initial1 writer is now
identified in the hash-gated owner executable. This is a bounded source replay
with the owner ST00 resource quartet, not a newly observed original load trace,
complete retail parity, or Old3DS execution. No retail instructions, tables,
resources, RAM dumps or saves are included in this source directory.

## Actual writer and admission

The original routine `0x8003A3B4` scans2048 IDX identities at `0x8015C000`,
stride12. It skips identities with a zero descriptor word. For each non-null
identity it scans up to512 HED entries at `0x8013A000`, stride8, using each
entry's pointer at+4. The first zero HED pointer terminates that search. It
compares the low24 bits of the HED pointer and IDX descriptor word, while
preserving the full IDX descriptor word for its subsequent descriptor load.

On the first match, it reads descriptor+24, then the unsigned halfword at
that pointed record+2. The original halfword store at `0x8003A434` writes
that value to IDX+6. Unmatched identities retain their original field. The
rule does not force a contact selector or prescribe a constant value1.
A populated identity0 follows the same admission rule; the actual ST00
identity0 has a null descriptor and is skipped.

The loader's stage-resource-change branch queues the shared resources and
waits at `0x8001F2FC`. It invokes this materializer at `0x8001F304`, then
invokes the separately audited variant routine `0x8003AD14` at
`0x8001F30C`. The writer therefore initializes shared stage IDX state,
including records beyond the current area's cell references. It is not an
actor-query-specific collision override.

## Owner ST00 replay

`idx_materialization_source_probe.py` executes original instructions in
`0x8003A3B4..0x8003A460`, retaining branch delay slots. This bound includes
the actual IDX materialization loop and ends before its later helper calls.
The owner quartet contains995 IDX rows and116 HED entries before its zero
terminator. All994 non-null IDX identities match HED entries and derive
initial halfword1 from the actual descriptor primary render header. The
original interpreter executed1,543,539 instructions and performed994
halfword writes, all at `0x8003A434`. Identity0 retains0. Every source XYZ
and descriptor field remains unchanged.

| Measured tutorial identity | HED match slot | Descriptor | Descriptor+24 record | Record+2 halfword |
| --- | --- | --- | --- | --- |
| IDX78 |32 | `0x80194630` | `0x80195BF8` |1 |
| IDX94 |40 | `0x801947B0` | `0x80195DFC` |1 |
| IDX123 |31 | `0x80194600` | `0x80195BB0` |1 |

These resulting fields match the earlier captured live tutorial records.
That agreement is `MATCHES_MEASURED_FIELDS`; the writer PC was found from
original source, not captured by a new live breakpoint in this subtask.
Resource hashes, bounds and aggregate scalar checks are in
`idx-materialization-owner-evidence.json`.

## Why the subsequent variant routine retains tutorial1

The original stage0 entry at `0x80089F04` is signed-1. `0x8003AD14` reads
that entry and exits at its negative-group branch before reaching the
0/100/200 mutation region audited in01416. If raw flag `0x800C1B74` is
nonzero, it first calls `0x8003B5B4`; that helper itself exits for stage0,
with its substantive path gated to stages5/6.

`idx_tutorial_variant_guard_probe.py` replays those original paths for
stage0 and explicit flag premises0,1,256,65535. The three initialized
reference records all retain1. This supports the tutorial loader order
without extending stage5/6 mutation semantics. Its hashes and four scalar
cases are in `idx-tutorial-variant-guard.json`.

## Available cooked metadata and implementation boundary

`SourceDescriptor.render_ptrs[0]` is descriptor+24. `SourceRenderRecord`
and cooked L3RG already retain the record's byte2/byte3, so that neutral
render record can provide the original unsigned seed as
`byte2 | (byte3 << 8)`. L3MP retains the corresponding STG offset but not
these header bytes.

The existing `parse_st00` collects a union of descriptors referenced by HED,
IDX, default negative terrain and HD2. It drops the original HED membership
and its early-zero scan admission. Therefore descriptor existence in the
cooked union alone does not prove admission to this initializer. A generalized
implementation should preserve the exact bounded HED match/derived seed at
cook time or in checked neutral metadata, including explicit unmatched state.
Both runtime and rendering map owners should consume the same initialized
state before their selectors are used. The source probe does not edit any
runtime, renderer, CMake, main file or owner asset.

Later runtime mutations, other stage fixtures and complete save/resume state
remain `UNKNOWN` in this bounded investigation. Hardware remains
`HARDWARE_PENDING`.

## Reproduction

From this source root, choose new output paths to preserve prior evidence:

```sh
python3 -B docs/research_01417/idx_materialization_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out /tmp/new-idx-materialization-source-cases.json
python3 -B docs/research_01417/idx_tutorial_variant_guard_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --out /tmp/new-idx-tutorial-variant-guard.json
```

The materializer also accepts `--skip-synthetic` for the actual owner-only
fixture. All outputs contain hashes, addresses, authored synthetic premises,
observed scalar fields and counts. No original byte arrays or instruction
listings are emitted.

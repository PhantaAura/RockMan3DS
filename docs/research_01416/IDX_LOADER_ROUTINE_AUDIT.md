# Bounded owner-source IDX initialization audit

Status: `SOURCE_BACKED_PARTIAL`. The owner executable is hash-gated to the
inherited USA profile. The inspected routines and existing ST00 overlay are
read-only. No original instructions, resource payloads or function tables are
serialized here.

## Loader route

`0x8001F23C` reads the signed stage/area bytes at `0x800C1B60/61`, then calls
`0x8001F740`. Its stage-resource change branch prepares the shared resource
range beginning at `0x8015C000`, queues the stage and area resource requests,
waits, calls `0x8003A3B4`, then calls `0x8003AD14` at `0x8001F30C`.
This route contains no direct IDX+6 halfword store in its own body.

The stage-indexed indirect call at `0x8001F328` resolves to the already-supplied
ST00 callback `0x80100324` for stage0. That callback installs six actor
dispatcher pointers into scratch storage; its bounded body does not modify IDX.

`0x8001F3C8` handles the subsequent area-resource branch, invokes an area-specific
indirect callback at `0x8001F498`, then initializes several other runtime owners.
For ST00 the callback is `0x8010038C`; its first direct call is `0x80022FD4`,
which reads area metadata and expands camera/area fields. Neither inspected
bounded body directly establishes the measured IDX.raw6=1 state.

The generic CFG helper deliberately keeps unconditional jumps as edges.
Its output is a conservative address index, not complete alias or semantic
proof. The original integer interpreter executes the specific mutation region
below with both branch paths and source delay slots retained.

## A real IDX writer that does not explain raw6=1

`0x8003AD14` constructs the absolute first IDX+6 address `0x8015C006` at
`0x8003AECC`. This explains why searching only stores with immediate offset+6
would miss this writer: its actual halfword stores use base+0.

The tail region `0x8003AEC0..0x8003B10C` walks 12-byte IDX identities in groups
of16, takes two-bit selectors from a separately derived packed state group,
skips identity0, and stops at the first zero descriptor pointer. Its upper
bound is2048 identities. The full routine temporarily establishes a nonzero
identity0 descriptor and clears that sentinel again before returning.

The measured synthetic mutation is:

| Explicit packed selector | IDX.raw6 result | Descriptor result |
| --- | --- | --- |
| 0 | 0 | Retained |
| 1 | 100 | Replaced through a derived lookup bank |
| 2 | 200 | Replaced through a derived lookup bank |
| 3 | Retained | Retained |

The source halfword stores are `0x8003AF78` (0), `0x8003AF88` (100), and
`0x8003B038` (200). Certain raw stage/descriptor premises choose different
lookup banks; their precise scalar premises are covered by the probe without
assigning content names or guessing game semantics.

`idx_variant_source_probe.py` executes this original tail region on1120 bounded
synthetic cases: seven identities around packed-word boundaries, all four
selectors, ordinary/special descriptor premises, four lookup slots and five
raw stage premises. All resulting flag/pointer fields, identity0 preservation,
earlier selector0 rows and termination-sentinel preservation matched. It
executed785352 original instructions. The matching results are in
`idx-variant-source-cases.json`; all lookup values and descriptors in that file
are explicitly synthetic. The full loader's helper-generated lookup values
and packed-group ownership are not claimed reconstructed.

This routine writes0/100/200 and therefore cannot by itself justify replacing
the captured static0 with live1. It must not be used as a blanket alternate-bank
initialization. The earlier reference's three measured records still require
their actual0-to1 writer, timing and domain to be observed.

## Reproduction

From the source root, preserving previous output:

```sh
python3 -B docs/research_01416/idx_variant_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --out /tmp/new-idx-variant-source-cases.json
```

The script refuses another executable profile or an existing output path. It
uses the inherited bounded integer helper, and emits scalar/hash evidence only.
No runtime flags, collision code, 0143 renderer path or owner files were changed.

# Original integer heading helper

Status: **SOURCE_BACKED_PARTIAL**. This implements the arithmetic helpers,
not a complete retail enemy callback or live gameplay parity.

The owner executable is the existing verified USA `ROCK_NEO.EXE`, SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
`80031198` reads signed16 X/Z from the origin and destination position pointers.
It subtracts coordinates as32-bit values, shifts delta-Z left4, and shifts
delta-X left16 **with32-bit wrapping before division**. Division truncates
toward zero. Positive Z uses catan+0x800; negative Z negates the wrapped
numerator before division and returns `(0x1000-catan)&0xFFF`. Zero Z chooses
0x400 for a negative shifted-X register and0xC00 otherwise, including a zero
vector. A generic floating-point atan2 changes these rounding, wrap and
zero-vector results.

The original `8007A414` catan is a12-step integer vector-rotation algorithm.
It starts X=4096, Y=the supplied ratio and angle=0; each signed arithmetic
shift and add/subtract matches the source's32-bit registers. The12 owner
step words at `80092B64` are an explicit input. Distributed source contains
no original table payload. Missing table data remains unsupported for a
non-axis heading query. No native runtime table or fallback atan is invented.

The source helper `source_angle.h/.cpp` has no allocations or floating-point
operations. `load_l3actor_angle(data,size)` loads an optional exactly80-byte
`actor_angle.l3ref` sidecar without heap allocation. Its L3AHv1 header is32
bytes: magic; u16 version1/header32; u32 total80/count12/payload CRC32; and
three zero flags/reserved words. The48-byte payload is12 signed little-endian
words. Wrong size, trailing data, magic, version, header fields, reserved words
or CRC reject the package. Format validation does not alone prove provenance.

`tools/cook_actor_angle.py` verifies the original executable and table hashes
before making this optional sidecar. It refuses to replace earlier outputs
or place the payload under the source tree. The local80-byte output has
SHA-256 `9c574c59a9c02599a00608cdf7b4cb210a15b6f2476e55eb6c98912f4362b2c5`;
it is excluded from the code-only release. Missing files remain optional,
with no manufactured table. Cook/parse ownership is now available; native
gameplay/AI integration is a separate gate.

## Checks

`source_angle_probe.py` executes the original helper instructions with no
call hooks. Four explicitly authored synthetic tables test shifts, branch
signs and accumulator wrapping without distributing original data. The
distributed fixture contains1848 cases; SHA-256 is
`fb2561e339108f37537a02b9d9c38afd2f92ad9ee68a3b59bd245eca9b2ab99f`.
The probe also executes462 original-owner-table cases locally. That fixture
and its evidence remain outside the source tree at
`work/pass-20261005-worldalive/source-angle/`. No table payload is exported
even there. Both comparisons passed. Combined original execution covered
687480 MIPS instructions. These are authored input cases, not live retail
motion traces.

Focused commands from the source root:

```sh
cmake -S . -B ../pass-20261005-worldalive/host-source-angle -DCMAKE_BUILD_TYPE=Release
cmake --build ../pass-20261005-worldalive/host-source-angle --target test_source_angle -j 4
ctest --test-dir ../pass-20261005-worldalive/host-source-angle -R '^source_angle$' --output-on-failure
../pass-20261005-worldalive/host-source-angle/test_source_angle \
  docs/validation_01416/source-angle-synthetic-cases.tsv \
  ../pass-20261005-worldalive/source-angle/source-angle-owner-cases.tsv \
  ../../owner-data/extracted/ROCK_NEO.EXE \
  ../pass-20261005-worldalive/source-angle/actor_angle.l3ref
```

The Release target passed with warnings-as-errors. Additional focused Clang
compilation of both the helper and test used `-Wconversion -Wsign-conversion
-Werror`. Test coverage includes every80-byte corruption offset, all truncated
sizes, trailing data, null input, signed-word roundtrips, imported actor query
agreement with the original heading cases, and missing/range/class/subtype
guards. Five cooker checks rejected empty/arbitrary/modified owner inputs,
overwriting a previous output and writing owner payload under source.

The owner input hash is checked by the fixture generator before original
execution. The standalone test reads only the known table for local comparison;
it is not a general importer or an owner-data authorization check.

## Original callback / movement ownership audit

Class20/subtype03 callback `8010226C` snapshots current animation request at
work+AC to+AD, raw/fixed-point position fields to their previous copies and
heading-related fields before consuming original RNG into+32A. It dispatches
state work+08, then resets the sequence with `800322A8` when the request
changed, or advances it with `80032488` when unchanged. Proxy submission is
conditional on source work flags and+328 bit4. Finally `801023F4` clears
work+33C. The earlier isolated hit consumer retains its packet, but the
outer callback owns the final clear: do not redeliver the same packet on
every frame or clear it before state1 consumes it.

State1 `80102600` determines detection bit+328/0x10 with `8003648C` unless
parameter+03 bit0x20 bypasses it. It consumes hit data only when+328 bit2 is
set, then dispatches action+09. Its selector/restore-action logic and defeat
completion depend on source state, RNG, contact and effect allocator inputs.
These are still separate native integration gates.

Action2 `80102AE0` sets phase1, animation request1 and work+330=0 on entry.
It advances heading by source turn+66, invokes `8002FEA4` with movement
parameter52 and zeros, obtains contact result from `8004B414`, and obtains
floor from `80048E6C`. A floor change magnitude of33 or more restores the
whole previous raw/fixed-point position and switches to action1. Contact
bits0xF00 also switch to action1; saved action+0E is4 when detection bit0x10
is set and2 otherwise. `8002FEA4` derives a fixed-point delta through
`80030058`, adds it to work+1C/+20/+24 and converts through `80030150`.
This is not enough evidence to substitute an unmeasured float movement or
collision model. No action2, attack defaults or full AI was added here.

The useful next native bridge is verified owner angle-table residency, then
original displacement and contact helper recovery, preserving callback
order and failing explicitly on missing source inputs.

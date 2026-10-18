# Source door lift/drop callback

Status: `SOURCE_BACKED_PARTIAL`. This implements the bounded scalar body behind
source callback `80113280` for initialized E0 subtype 3, parameter 2, source
states 1 and 2. It is not complete tutorial progression or retail parity.

The door's lift follows the current class-40 actor's part-7 Y halfword at byte
442, minus the unsigned source offset indexed by actor selector byte 14 and the
actual current sequence frame. It does not use a native clock or an authored
replacement animation. The frame receipt identifies the actual actor, sequence
pointer, overlay generation and byte read by the runtime from immutable EBD.

The leaf handles initialization (`80113194`), lift (`801131D8`), linked-object
handoff (`801132BC`), fixed-point drop (`801132EC`), and terminal no-op
(`8011334C`). Its handoff resolves event field `4C` to an occupied E0 record.
The drop subtracts source fixed-point `1.25` and preserves the source threshold,
wrapping and linked-object stores. The lift branch uses the original signed
comparison, including equality: when old Y is greater than or equal to desired
Y, it requires the original RNG call even if the resulting Y is unchanged.

When the source RNG result selects an effect, the implementation preserves the
family-80 first-free allocation scan, null-allocation branch, subtype-3 child
initialization, and the second RNG-dependent horizontal offset. Typed receipts
bind both calls to the current RNG owner/generation, source generation,
prop/actor identities, sequence/frame, actor bone Y and prior prop position.
The leaf neither generates RNG nor advances a global RNG cursor.

## Publication and limits

`consume_source_prop_lift_80113280` publishes the actual context prop, optional
linked prop, and optional child only after every prerequisite succeeds. A held
call leaves those records and allocation masks unchanged. Its caller must first
admit the current EBD/overlay owner and must not overwrite the result with an
older local prop copy. Known-byte checks and bounded source identities reject
unknown fields, wrong generations, malformed pool records and out-of-range
frames. There are no hot-loop heap allocations.

The runtime currently supplies no complete live RNG receipt provider for this
callback. Initialization, no-change lift and independent drop branches can run;
an RNG-dependent lift remains visibly held. The authored differential proof
does not establish the live outer scheduler, current cutscene chronology,
rendered door motion, or physical Old 3DS behavior. These require integration
and runtime evidence. Do not bypass the held RNG branch to make the door move.

## Owner-local L3PL profile

`tools/cook_source_prop_lift.py` retains three explicit 64-byte indexed windows
from the existing, SHA-verified owner overlay. It does not infer curve lengths
from adjacent pointers, unpack a disc, or include a substitute curve. Its
384-byte output is owner-local data and must be excluded from the source ZIP.

| Offset | Bytes | Meaning |
| --- | ---: | --- |
| 0 | 4 | `L3PL` |
| 4 | 2 | Version 1 |
| 6 | 2 | Header size 160 |
| 8 | 16 | Scope 1, count 3, window bytes 64, total bytes 384 (little endian u32) |
| 24 | 4 | CRC32 of bytes 32 through 383 |
| 28 | 4 | Reserved zero |
| 32 | 96 | Supported EXE, overlay and EBD SHA-256 identities |
| 128 | 32 | Table identity, three window identities, callback, bone offset, parameter, reserved zero |
| 160 | 192 | Three explicit source windows |
| 352 | 32 | Reserved zero |

Metadata is `8011C9FC, 8011C958, 8011C998, 8011C9D8, 80113280, 442, 2, 0`.
The loader also verifies payload SHA-256
`cb7f79c1f39c21a17b69dd55f71a149dd2f7429acd5e833e64fb15d1c2f96a54`;
copying identity metadata and recomputing CRC cannot admit altered curves.
The runtime binds the optional immutable profile to the current EBD witness and
clears it with source/area lifetime changes. Missing data preserves safe boot.

## Evidence and tests

`source_prop_lift_probe.py` executes the unchanged source callbacks, allocator,
RNG wrapper and selected OpenBIOS rand body on authored scalar work and authored
curve inputs. It uses no instruction patches or helper hooks. Only data
pointers/windows are authored. The checked-in fixture contains these authored
inputs and derived scalar outputs; it contains no original graphics, animation
curves or executable bytes.

`source-prop-lift-proof.json` records 202 cases, 13,279 original instructions and
120 selected BIOS calls. Cases cover all implemented sub-states, selector/frame
boundaries, signed Y boundaries, equality/no-change, RNG effect/no-effect,
full/cross-bank/last-slot allocation, drop threshold and integer wraparound.

The source-only CTest command is conceptually:

```sh
test_source_prop_lift tests/fixtures/source_prop_lift_01439/callback-authored.tsv
```

It passes all 202 differential cases and 2,759 atomic/profile guards, including
removing each observed input lane, stale receipts, invalid frames and malformed
profile lengths. Supplying a second, owner-local L3PL path also tests successful
loading and all 384 one-byte mutations with self-recomputed CRC: 3,143 total
guards pass. Allocation instrumentation observes zero allocations in the leaf.

Standalone source compilation passed actual devkitARM ARMv6K
`-Wall -Wextra -Wpedantic -Werror -O2`. Compiler static stack reports: callback
456 bytes, its receipt-check helper 40 bytes, and load-time profile loader
672 bytes. These are compiler frame figures, not measured total call-chain or
hardware stack peaks. Linked native build, emulator captures and hardware
status belong to the checkpoint's integration report.

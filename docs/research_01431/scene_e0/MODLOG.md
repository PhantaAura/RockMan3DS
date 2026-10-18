# E0 scripted actor behavior — 0.0.14.31

Universal Modder reverse-engineering workflow: unchanged owner code/data is the oracle. Original instructions, resource bytes, work snapshots and private fixtures remain outside this source tree.

The subtype3 wrapper at80112B9C calls80112BDC when byte08 is zero, otherwise80113EA4. The latter indexes variant byte03 into8011CAC0. Selected constructor variants0..7 have distinct callbacks; this is not the class20 enemy/pose bank.

Supported branches now update the existing native80-byte work slots. Each call preflights active immutable EBD residency and selected token/variant, copies only the slot, and commits the complete proved branch. Missing owners discard the transaction. No per-tick allocation or resource mutation occurs.

| Variant | Supported after initialization | Explicit holds |
|---|---|---|
| 0 | selector09=0 accumulator/state10, original packed lookup yaw matrix | selectors1..3 camera/GTE owners |
| 1 | idle, selector1 position bob, selector2 fixed accumulator/bob | missing opaque scratch half or invalid selectors |
| 2 | idle identity matrix, selector3 initialization and timer/bob stage2 | camera-linked selector1, linked/global selector2, RNG/child stage1 |
| 3 | idle, selector1 initialization, timer/bob stage3, terminal stage4 | RNG/child stage1, linked global stage2 |
| 4 | idle, selector1 initialization, UV callback prefix when no mutation yet, position stage4, terminal stage5 | UV mutation boundary, RNG/child stage2, linked global stage3 |
| 5 | idle and exact selector1 fixed-point Z update | unsupported selectors/stages |
| 6 | exact no-op | other variants remain unsupported |
| 7 | linked position copy through constructor pointer table | missing table bytes, invalid token, inactive/unknown linked pool slot |

Variant0 reuses existing ReferenceTables and reference.l3ref cooked from exact80092EC4. No extra sine asset or approximation was introduced. The packed4096-word table is distinct from the camera quarter-wave owner. Original helper8007BC68 rotates a freshly reset identity matrix using signed angle and absolute12-bit table index. Arithmetic/halfword stores preserve wrap, not floating point approximations.

Variant7 reads signed byte12 and source pointer table800C4C88. This table already lives in SourceEventWork at+40 after the constructor. The adapter resolves only bounded, occupied source E0 address identities into native pool slots; original virtual addresses are never dereferenced as native pointers.

The halfword at1F800002 is deliberately opaque. Its parity is read by several bob branches. Its producer has not been proved in this scoped investigation, so the native update accepts an optional source-owned value and otherwise holds. Native frame counters and button masks are not substituted.

Validation:184 authored state vectors matched all80 output bytes of unchanged original instructions (12940 instructions, no hooks or patches). A separate exact selected-owner reset/constructor sequence followed100 original E0 sweeps:1500 full80-byte record comparisons passed (168443 update instructions, plus26 stage/6050 constructor). Focused authored tests cover spin/link,12 dependency guards and4096 expired-owner holds, with allocation counters checked.

These proofs establish supported callback field behavior. They do not prove live main/fade/event chronology, full scene camera, draw order, UV resource animation, effect ownership, RNG chronology, combat, physical Old3DS performance or visual parity.

Native API: update_scene_e0_80112b9c(work, resources, active_ebd, inputs). Inputs borrow existing references, SourceEventWork and the source E0 pool, plus optional scratch_half_1f800002. Call through original update lifecycle only after that lifecycle is established; independently gated diagnostic chronology remains CANDIDATE.

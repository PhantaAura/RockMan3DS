# Phase5 family80 effect131 consumer

Status: `SOURCE_BACKED_PARTIAL`. This is a narrow consumer of the existing phase5 pool, without duplicating the pool or replacing subtype 2 behavior.

`tick_source_phase5_effects_800173b4(context)` visits the original 64 records, tests `work00 & work06 & 1`, skips subtype values below 128, and advances only subtype 131. It preflights the whole sweep before publishing any record. An unknown read lane, unsupported scalar state, or active unported callback holds the sweep without mutation. Writes promote only source-written lanes to known; initialization does not invent missing primitive Y or allocation-mask knowledge. The supported sweep performs no heap allocations.

`consume_source_phase5_effect_releases_80031aa4(context)` implements the family80 stores at `80031B84..80031BB0`. Both allocation-mask and deferred-release banks must be known. Cleanup updates the masks atomically and leaves record bytes and known-lane ownership untouched. The explicit cold pool reset now establishes deferred masks as zero; an uninitialized context retains unknown masks.

The original world caller `80015DB4` places the family80 call at `80015E2C`, after `80017154` and before `80038894`. The cooperative dispatch call at `800155EC` returns at `800155F4`, which calls cleanup `80031AA4` before yielding. These call boundaries were executed to their real entry points, without helper hooks. They establish order; opaque earlier and later world helpers and cooperative yielding were not executed. The runtime integrator must supply that source scheduling boundary. Thirteen source callback invocations do not establish thirteen native frames.

The saved proof `effect131-source-proof.json` records 76 original callback cases and 13 sweeps with three separated effects, one unaffected subtype 2, and one inactive 131. The restricted original-instruction probe executed 22,698 instructions with zero code patches and zero helper hooks. The authored lane fixture contains synthetic scalar inputs and exact original scalar results, with SHA-256 `a4aab9919c9d24087c913f2b80df93b1e7308184f34b092a77860950224850d4`; no owner instructions, texture tables, or assets are included.

Validation on 2026-10-13:

- `cmake --build work/pass-20261013-worldalive-01436/effect131-host --target test_source_effects test_source_event_phase5 -j 2`: passed with host warnings as errors.
- `work/pass-20261013-worldalive-01436/effect131-host/test_source_effects work/Legends3DS_0.0.14.36_WorldComesAlive/tests/fixtures/source_effects_01423 work/Legends3DS_0.0.14.36_WorldComesAlive/tests/fixtures/source_effects_01436/callback-known-lanes.tsv`: passed 65 allocator, 1,624 callback131, 120 callback128, 64 release, and 76 phase5 known-lane source cases, plus atomic failure and zero-allocation guards.
- `ctest --test-dir work/pass-20261013-worldalive-01436/effect131-host -R '^source_event_phase5$' --output-on-failure`: passed1/1.
- devkitARM GCC 16.1.0 compiled `src/world/source_effects.cpp` and `src/flow/source_event_phase5.cpp` with `-std=gnu++17 -Wall -Wextra -Wpedantic -Werror -O2 -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -mword-relocations -ffunction-sections -fdata-sections -fstack-usage -Iinclude`: passed. Reported static stack for the family80 sweep is 128 bytes and cleanup 0 bytes. This module check does not claim a linked, exercised 3DS binary.

Remaining boundaries: active subtype128/other callbacks, original complete scheduler, projection/material/draw, live scene observations, and physical Old 3DS validation.

# Class20/subtype03 behavior subset

`class20_03_behavior.cpp` translates the scalar action0 waiting body at
`0x80102884`, action1 turning body at `0x801029B4`, and explicit RNG-sample
selection at `0x80103FAC`. These are included in the shared host/native build.
They are not a replacement enemy controller and are not yet invoked by the
native actor callback. Full AI remains **SOURCE_BACKED_PARTIAL**.

The source action0 first invocation selects stored phase1 or2 from parameter
bit80, requests animation0, stores timer30, clears parameter bit80, and still
falls through the timer decrement to29. Phase1 decrements with signed16 wrap
and sets flags328 bit1 only when the resulting timer is negative. Phase2 sets
flags328 bit20 and enters action3/phase0 when its original distance/hitpacket
or parameter20 predicate condition passes. No action3 semantics are fabricated.

The source action1 requests animation1, obtains the absolute original angle
step divided by8, stores timer8, and immediately decrements it to7. Heading
updates preserve the original4096-unit wrapping and the doubled-limit/sign
rules of helper `0x80031BEC`. The world-position angle helper `0x80031198`
is a separate gate. The API takes its explicit scalar result; it does not
substitute a floating-point atan implementation.

The original distance owner `0x8003606C` writes class20 pool work+80 from the
first player's signed16 raw X/Z. It shifts each signed coordinate difference
right once, squares the shifted values, sums them, then stores four times the
sum. A sum greater than `0x10000000` stores `0xFFFFFFFF`. Native
`class20_03_distance_work80()` preserves odd negative coordinates and this
saturation boundary. Y does not enter this particular field.

Missing distance, predicate, required hitpacket or target-angle inputs return a named status and
leave the state unchanged. Action2 and later bodies return `UnknownAction`.
The RNG selector requires a supplied source sample; no source seeding or
global RNG ordering is inferred.

`class20_03_behavior_source_probe.py` verifies owner source hashes and executes
the original integer instructions. The fixture contains2018 action/selector
cases, each checking10 output fields, and675 distance-store cases. The action1
probe hooks only the world-position angle result; the original turn clamp
executes. Predicate cases supply explicit Boolean premises. Distance cases
run the original store subregion without hooks. These are synthetic source
function comparisons, not live retail replay parity.

The following remain outside this kernel:

- Full callback `0x8010226C` and state1 `0x80102600` admission and ordering.
- Detection helper `0x8003648C`, packet delivery/clearing and HP context.
- Generic animation timeline advancement and matrix/pose consumers.
- Movement, body collision, attack children and complete defeat/despawn.
- Native binding and hardware validation of the resulting enemy controller.

The standalone validation was compiled with AppleClang warnings as errors:

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I include \
  src/world/class20_03_behavior.cpp tests/test_class20_03_behavior.cpp \
  -o /tmp/test_class20_03_behavior
/tmp/test_class20_03_behavior \
  docs/validation_01415/class20-03-action-source-cases.tsv \
  docs/validation_01415/class20-03-distance-source-cases.tsv
```

Both fixture comparisons and non-mutating unknown-input guards passed. The
first guard test mistakenly asked for a missing angle while action1 phase2
is an original no-op. The test was corrected to phase0; the2018 source field
comparisons required no implementation correction.

# Source context layout validation

The first full01431 host run reported114/115 passes and an invalid-free abort
in `runtime_source_context`. This executable injects allocation failures and
checks copy/save/transition owner preservation; those checks were not removed.

An isolated AddressSanitizer/UndefinedBehaviorSanitizer build atO1 passed the
same test with three rejected allocation points. Compiling the test atO3 against
that sanitized core also passed. Freshly compiledO3 test executables linked to
the existing release core passed both with and without the diagnostic Clang
`-fno-assume-sane-operator-new` option. An exact release-options compile also
passed, so there was no evidence justifying an optimization flag change.

A completely fresh isolated Release build (`host-r4`) passed the same test,
again reporting three rejected allocation points. The reproducible difference
was the previously built executable versus fresh objects using the final header
layout after the C0/context additions. The old executable was therefore rejected
as stale ABI evidence. No production logic, assertions, allocator-fault coverage
or CMake flags were changed to obtain the pass. Complete validation uses the
fresh build directory and a frozen production source tree.

Private logs: `source-context-asan-test.log`, `source-context-asan-test-O3.log`,
`source-context-no-assume.log`, `source-context-sane.log`,
`source-context-exact.log`, and the parent host-r3/host-r4 build/test logs, all
under `work/pass-20261011-worldalive-01431`. The source checkpoint includes this
textual account; generated object files and test executables stay outside it.

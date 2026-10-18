# Commands and limits

Baseline: ctest --test-dir ../pass-20261014-worldalive-01439/host --output-on-failure (136/136,6.19s).

Current: python3 -B tools/build_hardware_test.py --pack ../pass-20261014-worldalive-01440/pack-runtime-r1 --out ../pass-20261014-worldalive-01440/native-r3 --host-build ../pass-20261014-worldalive-01440/host

Host native-r3:138/138 passed,13.64s. r1 failed because concurrent generated-build writers raced the static library; builds are now sequential. r2 found two inherited guards still treating phase7 as unsupported; they now use phase8, retaining rejection coverage while phase7 has original-backed consumers. No failures were disabled.

New coverage: source foreign-global257possiblecountvalues and RuntimeSession slot14release/noE0free/unload;6original composite registrations+15sampledpairs;18type10fade+48phase7handoff original cases/514guards; pipeline type10-to-phase7 and missing-display atomic hold. Full Buster firing/encounter remains blocked on genuine global-player pose endpoints, firing owner and other effects.

Native result is recorded separately after completion. No physical01440acceptance or new emulator screenshot is implied by host tests.

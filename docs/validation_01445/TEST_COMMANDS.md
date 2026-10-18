# Exact validation commands — 0.0.14.45

The actual complete build was invoked from `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca`:

```sh
python3 -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive/tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1 --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3 --host-build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host
```

It validated existing cooked inputs and wrapped them in the neutral area
container. It did not run an extractor or recook retail assets. Full host suite:
139/139 PASS in 5.06 seconds. Actual native build: PASS, no GCC warnings in its
retained log. Selected touched native modules compile with warnings as errors.

```sh
cmake -S /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host -DCMAKE_BUILD_TYPE=Release
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/host-configure.log`.

```sh
cmake --build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host --parallel 4
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/host-build.log`.

```sh
ctest --test-dir /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host --output-on-failure
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/host-tests.log`.

```sh
/Library/Developer/CommandLineTools/usr/bin/python3 /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive/tools/package_cooked_area.py --map /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3map --render /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3render --state-id local:verified-tutorial-pair --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/local-data/boot.mmlpkg --negative-materials /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3nm --spawns /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3spawn --map-initialization /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3init --runtime-flow /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3flow --runtime-ebd 0:0:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_00.l3ebd --runtime-ebd 1:1:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_01.l3ebd --runtime-ebd 2:2:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_02.l3ebd --runtime-ebd 3:3:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_03.l3ebd --runtime-ebd 4:4:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_04.l3ebd
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/area-package.log`.

```sh
/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host/l3area_runtime_probe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/local-data/boot.mmlpkg
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/area-runtime-probe.log`.

```sh
/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host/l3area_inspection_probe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/local-data/boot.mmlpkg
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/area-inspection-probe.log`.

```sh
make -C /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/native-source/platform/3ds -j4
```

Result: exit 0; log: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01445/native-r3/logs/native-build.log`.

Owner-local positive dispatcher test (exit 0):

```sh
/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host/test_source_effect_dispatch /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/source_effect_dispatch.l3f2 /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/source_event_phase5.l3p5 /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_00.l3ebd /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/source_rng.l3rg
```

The positive test exercises the actual dispatcher: full effect-4 burst has
25 callbacks, 24 children and 120 RNG calls in one ascending sweep. Explicit
provider precedence, late holds, partial/full pool cases, stale generation and
partial-commit rejection are covered. See owner-dispatch-tests-r3.json and the
focused logs. Existing 65536-call class03 scalar checks remain enabled.

# Exact checks — 01446

Full build wrapper:

```sh
python3 -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive/tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1 --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1 --host-build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host
```

All139 host tests PASS, 5.20 seconds; actual native build PASS with no warnings. Existing package validation/probes reused cooked data, with no extractor or recooker. Rejected prepare/commit restoration remains covered by inherited area_package tests. The affected native lifetime change is verified through the real emulator handoff trace.

```sh
cmake -S /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host -DCMAKE_BUILD_TYPE=Release
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/host-configure.log`.

```sh
cmake --build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host --parallel 4
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/host-build.log`.

```sh
ctest --test-dir /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host --output-on-failure
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/host-tests.log`.

```sh
/Library/Developer/CommandLineTools/usr/bin/python3 /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive/tools/package_cooked_area.py --map /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3map --render /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3render --state-id local:verified-tutorial-pair --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/local-data/boot.mmlpkg --negative-materials /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3nm --spawns /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3spawn --map-initialization /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3init --runtime-flow /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/st00.l3flow --runtime-ebd 0:0:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_00.l3ebd --runtime-ebd 1:1:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_01.l3ebd --runtime-ebd 2:2:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_02.l3ebd --runtime-ebd 3:3:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_03.l3ebd --runtime-ebd 4:4:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1/ebd/area_04.l3ebd
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/area-package.log`.

```sh
/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host/l3area_runtime_probe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/local-data/boot.mmlpkg
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/area-runtime-probe.log`.

```sh
/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host/l3area_inspection_probe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/local-data/boot.mmlpkg
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/area-inspection-probe.log`.

```sh
make -C /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/native-source/platform/3ds -j4
```

Exit 0; log `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261016-worldalive-01446/native-r1/logs/native-build.log`.

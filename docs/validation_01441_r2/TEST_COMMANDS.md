# Exact validation commands

```sh
cmake --build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host -j4
ctest --test-dir /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host --output-on-failure -j4
make -C /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441-r2/native-r1/native-source/platform/3ds -j4
```

Host buildPASS;138/138CTestPASS,18.74s. Native final makePASS with changed main.cpp/hardware_test.cpp under-Werror. Verified279 staged source inputs against working tree. Reused unchanged compiled core cache; no recooking. Intermediate missing-object link failure and repair are recorded in build-result.json and native logs.

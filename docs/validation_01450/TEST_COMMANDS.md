# Exact checks — 0.0.14.50

```sh
cmake -S /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive \
  -B /private/tmp/legends3ds-01450-host -DCMAKE_BUILD_TYPE=Release
cmake --build /private/tmp/legends3ds-01450-host --parallel 2
ctest --test-dir /private/tmp/legends3ds-01450-host --output-on-failure
```

The full result is retained in `host-tests.log`. The focused
`test_child_runtime` output includes:

```text
normal80 spawn -> callback -> segment -> enemy packet -> 8HP -> effect84/release PASS
```

Native build:

```sh
make -C platform/3ds clean
make -C platform/3ds -j2
```

The actual output is retained in `native-build.log`; warning/error scans are
recorded in `build-result.json`.

Source release audit:

```sh
python3 tools/release_source.py \
  /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.50_Source_r1.zip
unzip -t /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.50_Source_r1.zip
```


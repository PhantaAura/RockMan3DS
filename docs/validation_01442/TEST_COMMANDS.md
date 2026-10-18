# Exact commands and results

```sh
python3 -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive/tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1 --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261015-worldalive-01442/native-r1 --host-build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host
cmake --build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host -j4
ctest --test-dir /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host --output-on-failure -j4
make -C /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261015-worldalive-01442/native-r2/native-source/platform/3ds -j4
```

Clean nativepipelinePASS;packvalidation/runtimeprobesPASS;noextractors/recook. Final2changedTUs compiledandlinkedPASS,280stagedsourcesmatch.139/139hosttestsPASS3.17s. Defaultactortransactiontestincludes128originalshadowrows;GPUwaitandretained-live-rootchecks included. Baseline newtargetNotRun orderingfailure and correction recorded inbuild-result.json;noassertionfailure suppressed.

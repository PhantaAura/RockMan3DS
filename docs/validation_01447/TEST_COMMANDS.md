# Exact checks — 0.0.14.47

```sh
python3 -B tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1 --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261017-worldalive-01447/native-r1 --host-build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261017-worldalive-01447/host-r1
```

Exit 0: 139/139 host tests PASS in 5.27 seconds; package validation and runtime/inspection
probes PASS; actual ARMv6K native build PASS. No cooker or extractor was invoked. Exact argv,
return codes and full logs are retained in this directory.

```sh
python3 -B docs/research_01447/source_transition_completion_probe.py --exe /Users/emeraldhunter/Downloads/MML_Discoken/extracted/ROCK_NEO.EXE --out /tmp/source-transition-completion-proof-01447.json
```

Result: 16/16 authored cases PASS, 480 unchanged callback instructions, zero callback patches,
two explicit return-only helper hooks. Evidence: `docs/research_01447/source-transition-completion-proof.json`.

```sh
ctest --test-dir /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261017-worldalive-01447/host-r1 --output-on-failure
```


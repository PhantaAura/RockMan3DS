# Exact checks — 0.0.14.48

```sh
python3 -B tools/build_hardware_test.py \
  --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1 \
  --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261017-worldalive-01448/native-r1 \
  --host-build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261017-worldalive-01448/host-r1
```

Result: exit 0. Host 140/140 PASS in 5.19 seconds; package validation,
area-runtime probe, area-inspection probe and actual ARMv6K native build PASS.
No cooker or extractor was invoked.

```sh
python3 -B docs/research_01448/source_player_buster_select_probe.py \
  --exe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted/ROCK_NEO.EXE \
  --out /tmp/source-player-buster-select-proof-01448.json
```

Result: six authored broad-lane cases PASS; 1,648 unchanged selector
instructions and 12 modeled RTPS operations. Retained evidence:
`docs/research_01448/source-player-buster-select-proof.json`.

```sh
python3 -B tools/cook_source_player_target.py \
  --exe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted/ROCK_NEO.EXE \
  --out /tmp/source_player_target_01448.l3tg
```

Private validation result: `L3TG/1`, 44 bytes, payload CRC32 `ad686123`.
The resulting owner-derived profile is deliberately excluded from source and
app-only packages.

# Exact checks — 0.0.14.49

```sh
cmake --build /private/tmp/l3-host-01449 -j4
ctest --test-dir /private/tmp/l3-host-01449 --output-on-failure
```

Result: 143/143 PASS; final full run 5.94 seconds.

```sh
python3 -B docs/research_01449/source_player_pose_and_fire_probe.py \
  --exe /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted/ROCK_NEO.EXE \
  --decomp /Users/emeraldhunter/ROCK_NEO.EXE.2.c \
  --out /private/tmp/source-player-pose-fire-proof-01449.json
```

Result: PASS. Four direct fire cases execute unchanged original `0x80040E9C`
instructions; 15 pose parts, both muzzle joints, and 16 selection rows are
structurally witnessed. Retained evidence contains hashes and results, not code
or retail table bytes.

```sh
python3 tools/build_tutorial_pack.py \
  /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/owner-data/extracted/CDDATA/DAT \
  /private/tmp/l3-host-01449 /private/tmp/l3-pack-01449-r1 \
  --with-indexed --with-material-cache
```

Result: PASS, 119 local cooked files including `player/buster.l3bp`. This local
owner-derived pack is excluded from all distributable archives.

```sh
python3 -B tools/build_hardware_test.py \
  --pack /private/tmp/l3-pack-01449-r1/legends3ds/tutorial \
  --out /private/tmp/l3-native-01449-r1 \
  --host-build /private/tmp/l3-host-01449
```

Result: PASS. Host suite, package validation, area-runtime probe,
area-inspection probe, and actual ARMv6K native build all pass.

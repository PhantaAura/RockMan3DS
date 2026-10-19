# 0.0.15.10 results

| Check | Result |
|---|---|
| `test_source_gameplay_camera_matrix` (new) | PASS. 640/640 rows match the unchanged `0x80037FB4` code (443,689 instructions, 2,560 MVMVA): `0x800A3A10`, `0x1F800008..24` and `0x800A39F0`. L3RF v1/v2 load. `rsin`/`rcos` branches pass. Lock-on: enemy ahead selected in both lanes; to the side, behind or beyond depth `0x500` not selected; mode byte 0x01 gates the proxy out; pad 0/0xFFFF give the same result; bit 0x10 spawns lane 1 |
| `test_source_player_buster` | PASS, plus L3TG v2 decode (image mode byte, pad rejection) |
| Host suite | 151/151 PASS |
| Area 1 audit `l3edge_audit PACK 1 2822 -1 -4833 200 3000` (owner data, L3RF v2) | 0 escapes, 0 falls, 0 unsupported; camera distance 512..768; 0 unsupported eye tests; 0 packed-trig substitutions; Mega Man projected through `0x1F800008` on 599,800/599,800 frames at screen x 156..161, depth 507..780 |
| `l3door_replay ST00.mmlpkg PACK` | PASS (fall, both doors, terrain patch) |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 967,580 B, SHA-256 `29657e181edf2b0c1973141a0e64f4a39bfa6aad3d3527f6e276752f77f2d987` |
| Azahar | NOT RUN. Use `AZAHAR_TEST.md` |
| Physical Old 3DS | Not run |

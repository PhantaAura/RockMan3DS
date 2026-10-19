# 0.0.15.4 validation results

| Check | Result |
|---|---|
| Host suite (`ctest`) | 147/147 PASS |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 926,216-byte `.3dsx`, SHA-256 `f6c143d67bfca3b5cb49b8c0b9c36f9772b643950be1296a0d578c13880997e2` |
| Azahar 2126.0, Old 3DS model | `BOOT_SPAWN=SOURCE_HANDOFF_80101274`; trace frames 120–240: area 1, (2822,-1,-4833), heading 0, still without input. 0.0.15.3 booted Area 0 inspection instead. |
| Interactive Area 1 collision play in Azahar | Pending (owner test, see AZAHAR_TEST.md) |
| Physical Old 3DS | Not run |

The start-up GPU clear screen waits for a button press before gameplay loads.

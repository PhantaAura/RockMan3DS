# 0.0.15.9 results

| Check | Result |
|---|---|
| `test_source_gameplay_camera` (new) | PASS: init, first-frame snap, turn rate 0x40/0x30/floor 0x18, 0x200 lag clamp, wrap, distance hold / back-out to 0x300 / pull-in after 60 frames, `0x80049C54` free, blocked and no-cell |
| Host suite | 150/150 PASS |
| Area 1 audit 200×3000 (owner data) | 0 escapes, 0 falls, 0 unsupported; camera distance used 512..768; 0 unsupported eye tests |
| `l3door_replay` | PASS (fall regression and both doors) |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 960,864 B, SHA-256 `aeedd7fcceffbff21ec04f07cd87121f551712b5e74445e1158c92181d70c0f8` |
| Azahar | NOT RUN: Azahar was running the owner's other project. 0.0.15.9 is installed on Azahar's SD for the owner. |
| Physical Old 3DS | Not run |

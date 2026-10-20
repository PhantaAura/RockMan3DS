# 0.0.15.11 results

| Check | Result |
|---|---|
| `test_child_runtime` (new segment case) | PASS. State 0 registers endpoint → endpoint. On each of the 10 state-1 frames the work keeps the original +14 → +2C copy, while the recorded segment runs from the owner point to the endpoint and is non-zero. The release happens on the 11th frame. |
| Host suite | 151/151 PASS |
| `l3door_replay ST00.mmlpkg PACK` | PASS: fall, both doors and terrain patch as before. New phase 4 at the Azahar spawn (−334,−1,−5750), heading 0: 3 presses → 3 admitted → 3 spawns through `0x80034EB8` → 3 releases. 33 state-1 frames: 3 zero-length (the state-0 frames), 30 drawn, max length 2207. 0 live afterwards. |
| Area 1 audit `l3edge_audit PACK 1 2822 -1 -4833 200 3000` | 0 escapes, 0 falls, 0 unsupported; camera 512..768; 0 packed-trig substitutions; player projected on 599,800/599,800 frames, unchanged |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; pipeline host 151/151; 968,092 B, SHA-256 `a5849ae81d07306a0fd188db4ca7b6906922a3acad6024a5e11c90aa100dba99` (`pass-20261019-203149-m2-01511`) |
| Azahar 2126.0, Old 3DS (Claude, Oct 19) | Title `0.0.15.11`, door-room diagnostic spawn `1 -334 -1 -5750 0`. **Trace PASS:** 5 Y presses → 5 × `source_buster_runtime,fire_edge=1,spawn_status=0,...,live=1`, 110 `source_buster_draw` lines, 5 releases. **Visual PARTIAL:** a short yellow sliver appears between Mega Man's feet on the frame after a press, then disappears. The follow camera sits behind him, so the straight-ahead ray is seen almost end-on and mostly hidden by his body. Lock-on: status 0, 1–3 proxies projected, no target (`broad`/`window` −1; the Reaverbots are beside him, not ahead). |
| Physical Old 3DS | Not run |

Owner-local logs stay in the Azahar SD folder; they are not committed.

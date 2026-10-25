# Old 3DS hardware checklist (WCA build)

Status: **PENDING** (no physical test in this checkpoint). Baseline device:
Old 3DS XL. Azahar results are preliminary evidence only.

## Build and install
1. `tools/m1_mac_build.sh` (needs devkitARM and the owner's disc; set `CUE=`
   if it is not the default path). The script prints the `sd_out` folder.
2. Copy `sd_out/3ds/Legends3DS/` to `sdmc:/3ds/Legends3DS/` on the card,
   replacing the old folder. Keep `sdmc:/3ds/dspfirm.cdc` (DSP1 dump) for sound.
3. Make sure **no** `diagnostic_autoplay.txt` or `diagnostic_spawn_01414.txt`
   is in the folder: the run must be a normal boot.
4. Start `Legends3DS.3dsx` from the Homebrew Launcher.

The game always writes two small logs next to the `.3dsx`:
`boot_01516.log` (boot stages, memory) and `hardware_01516_<id>.csv` (one
row every 120 frames: position, frame interval, update/render time, draw
calls, triangles, linear/texture memory, plus `# story_flag` lines for every
tutorial flag change). They are the evidence to send back.

## Checks (tick each, note the frame or time when something fails)
| # | Check | Pass when |
|---|---|---|
| 1 | Boot and display | Both screens show; top 3D view, bottom map/tabs; no crash |
| 2 | Controls | Circle Pad moves (tank), A action/talk, Y Buster, L/R as mapped |
| 3 | Tutorial entry | Roll's radio call opens; answering works; control returns |
| 4 | Corridor | Walk to the trap room without falling out or getting stuck |
| 5 | Dialogue | Every message opens and closes; voice/typing blip plays |
| 6 | Trap room | Switch drops the gate; Reaverbots wake, fight, can be defeated; gate reopens |
| 7 | Buster | Shots fire every press, hit enemies, lock-on picks a target |
| 8 | Doors | Area 1 doors open and load Area 2 |
| 9 | Area 2 | Targets break, doors release, box breaks (optional), exit message |
| 10 | Completion | Final message at the far door; `# story_flag ... flag=0x622,value=1` in the CSV |
| 11 | Audio and music | Effects, voice and area music play without dropouts |
| 12 | Frame pacing | Note the `interval_us` column in the corridor, trap room and Area 2 |
| 13 | Memory | No crash after 20+ minutes; `linear_free` does not keep falling |
| 14 | Save | Not implemented in this build (record as N/A) |

## Report back
`boot_01516.log`, the newest `hardware_01516_*.csv`, and the table above with
pass/fail and notes. Do not mark hardware validation complete without them.

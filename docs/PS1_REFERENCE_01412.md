# PS1 reference evidence — 0.0.14.12

The owner-provided USA PS1 BIN/CUE was run in installed PCSX-Redux `f78f8ef8` (dev build 290, changeset `f78f8ef80d91598983739bb8c4e713c953ca2711`). The emulator identified disc label `MEGAMAN_LEGENDS`, ID `SLUS00603`, executable `SLUS_006.03;1`. This is actual original-game emulation evidence; retail-hardware parity and complete BIOS compatibility remain unverified.

## Verified opening tutorial state

The session was left **paused at the initial playable corridor, before first enemy contact**, using the interpreter and bundled OpenBIOS. Original owner saves were not loaded or changed. This note adds numerical measurements and owner-local references; it copies no retail image, screenshot, save state, code dump, or asset payload into the source tree.

Owner-local evidence:

- [Primary original capture: `tutorial_reference_idle.png`](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/ps1-reference/captures/tutorial_reference_idle.png)
- [Matching bounded live measurements](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/ps1-reference/camera_measurements/tutorial_reference_idle.json)
- [Status and limitations](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/ps1-reference/reference_status.json), [source/capture SHA-256 manifest](/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/ps1-reference/reference_manifest.json)

| Measured field | Initial playable idle value |
| --- | --- |
| Rendered raster | 320×240 |
| GTE H / screen center | 384 / (160,120) |
| GTE OFX / OFY, raw 16.16 | 10485760 / 7864320 |
| Estimated player bounds, exclusive xyxy | [127,118,189,233], height 115±2 pixels; excludes shadow/HUD |
| Camera distance / yaw / pitch | 512 / 2048 / 48, angles in 4096 units per turn |
| Actor position | [2822,-1,-4833] |
| Camera target / eye | [2822,-151,-4833] / [2822,-188,-4322] |
| GP / scale scalar at GP+0x498 | 0x80097864 / 512 |
| Global shift / actor yaw / extra yaw | 0 / 0 / 0 |
| Player root matrix, signed Q12 cells | [512,0,0,0,510,36,0,-37,510] |

H and center remained unchanged over 188 idle/forward and 115 further movement/turn vsync samples. These are current/last-draw GTE values, not proof that every primitive shares one setting. With vertical pixels mapped 1:1, H implies vertical FOV `2*atan(120/384)=34.708049°`; projection alone does not determine camera distance. See [camera/player source analysis](CAMERA_PLAYER_TRANSFORM_AUDIT_01412.md) for semantic grounding and implementation limits.

Other valid settled captures are `tutorial_stable_idle.png`, `tutorial_turn_settled.png`, and `tutorial_forward_settled.png` in the same owner-local capture directory. The labels `tutorial_stable_forward.png` and `tutorial_stable_turn.png` were captured during active movement/camera transition and are unsuitable as settled scale targets. **Exclude `first_encounter_idle.png`:** the idle player had been defeated by the first enemy and returned to title/attract before that capture. Intermediate `tutorial_restore_*`, `tutorial_restored_*`, and `tutorial_corridor_reference` captures include opening cinematic/loading states. No first encounter was completed or story progression beyond it performed.

## Exact owner-local CLI reproduction

The helper and paths below are owner-local research outputs, not redistributable source dependencies. Launch from the reference directory; the portable session isolates settings and newly created memory cards.

```sh
REF_ROOT='/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-visual/ps1-reference'
cd "$REF_ROOT" || exit 1
'/Applications/PCSX-Redux.app/Contents/MacOS/PCSX-Redux' \
  -portable "$REF_ROOT/session" -interpreter \
  -bios '/Applications/PCSX-Redux.app/Contents/Resources/share/pcsx-redux/resources/openbios.bin' \
  -iso '/Users/emeraldhunter/Downloads/MML_Discoken/Mega Man Legends (USA).cue' \
  -run -webserver -webserver-port 18080 -no-gdb -no-pcdrv -noupdate -no-viewports \
  -stdout -lua_stdout -dofile "$REF_ROOT/reference_capture.lua" \
  -logfile "$REF_ROOT/session/emulator-interpreter.log"
```

The local helper provides pad controls, rendered-display capture, and explicitly bounded scalar reads. Execution pause/status uses the [documented localhost API](https://pcsx-redux.consoledev.net/web_server/); [CLI/portable options](https://pcsx-redux.consoledev.net/cli_flags/) and [Lua screenshot/pad API](https://pcsx-redux.consoledev.net/Lua/redux-basics/) describe reproduction controls.

## Remaining limits

- Bundled OpenBIOS was detected as `0b0359a7`; no retail PS1 BIOS was found in the inspected owner paths. No BIOS or game downloads were performed.
- The ARM64 dynamic recompiler crashed with SIGILL before gameplay. The interpreter reached the tutorial and reported approximately 30 guest FPS.
- Direct CHD launch was misread as a 20:31:63 raw single-track image by this build. The existing BIN/CUE correctly identified data and audio tracks; use it for this reference.
- The original HP/gauge HUD is visibly captured. Its precise original GPU draw/page/CLUT/resource binding is **not verified**, so this evidence does not authorize a claimed exact replacement HUD mapping. CUA could not bind this app by name/path; the bundle lacks `CFBundleIdentifier`, leaving GPU logger UI tracing unavailable through that binding.

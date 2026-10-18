# Current native test handoff

Local-only SD root: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0147_LocalHardwareTest_20261001/sd_out`. Actual compiled binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001/validated-0147-final/build/Legends3DS_0147_AreaRuntime.3dsx`. Size403236 bytes; SHA-256`82328aa1b9934ca21335712b020f7f81cce1f2fabac8566b4786f29e84fdd34d`. Toolchain devkitARM r68-1/GCC16.1.0, libctru2.7.0, Citro3D1.7.1; ARMv6K/mpcore.

The native frontend tries `data/legends3ds/areas/boot.mmlpkg`; package geometry/contact data reaches the existing session/renderer. Generic unsupported sections remain unbound. Existing tutorial sidecars are admitted only after byte equality of the map/render pair. Missing/invalid/empty/over-budget optional packages use loose boot.

A starts selected rendering; X at the initial screen selects0143 fallback. Circle Pad/D-pad UP forward; left/right/L/R turn; Y fire; SELECT diagnostic area cycle; START exit. Normal retail progression, complete collision/enemies/events/audio are still absent.

When tested later, record PACKAGE/LOOSE HUD, player/terrain visibility, movement/turn/shot/area transition, and any error. Preserve original photos/captures and boot_0147.log/hardware_0147_*.csv with device/build labels. No current screenshots or hardware result exist.

Azahar2126.0 Old-model mode was used for earlier6d tests, not this build. Its direct3DSX loader uses96MiB Dev1 layout; see OLD_MODEL_MEMORY.md. It cannot prove standard Old3DS64MiB compatibility or physical pacing. Mac locked prevented current automated UI testing. No emulator configuration edits occurred in this pass.

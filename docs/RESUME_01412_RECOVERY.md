# Resume 01412 visual pass — in progress

Project: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.12_VisualParity
No Git branch/commit. Parent 01411 checkpoint is intact.
Inherited 20/20 tests passed. First integrated host revision passes 22/22.
Added mockup-based native bottom HUD, touch-tab/map sampling, shared candidate
view settings/projection/config parser. Player in-place pose optimization API
and its tests are being integrated. Native build and emulator capture PENDING.

Commands: cmake -S . -B ../pass-20261003-visual/host-current
cmake --build ../pass-20261003-visual/host-current --parallel 4
ctest --test-dir ../pass-20261003-visual/host-current --output-on-failure
Native: python3 tools/build_hardware_test.py --pack <01411 SD tutorial path> --out <fresh pass/install path>.

PCSX-Redux reference setup in ../pass-20261003-visual/ps1-reference; supplied
BIN/CUE with bundled OpenBIOS loads SLUS00603; ARM64 dynarec crashes SIGILL,
interpreter fallback being tested. Original user saves untouched. Private
reference images stay outside source ZIP.

Next: complete player scratch integration, compile native with actual toolchain,
exercise mockup touch tabs and shared camera in Old-model Azahar, capture retail
reference if PS1 interpreter runs. No FOV/model/map-size retail match accepted,
triangle shots remain provisional; object presentation chain unresolved.
No physical Old3DS tested. No reset used. Usage window refreshed naturally.

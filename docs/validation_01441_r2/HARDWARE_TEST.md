# 01441r2 hardware draw test

User report for prior01441 on Old3DSXL: RGB bars and cyan clear both appear. After X the map/status appears, top remains cyan. Marker motion is inconclusive. Physical game rendering and GPU completion are NOT proved.

Unzip the new SD archive onto the SD root. Launch **Legends3DS 01441r2 Draw Test** (separate entry). Keep existing owner-local data at `/3ds/Legends3DS/data/`. No assets are included in this archive.

1. RGB bars: press and release A.
2. Cyan: press and release **X** to test both terrain and player using0143.
3. If top stays cyan, relaunch; A at bars, **D-pad LEFT** at cyan tests terrain only.
4. Relaunch again; A at bars, **D-pad RIGHT** at cyan tests player only.

Report what appears for X/LEFT/RIGHT. The isolated modes deliberately omit other scene draws and overlays; they are diagnostics, not gameplay completion.

If all stay cyan, preserve `/3ds/Legends3DS/boot_01441r2.log` after a failed run. The last queued/waiting/synchronized labels distinguish reaching CPU submission from returning from the next GPU sync. The file is overwritten on launch, so copy it before rerunning. Hardware CSV files retain unique names.

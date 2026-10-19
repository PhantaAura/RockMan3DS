# 0.0.15.2 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar use Old 3DS mode, 100% CPU and
the printed SD folder, then load the printed `.3dsx`.

1. Confirm the title reports `0.0.15.2` and normal Area 1 gameplay appears.
2. Move along diagonal walls and their straight-wall corners; movement should
   slide along the surface without freezing or leaving the map.
3. Jump and land beside a diagonal wall; the player must remain contained.
4. Fire at the imported enemy and confirm the source combat path still runs.

Retain `boot_01502.log` and the newest `hardware_01502_*.csv`. Emulator success
does not establish physical Old 3DS performance or full collision parity.

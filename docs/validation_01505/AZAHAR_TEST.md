# 0.0.15.5 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar use Old 3DS mode, 100% CPU and
the printed SD folder, then load the printed `.3dsx`.

1. Confirm the title reports `0.0.15.5` and normal Area 1 gameplay appears.
2. Run diagonally into straight-wall corners and box corners; Mega Man should
   slide along the free axis rather than stopping dead.
3. Repeat the 0.0.15.2 checks: diagonal walls slide, jumping/landing beside
   walls stays contained, and the map edge holds.
4. Replay the Oct 19 route: from the start walk to the first door (around
   x -256, z -5376) while turning left. Mega Man must stay on the floor and the
   closed door must stop him; he must not sink or fall out of the world.
5. Fire at the imported enemy and confirm the source combat path still runs.

Retain `boot_01505.log` and the newest `hardware_01505_*.csv`. Emulator success
does not establish physical Old 3DS performance or full collision parity.

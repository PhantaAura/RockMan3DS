# 0.0.15.8 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar use Old 3DS mode, 100% CPU and
the printed SD folder, then load the printed `.3dsx`.

1. Confirm the title reports `0.0.15.8` and normal Area 1 gameplay appears.
2. Walk to the first door (around x −256, z −5376), face it and press **A**.
3. Expected: the same door sequence as 0.0.15.7 (turn, short hold, walk in,
   load, Area 2 door, walk in, control returns). The door still **looks
   closed**: the open and closed door descriptors draw the same geometry
   (see `docs/research_01508/DOOR_TERRAIN_DESCRIPTORS.md`). Report any change
   in the door picture, flicker, missing or doubled terrain near the door, or
   a hitch when the door opens or closes.
4. Walk around Area 2 and back through its door to Area 1; the floor and the
   walls around both doors must look as before.

Retain `boot_01508.log` and the newest `hardware_01508_*.csv` (look for
`IDX swap patch hide:1 … overlay_idx:1` when a door opens and `restore:1` when
it closes). Emulator success does not establish physical Old 3DS performance.

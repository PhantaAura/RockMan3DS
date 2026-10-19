# 0.0.15.7 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar use Old 3DS mode, 100% CPU and
the printed SD folder, then load the printed `.3dsx`.

1. Confirm the title reports `0.0.15.7` and normal Area 1 gameplay appears.
2. Walk to the first door (around x −256, z −5376) and face it. Press **A**
   (the original Circle).
3. Expected: input stops responding; Mega Man turns to face the door, stands
   still for about half a second (the door opening; the door leaf itself is
   not drawn moving and the slab stays visible), then walks forward on his own
   through the door for about 18 frames and stops.
4. About 16 frames later the game switches to Area 2. He appears at about
   (−256, −1, 6208) facing +z, stands still for about half a second, then walks
   forward on his own through the Area 2 door and stops near z 7100. Control
   then returns. Note: he passes through the visible closed door slab, because
   the 3DS terrain is not yet redrawn when the door opens.
5. After control returns, walk around Area 2 briefly: he must stay on the floor.
   Walk back to the Area 2 door, face it and press A: it should take him back
   to Area 1 the same way.
6. Note anything else (stuck before the door, falls, camera under the floor,
   wrong area, input not returning).

Retain `boot_01507.log` and the newest `hardware_01507_*.csv` (look for
`door_transition_before/after`). Emulator success does not establish physical
Old 3DS performance or full collision parity.

# 0.0.15.6 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar use Old 3DS mode, 100% CPU and
the printed SD folder, then load the printed `.3dsx`.

1. Confirm the title reports `0.0.15.6` and normal Area 1 gameplay appears.
2. Walk to the first door (around x −256, z −5376). Mega Man must stay on the
   floor and the closed door must stop him (0.0.15.5 regression).
3. Face the door (within about 45° either way) and press **A** (mapped to the
   original Circle). The game must switch to Area 2 with Mega Man at about
   (−256, −1, 6208), facing +z. No door animation, walk-in or fade plays yet.
4. Expected limitation: he is in the small space in front of the closed Area 2
   door. Walking forward stops at the door; pressing A there returns to Area 1
   at about (−256, −1, −5184). Note anything else (falls, camera under the
   floor, wrong area).
5. Repeat the 0.0.15.5 corner/diagonal-wall checks briefly.

Retain `boot_01506.log` and the newest `hardware_01506_*.csv` (look for
`door_transition_before/after`). Emulator success does not establish physical
Old 3DS performance or full collision parity.

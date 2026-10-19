# 0.0.15.1 Azahar test card (Old 3DS model)

Setup: build with `tools/m1_mac_build.sh` (prints the exact paths). In Azahar set
New 3DS mode OFF, custom SDMC = the printed `sd_out` folder, load the printed
`Legends3DS.3dsx`.

Controls: Circle Pad move · B jump · Y fire Buster · X lock-on (see note) · START exit.

| # | Step | Expected | Result |
|---|------|----------|--------|
| 1 | Boot: A at the RGB screen, A at the cyan screen (enhanced textures) | Textured tutorial room + Mega Man | |
| 2 | Bottom console / boot log | `GAMEPLAY_PROFILE=SOURCE_COMBAT_DEFAULT` and `COMBAT_OWNER=SOURCE` lines in `boot_01501.log` | |
| 3 | Boot location | Starts in the Area 1 ruin room (not inside an Area 0 block); boot log has `BOOT_SPAWN=SOURCE_HANDOFF_80101274` | |
| 3b | Run into every wall and corner of the room for ~2 min, including running jumps into corners | Never passes through a wall, never sinks through the floor, never leaves the room | |
| 3c | Diagonal wall corners | Mega Man stops at them (may feel slightly "sticky"; sliding is not ported yet) | |
| 4 | Press Y while standing | A short yellow ribbon (diagnostic Buster segment) for ~10 frames; **no** old yellow triangle projectile | |
| 5 | Walk to the first tutorial enemy (Area 1) | Enemy is textured, animates and moves (needs `source_rng.l3rg`) | |
| 6 | Face it and press Y repeatedly | After 2 hits: enemy disappears (defeat), defeat sound requested | |
| 7 | Keep walking near where it died | It does not reappear; no crash | |
| 8 | Press X | Nothing happens (lock-on is deliberately disabled in this mode — see blockers) | |
| 9 | Restart holding nothing, X at cyan (0143 safe path) | Old behaviour: untextured safe scene, old triangle projectile | |

Please send back: `3ds/Legends3DS/boot_01501.log`, the newest `hardware_01501_*.csv`,
and a screenshot after step 6. Note anything that differs from "Expected".

A/B switch: create `3ds/Legends3DS/gameplay_profile.ini` containing
`SOURCE_COMBAT_DEFAULT=0` to get exactly the 0.0.14.51 default behaviour.

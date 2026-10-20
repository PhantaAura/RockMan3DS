# 0.0.15.12 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar, use Old 3DS mode, 100% CPU and
the SD folder the script prints, then load the `.3dsx` it prints.

1. Confirm that the title reports `0.0.15.12` and that normal Area 1 gameplay
   appears.
2. **Buster.** Stand still and press **Y** (Azahar keyboard: the key bound to
   3DS Y). Optional repeat: the door-room spawn
   `sdmc:/3ds/Legends3DS/diagnostic_spawn_01414.txt` = `1 -334 -1 -5750 0`.
   - Expected: a thin yellow ribbon from Mega Man straight ahead, lasting about
     10 frames (~0.17 s), on every press. It is a FRONTEND_DIAGNOSTIC drawing
     of the hit segment, not the original shot sprite, and its length (a fixed
     forward ray) is CANDIDATE.
   - CSV `projectiles` stays 0 by design: that column counts the retired
     candidate projectile.
3. Send back `boot_01512.log` and the newest `hardware_01512_*.csv`. On each
   press, look for:
   - `# source_buster_runtime,...,fire_edge=1,spawn_status=0,identity=800bfb20`
     (or a later slot), then `live=1`;
   - `# source_buster_draw,...,drawn=1` on the following frames.

Emulator success does not establish physical Old 3DS performance.

0.0.15.12 reward pickups:
- **Defeat:** shoot a Reaverbot until it is defeated. Diamonds may pop out:
  cyan for refractor shards, red for energy. Many defeats drop nothing,
  because the original size table often rolls "none".
- **Lifecycle:** they arc out, bounce, rest about 5 s, blink, then vanish.
- **Collection:** walk over one; it rises and disappears.
- **Trace:** `# source_reward_pickups` lines show money/HP going up, and
  `# source_reward_expand` shows the spawn count. The marker is a diagnostic;
  the original pickup model is not ported.

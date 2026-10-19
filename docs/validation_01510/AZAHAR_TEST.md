# 0.0.15.10 Azahar test card (Old 3DS model)

Build with `tools/m1_mac_build.sh`. In Azahar, use Old 3DS mode, 100% CPU and
the SD folder the script prints, then load the `.3dsx` it prints.

1. Confirm that the title reports `0.0.15.10` and that normal Area 1 gameplay
   appears.
2. Turning, running and the doors should behave as in 0.0.15.9
   (`docs/validation_01509/AZAHAR_TEST.md`). The camera eye now uses the
   original `rsin`/`rcos` table, so its position can differ from 0.0.15.9 by a
   few units. That should not be visible.
3. **Lock-on.** Walk toward the tutorial enemy (class 20/03) until it is ahead
   of Mega Man and on screen, then press **Y**.
   - Expected: the shot is spawned toward the enemy, because `0x80034EB8`
     selected it.
   - With the enemy well off to the side or behind Mega Man, the shot should go
     straight ahead (no target).
4. Send back `boot_01510.log` and the newest `hardware_01510_*.csv`. Look for:
   - `# source_player_projection,bound=1` once at boot;
   - `# source_lock_on,...,status=0,...,window=N` on Y presses. `N ≥ 0` means
     a target was selected, and `-1` means none was.
   - `packed_trig_substitutions=0`. A nonzero value means the SD pack has an
     old L3RF v1 table; rebuild the pack.

Emulator success does not establish physical Old 3DS performance.

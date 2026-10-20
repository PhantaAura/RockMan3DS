# 0.0.15.14 Azahar test

Manual play (no diagnostic files on the SD card):
1. Walk to the Reaverbots. Press Y facing one: shots should travel straight
   out of the arm and hit it. It loses HP, bounces around at low HP, then
   explodes and drops pickups.
2. Walk over a pickup: money goes up once. Uncollected pickups blink and vanish.
3. At the first door, push Up into it and press A: Area 2 loads. Turn round,
   line up with the door, then Up + A to return.
4. Send back `boot_01514.log` and `hardware_01514_*.csv`.

Unattended (no screen control): copy a script from `autoplay/` to
`sdmc:/3ds/Legends3DS/diagnostic_autoplay.txt` and the spawn line from its
first comment to `diagnostic_spawn_01414.txt`. Remove both files afterwards.

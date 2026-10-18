# 01443 Old 3DS XL check

This build changes the enhanced vertex layout and shader output. It is a candidate fix; Azahar accepted the previous build too, so emulator success cannot settle the hardware bug.

1. Extract the separately named `LegendsWCA43` application folder from the new SD ZIP into the SD card’s `3ds` folder. Keep the existing owner-data folder at `sdmc:/3ds/Legends3DS/data/`. The app archive contains no retail data.
2. Launch **01443 World Comes Alive**. Confirm the version number on the RGB test screen, then press A.
3. At bright cyan, press **B** for enhanced drawing with textures disabled. Allow about five seconds for a visible timeout. Report whether the player and terrain appear or send the error text/photo.
4. If B fails, relaunch and test **D-pad LEFT** at cyan for enhanced terrain only, then **D-pad RIGHT** for enhanced player only. These two choices now test the enhanced renderer; their meaning changed from 01442.
5. **X** still selects the trusted 0143 renderer. **A** selects the initially chosen renderer, normally ANIMATION, with its existing feature gates.

Keep earlier app entries until 01443 has been checked. No data recook is required. The boot log is `sdmc:/3ds/Legends3DS/boot_01443.log`; diagnostics use separately named `hardware_01443*.csv` files. After an error, START exits. GPU resources remain allocated deliberately after a timeout because queued hardware may still reference them.

A still photo of the safe scene establishes visible rendering. It does not establish input response, frame rate, collision fidelity or complete tutorial progression.

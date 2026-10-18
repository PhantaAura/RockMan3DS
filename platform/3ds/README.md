# Native Old 3DS backend — 0.0.14.46

The cumulative backend preserves the 0143 safe path and hardware-observed 01443
enhanced shader/layout. Actual devkitARM r68-1 / GCC 16.1.0, libctru 2.7.0-1 and
Citro3D 1.7.1-2 ARMv6K/mpcore build passes. No New 3DS CPU speedup is required.
Physical 01446 acceptance remains pending.

A at RGB advances to cyan. A at cyan selects enhanced textures, B disables
textures and X selects 0143 safe. START exits. Logs: boot_01446.log and
hardware_01446*.csv under sdmc:/3ds/Legends3DS/. Existing owner data stays under
sdmc:/3ds/Legends3DS/data/. The app-only ZIP adds LegendsWCA46 separately.

Research RNG/source-scene/replay gates are opt-in and excluded from the app ZIP.
The candidate scene-to-Area-1 observation uses a recorded constructor clock and
does not establish fresh boot or retail parity. Do not enable it for an ordinary
hardware regression comparison. No retail assets or DSP are bundled.

[Current report](../../docs/validation_01446/FINAL_REPORT.md) · [Build/tests](../../docs/validation_01446/TEST_COMMANDS.md) · [Hardware instructions](../../docs/validation_01446/HARDWARE_TEST.md).

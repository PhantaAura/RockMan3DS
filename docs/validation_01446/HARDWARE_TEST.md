Legends3DS 0.0.14.46 — World Comes Alive continuation

Extract 3ds/ to your SD. LegendsWCA46 is a separate launcher entry. Keep existing
owner-local cooked data under 3ds/Legends3DS/data/. No recook required. No retail
data, DSP, saves or research INI files are included.

A at RGB advances to cyan. A at cyan selects enhanced textures; X selects 0143
safe. START exits. Logs: 3ds/Legends3DS/boot_01446.log and hardware_01446*.csv.

This fixes outgoing scene CPU resources surviving a committed area replacement
when replay scheduling stops the old actor loop, plus repeated expired-pool
cleanup. Actual Old3DS-targeted native build and all 139 host tests pass. The
affected candidate scene handoff was exercised in Old-model Azahar. Physical
01446 testing remains pending. Renderer/shaders/layout are unchanged.

WCA is not finished. Camera obstruction, fresh source clock, map-edge fall,
full combat and tutorial progression remain open. Keep research gates off for
ordinary hardware tests. The local candidate scene uses a recorded constructor
clock and does not establish retail parity.

# Resume 0.0.14.49

Project: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`.
There is no Git repository, branch, or commit.

Completed: audited the second external Ghidra C export; implemented the bounded
source-backed ordinary 15-part player pose; bound source part-4/part-7 Buster
muzzles; added the owner-local neutral `L3BP/1` cooker/runtime format and fresh
16-selection counter initialization; implemented direct `0x80040E9C` actions
0–3; integrated the new profile and pose attempt into the native runtime. The
real owner-data cook, 143/143 host tests, package probes, and actual ARMv6K build
pass.

Held deliberately: limb selectors 1–3 need the uncooked alternate arm banks;
fire actions 4–7 need helper receipts; live source state must publish the player
position and limb selector before the runtime commits a muzzle pose. No fallback
values are invented.

Next concrete step: recover and cook the alternate eight-part limb animation
bank and the action 4–7 helper outputs, then carry one allocated projectile
through callback/motion, collision, imported-enemy hit, HP, reaction, and defeat.
Fresh retail RNG chronology, broader EBD/dialogue, cross-package progression,
full player collision parity, and physical 0.0.14.49 testing remain open.
`PHYSICS_1TO1=false`; WCA is not finished.

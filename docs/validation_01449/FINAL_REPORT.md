# 0.0.14.49 — source player pose and firing state

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`.
This directory is not a Git repository, so it has no branch or commit. Parent:
0.0.14.48 source ZIP, SHA-256
`7a9a965d01afce6768594aeade9604bcb3e10361655835adf2792a832f12520a`.

The owner-supplied `ROCK_NEO.EXE.2.c` is an external Ghidra-style research
export, not original or buildable game source. It established that
`0x800325D4` evaluates the player pose, `0x800331A0` writes part positions, and
`0x80035724` consumes part 4 (`+0x3C8`) or part 7 (`+0x3E0`) as the two Buster
muzzle lanes. It also exposed the 16-selection counter initialization and the
direct action 0–3 stores in `0x80040E9C`.

The native implementation evaluates the ordinary 15-part primary pose with
bounded fixed storage, fixed-point transforms, the source hierarchy, and no
hot-path allocations. It publishes the two proven muzzle endpoints. It refuses
alternate limb selectors because the current cooked animation contains only the
primary bank, and it refuses an absent player position. Actions 4–7 hold until
their helper outputs are recovered. These limits are reported as
`SOURCE_BACKED_PARTIAL`, not retail parity.

The new owner-local cooker emits a 96-byte CRC-checked `L3BP/1` neutral package
containing the 16 selection subtype/counter rows. The real owner-data cook
completed with 119 files and the runtime pack contains `player/buster.l3bp`.
Neither this profile nor any other cooked retail payload is distributed in the
source checkpoint.

Toolchain: devkitARM r68-1, GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2,
ARMv6K/mpcore hardfloat. All 143 host tests pass in 5.94 seconds. Package
validation and both area probes pass. The actual Old 3DS build passes: 909,556
bytes, SHA-256
`a47d0cc3b78d06dc17b68677bcbaf48a75920977eaa14fc859078cd1a16808a9`.

No 0.0.14.49 emulator or physical-hardware gameplay claim is made. Projectile
callback/motion, Buster-to-enemy hit delivery, HP/damage, reaction/defeat,
complete physics, broader events/dialogue, cross-package transitions, and
natural tutorial progression remain incomplete. `PHYSICS_1TO1=false`; World
Comes Alive remains a technical prototype.

The source ZIP excludes ROM/CHD/BIN/CUE, retail executables, decompiler exports,
retail models/textures/audio/video, cooked retail payloads, DSP, saves, and
generated build output.

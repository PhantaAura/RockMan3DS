# 0.0.14.48 — source player target/Buster bridge

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`.
This directory is not a Git repository, so it has no branch or commit. Parent checkpoint:
0.0.14.47 source ZIP, SHA-256
`dfc397de6077d45b932d98aa269d0879cdf8a68526d04f076724a42609c2b467`.

The owner-supplied `ROCK_NEO.EXE.1.c` is a Ghidra-style external decompiler
export, not buildable or complete game source. It contains 607 address-named
functions but omits dynamic `0x8010` overlay bodies. It established the real
player targeting bridge: `0x80034EB8` consumes the existing `0x80035508`
proxy/GTE owner, performs two target scans, then calls the existing exact
`0x80035724` Buster producer.

The native bridge preserves source mode filtering, paired proxy-row advancement,
target identity and flag rejection, player-relative depth tests, broad and
profile-driven screen windows, and closest-X selection. It has bounded fixed
storage and no hot-loop heap allocation. The five private retail window fields
are emitted only by an owner-local cooker into a CRC-checked `L3TG/1` profile;
neither the profile nor retail values are in this source release.

Six authored broad-lane cases passed through the unchanged original selector:
center selection, nearest of two, screen rejection, depth rejection, flag-`0x80`
rejection, and paired-row advancement. This executed 1,648 original instructions
and 12 modeled RTPS operations. The prerequisite `0x80035508` was return-only and
the `0x80035724` call was captured, so the proof covers selector control flow and
arguments rather than projectile behavior. The strict lane has native tests but
still needs an owner-profile differential.

Toolchain: devkitARM r68-1, GCC 16.1.0, libctru 2.7.0-1 and Citro3D 1.7.1-2,
ARMv6K/mpcore hardfloat. All 140 host tests pass in 5.19 seconds. Package
validation and both area probes pass. The actual native build passes without
warnings; all 282 staged native inputs match their manifest. Binary: 905,700
bytes, SHA-256
`619abb177f923bbe1dc8b5d3ac57c93e06b6105a9aee9d7f04fd7cf8a70553ec`.
The build reused the previously audited cooked pack and did not run an extractor
or recooker.

No 0.0.14.48 emulator or physical-hardware gameplay claim is made. The bridge is
compiled into the native target but is not invoked with invented state. Live
source camera/projection ownership, player muzzle endpoints, Buster counters and
firing bits are not all populated by the current gameplay runtime. Projectile
callback/motion, Buster-to-enemy hit delivery, HP/damage, reaction/defeat,
broader events/dialogue, cross-package transitions and natural tutorial
progression remain incomplete. `PHYSICS_1TO1=false`; WCA is still a technical
prototype.

The source ZIP excludes ROM/CHD/BIN/CUE, executables, decompiled output, retail
models/textures/audio/video, cooked retail payloads, DSP, saves and generated
builds. The app-only SD ZIP contains README, 3DSX and SMDH and expects the user's
existing local cooked data.

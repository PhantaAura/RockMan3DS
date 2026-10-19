# Mega Man Legends 3DS — 0.0.15.3 (M2c deep-overlap recovery)

Ports the original deep-overlap recovery `0x80050400` and the `0x8004B6AC`
correction history. Both are verified against the original R3000 instructions
and cross-checked with Ghidra 12.1.4. When Mega Man is embedded in geometry, he
now moves out to the nearest face under the original rules instead of holding
his pre-move pose. At corners, the previous push is undone as the original
does, so he slides along the free axis. Diagonal walls, Area 1 containment and
source combat are unchanged. The host suite passes 147/147 and the Old 3DS
build passes. Azahar shows boot only; no gameplay collision was checked there.
Details: [docs/validation_01503/RESULTS.md](docs/validation_01503/RESULTS.md) ·
[research](docs/research_01503/DEEP_OVERLAP_80050400.md) ·
Resume: [RESUME_01503.md](RESUME_01503.md). `PHYSICS_1TO1=false`.

---

# Mega Man Legends 3DS — 0.0.15.2 (M2b diagonal walls)

Default boot now starts where the original first gives Mega Man to the player
(Area 1 handoff). Source-backed Buster/hit/defeat is the default combat on the
enhanced renderer. The Area 0/1 map-edge escape remains fixed. Original
diagonal wall classifiers and response paths for families `0x21`, `0x31`,
`0x41`, and `0x51` now drive native contact and sliding. Executable-derived
vectors match for all four families; the owner-local Area 1 audit records zero
unsupported frames, escapes, or falls across 600,000 frames.
Build on the Mac with `tools/m1_mac_build.sh`; test with
[docs/validation_01502/AZAHAR_TEST.md](docs/validation_01502/AZAHAR_TEST.md).
Resume: [RESUME_01502.md](RESUME_01502.md). `PHYSICS_1TO1=false`.

---

# Mega Man Legends 3DS — 0.0.14.51

Cumulative native Old 3DS/XL technical prototype. The imported class-20/03
enemy now completes a bounded source-backed defeat transaction after Buster
damage reaches zero: reaction timing, effect `0x80/0x41`, sound `0xA1`, actor
removal, proxy cleanup, and one exact reward handoff.

The L3CB version-4 owner-local profile retains the source reward descriptor. A
regression drives two eight-damage packets through 16→8→0 HP and proves cleanup,
effect, sound and reward publication happen once without heap allocation.
Missing source evidence or queue capacity holds before partial publication.

The exact RNG-driven pickup expansion, pickup collection/progression and full
defeat-effect visual callback remain incomplete. The host suite (144/144),
owner-local package/replay pipeline and actual ARMv6K build pass. No 0.0.14.51
emulator or physical-hardware result is claimed. WCA is not finished.
`PHYSICS_1TO1=false`.

[Report](docs/validation_01451/FINAL_REPORT.md) · [Tests](docs/validation_01451/TEST_COMMANDS.md) · [Readiness](state/CURRENT_READINESS.json) · [Resume](RESUME_01451.md)

Source releases contain no retail payloads, decompiler exports, DSP, or saves.
Owner-derived cooked data remains local and is excluded from distributable ZIPs.

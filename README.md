# Mega Man Legends 3DS — 0.0.15.0 (M1: default combat)

The source-backed Buster → hit → reaction → defeat path is now the default
gameplay path on the enhanced renderer; the old candidate projectile is retired
there. Build on the Mac with `tools/m1_mac_build.sh`; test with
[docs/validation_01500/AZAHAR_TEST.md](docs/validation_01500/AZAHAR_TEST.md).
Resume: [RESUME_01500.md](RESUME_01500.md). `PHYSICS_1TO1=false`.

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

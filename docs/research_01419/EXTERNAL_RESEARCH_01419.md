# External research used for the 01419 continuation

Research checked 2026-10-07. These are primary project sources and reference
leads. No external engine code or retail payload was copied into this project.

- https://github.com/JoshuaDav10/MML1x10 : work-in-progress PS1 decompilation.
  Its archive chunk document corroborates 2048-byte alignment already used by
  the supplied importer. Actor declarations remain sparse; player health
  offset6E and sound entry addresses are useful independent cross-checks.
- https://github.com/ChrisNonyminus/mml1 : predecessor decompilation. The
  presence of assembly includes is not a completed native implementation.
- https://megamanlegends.gitlab.io/ : DashGL author tools for original model,
  texture, animation, message and tile inspection. These offer format leads;
  they do not establish native physics or enemy behavior parity.
- https://github.com/Drakfix/MML_Modernization-Project : PC engine patches,
  useful as research leads but not an Old3DS source port. Its modern camera is
  not evidence for the requested original-camera rules.
- https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/README.md :
  original BIOS research context and emulator tracing facilities. BIOS routine
  similarity cannot prove Mega Man Legends boot seed or shared RNG call order.

Bounded reference copies are kept outside release source in
`work/pass-20261006-worldalive-01419/external-reference/`. The decompilation's
`include/obj.h`, player declarations and sound source still leave the current
actor detector/contact details mostly unresolved. We therefore used those
projects to locate and corroborate structures, then retained separately
measured original-instruction differentials for the implemented fields.

PHYSICS_1TO1=false. Public research and authored native/native replay are not
live retail/native parity. Original shared BIOS RNG seed and full consumer
cadence remain UNKNOWN; no private per-enemy random stream was substituted.

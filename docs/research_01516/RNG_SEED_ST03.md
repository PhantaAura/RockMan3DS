# rand() seed in ST03 (oracle read, 2026-11-18)

0x8007FE50 is the BIOS A0 stub for function 0x2F (rand); the EXE has no A0:30 (srand) stub, so the
game never reseeds. In the PSXRecomp oracle (OpenBIOS) the seed is the word at **0x80008548**:
two RAM snapshots one frame apart found the only word whose new value is the old one stepped by
seed * 0x41C64E6D + 0x3039, and eight later reads all matched whole LCG steps (9..31 draws per
read, about 2 per frame while ST03 area 1 idles).

* `m3_01_st03_crash_control` at load: seed = **0x0325C4C7**.
* The address is OpenBIOS's; a retail BIOS keeps its seed elsewhere (CANDIDATE for retail).
* The port's shared SourceRngService takes its starting seed from the owner's observed profile;
  this value can seed an ST03 diagnostic profile (CANDIDATE: draw chronology before the
  savestate is not modelled).

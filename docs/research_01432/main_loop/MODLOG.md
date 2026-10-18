# Main-loop reverse-engineering journal

- Inspected hash-guarded owner executable, inherited callback-tail proof, selected
  installed OpenBIOS image and existing boot/linker references. Raw disassembly
  and direct-access candidates remain in the private pass directory.
- Corrected `80012298` meaning: pad decode, before cooperative phase resume.
  Original packet0/1/2/4 produces pressed halfword at pad0+42=`800C0C2A`.
- Proved cold-pad zero initialization through actual `800177CC` loops. Explicit
  boot admission is separate from warm lifecycle and native packet adaptation.
- Located main phase/subphase at `800C1B10/11`, selected phase3 callback
  `800158F0`, update pass `80015DB4`, E0 sweep `80017528`, and active-event
  store-free gate using control byte `800C356C` and active byte `800C4C48`.
- Proved wait comparison and `800123FC` vblank increment. Scratch initializer
  writes only bytes0/1; initial halfword2 stays UNKNOWN. No zero/frame/button seed.
- Added source-safe bounded module, unchanged-instruction authored fixtures and
  host tests. Whole two-pad outputs match all256 cases. A missing-owner test was
  corrected to exercise a branch that actually reads the withheld repeat byte.
- Host warnings-as-errors and independent ARMv6K warnings-as-errors compile pass.
  Decoder stack336 bytes; no consumer allocations. Source scheduling and BIOS
  observation limitations are retained in `../SOURCE_MAIN_LOOP.md`.

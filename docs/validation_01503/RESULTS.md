# 0.0.15.3 validation results

| Check | Result |
|---|---|
| Host suite (`ctest`, clang Release, asserts on) | 147/147 PASS |
| Focused: `test_runtime_contact` deep-overlap + history vectors | PASS |
| Area 1 audit `l3edge_audit PACK 1 2822 -1 -4833 200 3000` | 0 escapes, 0 falls, 0 unsupported, 54 cells |
| Embedded-pose sweep, Areas 1–4 (owner data, throwaway probe) | 1,016,352 formerly held → 622,988 recovered, 389,292 source 9-pass rollback, 4,072 unsupported (Area 2) |
| Ghidra 12.1.4 headless listing vs `psx_mips_dis.py` (0x80050400, 0x8004BB24, 0x8004F5F0 ranges) | 375/375 instructions identical |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 926,240-byte `.3dsx`, SHA-256 `ed6d4e621bb86126ea85386e90bc592c833a7a71a96a79c04d9c81920ae21f1d` |
| Azahar 2126.0, Old 3DS model | Boot only: runtime initialized, first GPU sync, frames 240→1080 traced. Boot entry was terrain inspection (Area 0), so collision gameplay was **not** exercised. |
| Physical Old 3DS | Not run |

REA 6.7.0 (`REA_ANALYSIS_PROVIDER=ghidra`) refused the target before reaching
Ghidra: "unsupported binary format" for the PS-X EXE and "unsupported ELF
architecture" for a MIPS ELF wrapper. Ghidra headless was used directly for
the cross-check.

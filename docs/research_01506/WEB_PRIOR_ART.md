# Web prior-art sweep for event 0 / the Area 2 door (2026-10-19)

Goal: before porting event 0 (`0x800485E4`, forced walk `0x80048A9C`, overlay door
object `0x80112568..`), check whether anyone has already decompiled or documented it.

## Target identity — VERIFIED_1TO1
- Local `ROCK_NEO.EXE` SHA-1 `ffc08fd2b6113e84dc0e75ba63a4feaca0eb1d46` equals
  `hash/us/rock_neo.BIN.sha1` in ChrisNonyminus/mml1
  (https://github.com/ChrisNonyminus/mml1/blob/master/hash/us/rock_neo.BIN.sha1).
  Every address in that decomp and its forks applies to our binary as-is.

## Findings
| Source | What it has for our task | Credibility |
|---|---|---|
| ChrisNonyminus/mml1 (last push 2023-07-23) | `config/syms.us.rock_neo.txt` (429 lines) has **no** names in 0x80044000–0x80053000 except `Pl00_shot_enable_*` and `MojiTask*`. Our functions exist only as splat `nonmatchings` asm (`asm/rock_neo/nonmatchings/func_8003697C/func_800485E4.s`, `…/func_80048A9C.s`, `…/func_80050C2C.s`, `…/func_800514FC.s`, `…/func_80051970.s`, `…/func_800519E4.s`). No C. | Primary (matching-decomp project), but nothing beyond our own disassembly |
| ChrisNonyminus/mml1 ST00 overlay config | `config/overlay/splat.us.ST00/ovl0__PROGBIN_R3_ST00.BIN.yaml`: vram `0x80100000`, rodata at file 0x800, code at 0xB24, data at 0x1A454, end 0x22F00. Symbol file has no names. So `0x80112568` lies in ST00 code/data, unnamed. | Primary |
| ChrisNonyminus/mml1 `src/ST1A/.../eve19.c` (commit "found eve19.c", 2023-03-23) | Only real event naming found: `StXX_eve19_move`, `eve19_main`, `eve19_end`, `eve19_main_area_NN`, `check_block`, `check_swing_camera`. All INCLUDE_ASM. Suggests the original per-event layout is `eveNN_main` → `eveNN_main_area_NN` dispatch; usable as **CANDIDATE** naming for event 0, not as behaviour. Name origin is not documented in the repo. | Single source, origin unstated → CANDIDATE |
| JoshuaDav10/MML1x10 (fork, last push 2025-09-18) | Same syms file as mml1. Has ~1371 per-function `src/rock_neo/FUN_*.c` stubs; none for 0x80048000–0x80053000 targets; the targets again only as asm. Adds per-stage `ST00*.ld` linker scripts. | Primary, no new info for this task |
| Other mml1 forks (MrAdamDonovan, diegocorvalanc, leroysdeath, darcagn, jmanfan213, CryptoDevving) | No commits after upstream 2023-07-23. | — |
| cloud5414/SLPS-91135 (JP Rockman DASH recomp, created 2026-09) | README only, repo size ~1 KB. JP addresses would differ anyway. | Nothing yet |
| GitLab megamanlegends/mml1-psx, dash-tex, mml1 (savestate viewer) | Asset/format tools (EBD/MDT viewers, stages.json). No event/door logic. Possibly useful if the door object's model swap needs EBD/IDX context. | Reproducible community tools |
| GitLab megamanlegends/mml-engine-godot (2023-03-25) | Godot gameplay remake. Top-level scripts are hand-written (`Camera.gd`, `Globals.gd`, `Story_Data.gd`, `Message_handler.gd`, `Cardon_Debug_Map.gd`, …); there is no door or event script at the top level, and the subfolders were not read. It is a reimplementation, not a decomp, so it would not count as source evidence anyway. | Secondary, not useful |
| TCRF Rockman Neo Aug 1997 demo page, MML2 English debug proto | Prototype/debug builds exist for MML2 and an MML1 pre-release demo; no published symbols or event docs for MML1. | Wiki, secondary |

## Cross-check with the local Ghidra export
`/Users/emeraldhunter/ROCK_NEO.EXE.2.c` has no function at `0x800485E4` or
`0x80048898` (only `FUN_80048a9c` at line 22868, then `FUN_80048bec`…). Ghidra did not
create them because they are only reached through the event table `0x80089024`.
Splat in mml1 does split `func_800485E4`, which confirms the start address. Re-export
from REA/Ghidra after marking `0x800485E4`, `0x80048620`, `0x80048704`, `0x80048898` as
functions before porting.

## Gaps
- Nobody has published decompiled C for event 0, the door object, or ST00 overlay code.
  Our port stays the first source-level reading. Evidence must come from our own
  disassembly and replay.
- The `eveNN` naming origin is unknown. Ask in the mml1 project (Discord/issues) if names matter.

## Queries used
- GitHub: `gh search repos "mega man legends decomp"`, `"rockman dash"`, `rock_neo`;
  mml1 + fork trees grepped for the eight target addresses.
- Web: `"Mega Man Legends" decompilation door event overlay ST00 ROCK_NEO`;
  `"Rockman Dash" OR "Mega Man Legends" debug symbols prototype eve19 OR "eve_main"`;
  `Mega Man Legends 1 reverse engineering format EBD PBD MDT IDX dashgl`.
- GitLab API: `groups/megamanlegends/projects`.

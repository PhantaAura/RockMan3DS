# Decomp research for 01421

Checked 2026-10-07 before new implementation. No external engine code or assets were added to the cumulative source.

The cited project is **not a complete decompilation**. The current maintainer describes it as work in progress and not shiftable. Its build recreates a US PlayStation executable/disc using MIPS/R3000 tools and owner game data. That does not provide a native ARM/3DS engine. [Pinned README](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/README.md) and [build rules](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/Makefile).

A decomp translates original machine instructions back into source. A finished PS1 decomp could make accurate behavior reconstruction much easier, but PS1 graphics/audio, fixed addresses, data loading and CPU assumptions would still require a native platform adaptation. Here, substantial code remains original assembly, so switching projects would retain the same unresolved behavior while replacing the already working 3DS infrastructure.

Current `MML1x10` revision: `67c8f34fa94ada6ea225e31b36d841f10067fb6a` (commit date 2025-09-06). Predecessor `mml1` current revision: `964bad3096a2d7a4a3d6cc65678d8440b2ea9cb1` (2023-07-23). These are current master revisions retrieved from the GitHub API; fetched text hashes are in `fetch-manifest.json`.

The sampled player, scene and sound files have 41, 59 and 34 active assembly includes, respectively, with no C bodies in those files. The tutorial overlay is assembly/data rather than recovered actor C. The tree's 1,409 C paths must not be called completed behavior: many are assembly wrappers. No completion percentage is asserted. [Player source](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/src/rock_neo/player.c) and [scene source](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/src/rock_neo/scene.c).

No repository software license was located in the full, non-truncated API tree. This audit does not establish permission to directly redistribute external implementation code. Nothing was copied into the engine. The repository also lists disc payload files; none were fetched. No upstream build or extraction was run.

Useful navigation anchors are `rand` at `0x8007FE50`, `Player_work` at `0x800B51B0`, player `life` at offset `0x6E`, firing enable gates `0x800423A0/0x800423C8`, scene flag read/write `0x8001DA58/0x8001DA8C/0x8001DCD0`, actor flag update `0x8002EE90`, and audio request `0x8001A050`. These labels help locate original routines; they do not resolve shared RNG chronology, enemy attack helper `0x80104078`, enemy damage packets, or EBD opcodes. [Pinned symbols](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/config/syms.us.rock_neo.txt) and [player declarations](https://github.com/JoshuaDav10/MML1x10/blob/67c8f34fa94ada6ea225e31b36d841f10067fb6a/include/rock_neo/player.h).

Continue the existing importer/native runtime. Verify symbol leads against the exact owner executable, then use instruction differentials and read-only live PS1 traces for the missing encounter. Keep unknowns unknown and `PHYSICS_1TO1=false`.

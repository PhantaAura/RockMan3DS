# Player head audit — 0.0.14.26

**The previous pack selected the wrong original head for a fresh game.** The inherited cooker fixes `PL00H00P.PBD`, but the original fresh player initialization and component loader select `HEAD01.BIN` / `PL00H01P.PBD`. This is a supported original-resource selection fix, not a remodel or an inference from filenames.

Source chain:

- Player initializer0x8003BE6C receives an explicit fresh/retained branch argument. In the fresh branch,0x8003BF58 stores architectural zero to `Player_work+0x16A`. The retained branch0x8003BFB8 copies the saved selector instead; it must not be silently treated as fresh.
- Component loader0x8001F5E4 reads that signed byte at0x800B531A and indexes halfword table0x80089200. The original table maps fresh selector0 to fileID28. It compares cached ID0x800C1B80 and calls original `Cd_read_comb`0x8001D440 only when the selected file differs. The source startup cache initializer0x8001338C writes the retained cache to-1.
- The original64-byte filename array begins at0x80083870, including LOGO.EXE, ROCK_NEO.EXE and SYSTEM.CNF before DAT entries. FileID28 points to the HEAD01 filename at0x80083F70 and directory record0x80082E20: LBA1781,length14336. The supplied HEAD01 container matches that length and contains the original H01 mesh at load0x800F7800.

A bounded unchanged-instruction probe tests256 authored initial selector values through the fresh stores and8 authored selector/cache combinations through the loader. All264 cases pass. It stops before external asset-loading calls; no instructions are replaced and no geometry/palette/instruction payloads are exported. `fresh-player-head-source-proof.json` records exact counts and scalar provenance. This establishes selected fields in this fresh branch; it is not hardware or full gameplay parity.

Read-only owner inspection confirms two distinct original four-part meshes with IDs1,15,16,17. HEAD00 has112 vertices,46 triangles,61 quads; HEAD01 has102 vertices,76 triangles,31 quads. HEAD00 has80 faces using0x3C000005 plus27 using0x3E48000F; HEAD01 uses0x3E48000F for all107 faces. Original head materials/UVs must travel with the selected resource and ordered VRAM uploads. Do not borrow a palette from the other head.

Both meshes use packet codes0x25/0x2D, RGB128, with the semitransparency bit clear. Root separately found that the native renderer retained intermediate STP alpha for opaque primitives and is correcting that. This does not establish full blend parity, dynamic-palette timing, ordinary camera fidelity or complete player appearance.

Root checked genuine original PS1 tutorial captures showing brown hair without a helmet. This observation agrees with the source-backed fresh selection; the original captures remain outside the code-only release. Runtime visual acceptance is recorded by the root after cooking/building/capturing the corrected original resource.

Implementation helper: `tools/source_player_head.py`, API `select_fresh_player_head(image: bytes, dat_dir: Path) -> dict`. The existing OwnerExe guard admits only the exact supplied executable; the helper derives selector/table/file/resource identities from owner inputs, validates filename/record/length/resource bounds, and writes no payload. Three authored unittest methods pass, including11 malformed variants. No retail selection table is embedded in the source. The proof is reproducible with:

```sh
python3 -B docs/research_01426/player_head_source_probe.py --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT
python3 -B -m unittest discover -s tests -p test_source_player_head.py -v
```

An initial filename-slot-to-file-ID guess failed length checks and was rejected. Correcting the array origin with its three executable/config entries resolved the mapping; the final helper validates the selected directory length. No guessed head replacement is used.

Public decomp navigation from the pinned MML1x10 research cache labels `Cd_read_comb`; its sampled player code remains assembly wrappers. New web fetches of pinned source/symbol pages failed, so no new conclusions depend on them and no external implementation was adopted. The owner executable is the behavioral evidence.

Remaining unknown: resumed saves and later gear/head mutation must preserve their proven selector ownership; this fresh-only correction does not implement those systems. Physical Old3DS and full original/native visual comparison remain pending.

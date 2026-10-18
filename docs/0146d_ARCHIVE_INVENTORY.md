# Bounded archive/input inventory — 2026-09-30

The next hardware pass has an existing cooked-pack input, but this inventory found no new loose source-data filenames that can fill the recorded asset gaps. This is a metadata conclusion, not a content, provenance, rights, or retail-fidelity verification.

## Scope

Read only ZIP central-directory filenames, sizes, and archive-supplied CRC values for the two supplied archives; current source-tree filenames/sizes; filenames/sizes in the existing tutorial pack; and the text of `docs/pack_audit.md` and `tools/build_hardware_test.py`. No member payload was opened, no nested archive was read, no raw commercial binary was opened or parsed, no extraction/cooking ran, and no asset hash was recomputed. `owner-data/` and synced `sources/` were not examined or modified. Complete metadata is in `archive-inventory.json`.

CRC values below are stored archive metadata. They were **not independently verified against contents**.

## Supplied game-files archive

`/Users/emeraldhunter/Downloads/Mega Man Legends Files.zip`: 646,852,939 archive bytes; 6 file members.

| Member filename | Uncompressed bytes | Compressed bytes | CRC32 metadata |
|---|---:|---:|---|
| `Megaman Legends Rom/Mega Man Legends (USA).cue` | 214 | 116 | `9e88194d` |
| `Megaman Legends Rom/MML_extracted.zip.part-ab` | 7,985,612 | 7,918,056 | `52b25f5d` |
| `Megaman Legends Rom/Mega Man Legends (USA) (Track 2).bin` | 37,396,800 | 36,354 | `7976083e` |
| `Megaman Legends Rom/Mega Man Legends (USA).chd` | 217,298,243 | 217,008,027 | `207b4abb` |
| `Megaman Legends Rom/MML_extracted.zip.part-aa.zip` | 188,743,680 | 188,772,485 | `12cf1f66` |
| `Megaman Legends Rom/Mega Man Legends (USA) (Track 1).bin` | 487,205,040 | 233,116,027 | `1f55bd97` |

There are no visible outer ZIP members with loose `.TIM`, `.CLT`, `.PBD`, `.EBD`, `.MBD`, message, audio, or cooked `.l3*` asset suffixes. The two `MML_extracted.zip.part-*` names suggest split/nested storage, but filenames do not establish their contents or completeness. Their payloads remain unknown under this audit's restrictions. CHD/CUE/track BIN members are excluded importer inputs.

## Integration source archive and loose current source

`/Users/emeraldhunter/Documents/Legends3DS_0.0.14.6a_Integration_Source.zip`: 346,296 archive bytes; 225 file members. Filename suffixes comprise C/C++/headers, Python/shell scripts, shaders, documentation, JSON, a SHA-256 manifest, CMake text, and a Makefile. No raw or cooked asset suffix candidates occur in this archive.

`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.6c_HardwareTest`: 352 regular loose filenames inventoried with protected/owner-data directories excluded. No filename has a raw asset suffix or a supported cooked `.l3*` suffix. Existing `.3dsx`, `.elf`, `.smdh`, `.o`, and shader binaries are build artifacts; their bytes were not read and they are not game-data importer inputs. These observations apply only to the bounded locations above; no broader search was made.

## Precise allowed asset input

The existing builder is `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.6c_HardwareTest/tools/build_hardware_test.py`. Its asset argument is a directory, not an archive or raw source-data path:

```text
--pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/full-build/tutorial/legends3ds/tutorial
```

This directory currently has 101 regular filenames: `manifest.json` and 100 cooked files totaling 3,673,827 bytes by filesystem metadata. All 29 frontend-required filenames from the builder are present. Supported cooked suffixes are `.l3anim, .l3ebd, .l3flow, .l3map, .l3mc, .l3model, .l3msg, .l3pal, .l3pc, .l3ref, .l3render, .l3skel, .l3tex`.

The input remains conditional on the builder's normal content validation: format-1 manifest, exact listed inventory, byte lengths and SHA-256 matches, no symlinks, supported suffixes, and required frontend files. This inventory did not rerun those validations. `docs/pack_audit.md` reports its earlier 100/100 size/hash check and 22/22 read-only probes; those are prior documented results, not checks performed here. The builder requires a fresh or empty `--out` separate from source and pack, and copies the validated pack for a local SD tree without cooking or extraction.

The builder does not accept raw loose assets, CHD/CUE/track BIN, EXE/ROM/raw commercial binaries, nested split archives, or owner-data. No change to that contract is proposed.

## Remaining blockers

- No newly usable raw source-data candidate is established by the supplied archive/member filenames or current source inventory.
- The existing audit records 3 unique unresolved material words (`0x7c810016`, `0x7c830016`, `0x7cc20017`) affecting 1,184 terrain primitives. This inventory found no independently verified replacement input for them.
- Correct original palettes/load ownership, animation semantics, event behavior, audio playback, retail fidelity, and Old 3DS performance cannot be established from filenames or CRC metadata.
- Any future separate loose-input importer needs explicitly supplied, permitted input paths and a declared format contract. It must not infer that nested archive contents, raw commercial binaries, or unexamined locations are usable inputs.

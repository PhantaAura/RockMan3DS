# Offline local game-data installation

This cumulative checkpoint adds the local installation pattern discussed with
the user: project releases carry original source/tools and compatibility
fingerprints; the player supplies supported game data on their own computer.
The user explicitly lifted the earlier restriction on local extraction/conversion
in this conversation. No website, server upload or disc-protection bypass is
implemented. A supported-file hash is a compatibility check, not ownership proof.

## Supported input

Use the supplied reference extraction root containing `ROCK_NEO.EXE` and
`CDDATA/`, its `CDDATA` or `CDDATA/DAT` subdirectory, or one complete ZIP containing
exactly one such extraction. Directory input is read in place. ZIP input is
bounded, checked for unsafe paths/link/encryption/type/collision entries and
unpacked only into a new private local staging directory. All 247 named profile
files (205 DAT, 38 XA, one STR and three root anchors) must match the recorded
sizes and SHA-256 values before cooking. Safe unrelated ZIP files are ignored;
extra files in the canonical game directories are rejected.

The full profile fingerprints the user-supplied dataset. Its 13 inherited
reference anchors pass; the remaining fingerprints identify this dataset, not
an independently authenticated retail master. The profile contains no retail
bytes. Source validation and build-time tests use synthetic fixtures only.

Direct CHD/BIN/CUE/ISO input and split archives are rejected with a specific error.
The supplied mixed game-files ZIP contains disc images and split extraction ZIP
pieces rather than one complete loose extraction, so choose the existing loose
folder for this milestone. Future disc intake must preserve PS1 MODE2/XA sectors
and pass the same output profile; an untested generic ISO reader is insufficient.

## Commands

Run Python 3.9+ and CMake with a C/C++17 host compiler. For native output, install
devkitARM/libctru/Citro3D and set `DEVKITARM`/`DEVKITPRO` if they are not under
`/opt/devkitpro`. No supplied game executable is executed.

```sh
./build_mml3ds.sh /absolute/path/to/extracted \
  --out /absolute/path/to/new-local-build \
  --cache /absolute/path/to/separate-local-cache
```

`--out` must not exist and must be separate from source and owner input. Cache is
also separate; its default is `mml3ds-local-cache` beside `--out`. The cache is
locked during installation. An interrupted run can leave `.installer.lock`;
confirm that the old installer stopped before removing that lock. Prior output
is never replaced. Failed runs preserve a `build-result.json` and command logs.

For a code-only rebuild, repeat the command with a fresh `--out` and the same
cache. Relevant input/cooker/script hashes key the cooked tutorial cache, and
every cached manifest/inventory/hash is revalidated. Generalized resource tasks
use the same external cache and are keyed by real source/tool bytes. A changed
cooker invalidates affected tasks. Invalid generated caches are rebuilt or
quarantined; they are not accepted as compatibility evidence.

`--host-only` produces local data without a native executable. `--tutorial-only`
restricts generalized cooking to the eleven current tutorial/player containers;
the default processes the complete supported DAT inventory. Neither option
changes unknown gameplay semantics.

## Connected output and limits

The command verifies the source foundation, checks the owner-data profile, builds
the inherited host tools, runs all host tests, cooks/validates generalized MML2
packages and the supported tutorial sidecars, runs native-format/runtime probes
and nine deterministic native replay scenarios, then compiles the actual current
ARMv6K target when the toolchain exists. It does not run the expensive full
executable-research script during an ordinary install.

It publishes `build/Legends3DS.3dsx` only after successful native compilation.
`sd_out/3ds/Legends3DS/` contains the executable, tutorial sidecars and the actual
generalized importer's boot package at `data/legends3ds/areas/boot.mmlpkg`.
The generalized map/render sections must be byte-identical to the tutorial
sidecars before the native compatibility adapter can bind them. This is the
existing runtime contract, not guessed room-specific material/event selection.
Per-package validation uses separate logs. The copied SD data is verified again
and recorded in a local `sd-manifest.json` before exclusive publication.

Only map/render are read from the generalized boot package at runtime. Tutorial
messages/materials/flow/EBD still use the verified compatible sidecar adapter.
Generalized objects, enemy roles, event semantics, camera, audio and progression
bindings remain incomplete. All-package structural parsing and native-vs-native
replays do not demonstrate a complete game, retail parity or physical performance.
PHYSICS_1TO1 remains false; the inherited terrain fidelity gaps remain open.

## Distribution

`new-local-build`, the cache, SD data, raw/extracted game files and generated
packages contain owner-local data. Do not redistribute these directories.
The code-only source ZIP contains code, build scripts, tests, formats, hash-only
profiles and validation documentation. The source release collector rejects
raw/cooked/build payload suffixes, symlinks, special files, unexpected extensions
and binary/non-UTF-8 content. A filename check alone cannot grant rights to
arbitrary material added later; review release contents before distribution.

The current end-to-end run was tested on macOS. Linux/Windows host installation
has not been exercised in this milestone; Windows executable-suffix/toolchain
handling requires a separate pass.

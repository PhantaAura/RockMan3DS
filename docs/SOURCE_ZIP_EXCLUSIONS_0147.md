# Source checkpoint exclusions

Preserve all cumulative source, headers, project/Make/CMake files, shaders, tests, tooling, historical/current documentation, metadata, provenance hashes and diagnostic text logs/CSV. Exclude only generated cache/build directories such as __pycache__, build_3ds and Git internals when encountered. No such excluded directory is required to build.

No retail assets or cooked payloads are present in the distributable source tree. All original ROM/CHD/BIN/CUE/EXE files, retail saves, game model/texture/audio/video data, tutorial pack, boot.mmlpkg, SD layout, native binaries and native-source build output live outside this tree and are omitted from the source ZIP. The local SD handoff is not a redistribution artifact. The actual compiled .3dsx is supplied separately.

The packager rejects unexpected source extensions and symlinks, refuses an existing output, regenerates SOURCE_MANIFEST.sha256, and verifies archive CRC. A separate final check compares every ZIP member to the expected source inventory and bytes.

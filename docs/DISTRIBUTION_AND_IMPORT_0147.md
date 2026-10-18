# Distribution and local game-data installation

This remains a native Mega Man Legends port/reimplementation for Old 3DS. The goal is the same game content and behavior through the first Tron Bonne fight; no replacement game content is introduced. This is not PS1 emulation.

Distribute project source, runtime executable, tests, tooling and neutral format documentation. Keep the original retail files and all cooked game payloads on the owner's machine/SD. Do not bundle or host models, textures, dialogue, audio, video, save data or cooked packages.

Unleashed Recompiled documents a program release without game assets, with the user supplying files from a legally acquired copy: https://github.com/hedge-dev/UnleashedRecomp/blob/main/README.md . Its static recompilation backend is not an Old 3DS solution for this project; the applicable distribution pattern is user-supplied local data plus a separately distributed native program.

An eventual website should run supported conversion/patching locally in the browser and offer a download of the owner's SD data. It should not upload game files, host retail payloads, claim that file hashes prove ownership, or promise legal immunity. A PS1 binary patch alone cannot implement the missing ARM/PICA native gameplay systems. The native loader and proven converter contracts must work first. No website or ROM/CHD/EXE importer was built in this checkpoint.

Current implemented step: tools/package_cooked_area.py accepts only two explicit ALREADY COOKED L3MP/L3RG files, checks format/size/CRC, writes MML2 sections1/2/3, and records SHA-256 provenance. It does not extract retail files or invoke a cooker. tools/build_hardware_test.py validates the existing authorized cooked manifest, runs host tests, wraps that pair, runs the runtime probe, compiles the native target, and produces a LOCAL ONLY SD tree. Code changes do not require retail recooking.

The native runtime first tries data/legends3ds/areas/boot.mmlpkg. It validates/indexes every envelope section, decodes map/render only, and preserves unsupported sections as descriptors. The legacy tutorial adapter binds flow/messages/materials only after bounded byte equality of package map/render and the already supplied local pair. A different pair gets no tutorial palettes or invented object/event bindings. An absent/invalid/unrenderable optional package uses the loose data path.

Keeping retail assets out of a release avoids bundling them; it does not itself grant permission for every use or derivative work. See the U.S. Copyright Office's explanation of derivative works: https://www.copyright.gov/eco/help-limitation.html . No copy-protection bypass, new extraction of code/assets from commercial binaries, or original save modification was performed in this pass.

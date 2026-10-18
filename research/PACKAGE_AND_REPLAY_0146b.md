# 0.0.14.6b formats and validation boundaries

The generalized owner-side cooker discovers resources by container offset and internal header. A gameplay state is identified by its container, never by its geometry hash. Geometry identity is the SHA-256 of a canonical map of the four source geometry payload hashes. States without their own quartet retain an unknown geometry binding; a matching filename prefix does not establish a retail loading relationship.

The graph distinguishes measured containment from runtime use. An EBD resource contained by a state is not proof that a particular event executes. The object scanner remains a candidate detector: terminated 20-byte records and consecutive overlay pointers do not establish object roles. Texture and palette uploads are catalogued, but generalized upload ordering is unknown. The inherited tutorial material cooker still supplies the actual frontend using its previously researched load sequence. No unrelated palette is substituted.

## Area envelope

All integers are little endian. Header: magic `MML2` (u32), version 2 (u16), count (u16), total bytes (u32), source-profile identifier CRC (u32), section-table CRC-32 (u32). Each 16-byte section row contains kind, offset, byte length, payload CRC-32. The CRC uses the standard reflected IEEE polynomial. Empty sections are allowed; nonempty sections cannot overlap. Section kinds must be unique. Offsets may be unaligned, and readers use byte loads. CRCs detect corruption, not malicious modification.

Kinds: 1 is UTF-8 JSON provenance/metadata; 2 is the existing collision/map format; 3 the existing render format; 4 the existing object-list format. Kinds 0x100 and above hold individually cooked EBD directories or message banks, with the resource-to-section association in metadata. Unknown components are absent, never populated with invented scripts. CRC enforcement tightens the previously incomplete v2 validator: packages with zero placeholder CRC fields must be recooked. No production v2 writer existed in the baseline.

The standalone native reader verifies every envelope and delegates each known payload to its inherited parser. The current 3DS frontend continues to consume the tutorial layout; automatic runtime loading of arbitrary new envelopes is not integrated. Thus host import coverage is not playable-content coverage.

## Incremental dependencies

Cache keys include the actual input payload hashes, command arguments, cooker executable hash, and the Python importer/decoder hashes. Hits verify the output hash. A code change unrelated to a cooker does not invalidate that cooker's output. The compatibility tutorial pack has its own source/tool fingerprint and output hash checks. The graph and small package wrappers are regenerated; the expensive decoder tasks are reused. Cache directories are local owner data and must not be distributed.

## Replay

`l3replay TUTORIAL_DIR INPUT.tsv OUTPUT.json [--no-animation]` runs the existing RuntimeSession and the same animation bridge logic as the 3DS frontend. Input columns are decimal `frame held_psx_mask fire lock_held advance_dialogue`, starting at frame zero without gaps. There is no wall-clock input. Output includes position, fixed accumulators, work velocities, heading, states, animation request/frame, map lookup, camera, projectiles, HP and dialogue waits. Grounded remains null because the full retail solver is not connected. The frontend's out-of-map rollback and rendering are outside this harness.

The suite runs each scenario twice, then with animation disabled and compares all non-animation fields. Named turn scenarios specify input durations, not an assertion that the final heading equals the name. Lock scenarios do not demonstrate target acquisition because no retail target is registered. Unknown movement cases are explicitly recorded as UNKNOWN. No PS1 golden trace was supplied, and repeatability does not prove retail parity.

## Save and profiler

Native save format v2 is explicitly encoded little endian. Writes use unique sibling temporary files and rename, preserve a valid backup before replacing a valid primary, and do not overwrite a good backup from a corrupt primary. Reads validate before exposing payload to the caller, reject trailing bytes, and support backup recovery. Tests cover host filesystem behavior, not SD power-loss durability. Directory metadata is not fsynced; generation/header corruption beyond the checked fields is not CRC protected by the inherited v2 format. The original retail save was not present in supplied assets and was not modified.

Diagnostic feature masks allow animation without textures and player textures without terrain textures. Dependencies needed by GPU code are validated, invalid masks choose the fallback, and SELECT overrides the file. Rendering fallback shader remains byte-identical. Profiler fields include phase time/peaks, memory and draw budgets. The frontend records update time, render time including synchronization, frame time, and draw calls. Heap allocation completeness, GPU time, frame pacing and memory peaks still need device instrumentation and testing.

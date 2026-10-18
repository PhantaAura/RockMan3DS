# Existing tutorial pack accuracy audit — 2026-09-24

**Verdict: internally consistent but incomplete; retail accuracy remains unverified.** The pack can support the authorized local technical test, provided its missing materials and gameplay semantics stay explicit. It should not be called a faithful or complete port.

Pack: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/full-build/tutorial/legends3ds/tutorial`

## Genuine checks

- All **100/100** manifest outputs match their recorded SHA-256 and lengths. The only additional file is the manifest itself. Its claimed source is self-declared metadata, not independent provenance or rights evidence.
- **22/22** existing read-only host reader/probe commands returned zero. Full command arguments, executable hashes, exit codes, stdout and stderr are preserved in `pack_audit.json`.
- Render bridge expands **15,512 terrain primitives** across five areas (93,072 submitted triangle vertices under its quad expansion). This is geometry self-consistency, not an Old 3DS performance measurement.
- Host session probe loads/transitions/moves one tick in all five areas, produces 65 bottom-screen model commands, serializes a 322-byte save, and queues one audio request. It does not play audio or save to the 3DS SD card.

## Material accuracy gaps

| Area | Resolved / total terrain primitives | Missing material words |
|---|---:|---|
| 0 | 928 / 1,752 | `0x7c810016`, `0x7c830016`, `0x7cc20017` |
| 1 | 494 / 854 | `0x7cc20017` |
| 2 | 674 / 674 | None recorded |
| 3 | 120 / 120 | None recorded |
| 4 | 12,112 / 12,112 | None recorded |

A total of **1,184** terrain primitives reference unresolved entries. Both player material words resolve in all five areas. Inherited documentation calls the three unique unresolved words dynamic CLUT cases (`platform/3ds/README.md:71`, `research/integration_0146.md:38`); this audit confirms that the entries remain unresolved but cannot independently prove their original dynamic palette ownership or loading sequence. Resolved means the existing resolver finds data, not that its palette, UVs, transparency, and load ordering match the original game.

## Animation and object limitations

Main animation CRC and **55** S-to-M request mappings pass structural bounds checks, including movement request `0x0c`: 833 records, 10 branch records, 45 repeat records. Left/right animation files pass their readers. The one special M entry is not a normal frame table; source identifies the skeleton anchor separately. These probes do not replay all track transitions or independently verify original animation timing, pose transforms, state semantics or visual fidelity. Current source encodes a limited PL00M00 request-to-pose relationship (`src/animation/pbd_animation.cpp:108`).

Across 49 stage object records the runtime probe reports **3 unique, 26 narrowed, 6 ambiguous, and 14 without a local EBD candidate**. The six ambiguous records occur in area 2. Missing local candidates may require a different runtime resource context; zero-exit parsing does not establish correct object behavior.

Full collision response, retail motion/camera semantics, enemies, event execution, audio playback, natural transitions, cutscenes and 3DS save integration remain outside these probe results. Prior source audit found those incomplete or unused in the frontend. No independent retail trace, original-run comparison, or supplied reference screenshot was available to this bounded audit.

## Scope and preservation

Read only the existing cooked tutorial pack, supplied source and existing reader/probe executables. No raw owner data, commercial executable or disc was inspected; no extraction or cooker ran; no game assets were written or modified. JSON and this report contain metadata and diagnostics only. Emulator/hardware visual and performance tests belong to the next step after reviewing these limitations.

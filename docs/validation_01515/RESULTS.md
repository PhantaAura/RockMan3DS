# 0.0.15.15 validation: tutorial message triggers (E0/06) and message box

Build: `pass-*-m2-01515`. `Legends3DS.3dsx` is 1,013,900 B, SHA-256
`1b23aa0d6699d4e0bab64b054920cad1964a62311fb61d84195cba24613d8b1b`
(not committed). Host: 155/155 PASS (new `source_tutorial_triggers`). Native:
PASS, 0 warnings.

**Azahar 2126.0 (Old 3DS).** A normal boot with
`autoplay/tutorial_radio.txt`; trace evidence (`# source_tutorial`). Screen
capture failed during the final run. The owner's own Azahar session ran
alongside it and was left untouched.

| Step | Original (PCSX-Redux, `docs/research_01514/TUTORIAL_MESSAGE_TRIGGERS.md`) | Port (Azahar) |
|---|---|---|
| Radio trigger (type 1, cell z −12) | freeze at z −5643 | **match**: freeze at z −5633 (cell 5:−12); `+0xBD`=0x40, pause mode 1 |
| Message flow | block 0 → CALL → block 1 instructions → Yes/No | **match**: block 0 → 1. Yes repeats block 1, No continues to the end |
| Completion | flag 1552 set, control returns | **match**: flag 1552 = 1, `+0xBD`=0, pause mode 2, walking resumes |
| Next trigger (type 2, cell z −19) | "Turn right there." without a freeze | **match**: block 2 shown without a freeze; it closes after the CLOSE delay |

Not covered here:
- Types 3–8 later in Area 1 and Area 2's types 9–16 were not reached.
- Type 16 (`0x801156C8`) is not decompiled and holds.
- Voice lines (`0x8001D648`, audio 0x8F) are logged, not played.
- Player `+0x44D` (camera eye test) belongs to the player-movement writers
  (`0x8003CF58` etc.), which are not ported.
- The message box is drawn on the bottom screen (FRONTEND_DIAGNOSTIC), not as
  the original top-screen window.

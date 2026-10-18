# Prebuild Fidelity Pass

This source checkpoint folds proven gameplay-fidelity fixes into the integrated runtime before any 3DS hardware build.

Implemented in this pass:
- L3SV v2 expands story/progression storage to 0x800 flags (0x000..0x7FF).
- Classic lock-on default range is the proved USA threshold (<0xD0), scored in X/Z, and Hold mode rescans while held.
- Unlocked Buster forward direction uses the same PS1/Q12 heading convention as movement when reference tables are loaded.
- Gameplay camera offset rotates behind the player rather than remaining fixed on world Z.
- Dialogue/script camera ownership suppresses gameplay movement/fire/lock-on input.
- AUDIO message commands block the message cursor until the matching 32-bit command id is completed by the audio backend.
- WAIT message commands use an explicit frame countdown and are not bypassed by Fast/Instant text.
- Safe cutscene skipping requires a registered completion contract and a 60-frame hold. Completion applies the registered story flags, Buster state, optional destination, dialogue cleanup, and camera ownership cleanup.
- The bottom-screen cinematic layout only exposes the skip affordance when a completion contract exists.

Validation:
- Release build uses warnings-as-errors.
- CTest core suite passes.
- Exact USA reference verifier passes against the project extraction and CHD.
- Fresh user-data tutorial cook produces 91 files.
- All five tutorial area runtime probes pass.
- Integrated tutorial runtime session probe passes across all five areas.

Still not claimed as playable 0.1: exact collision response, full combat ownership, retail EBD/event execution, enemy AI, complete camera choreography, XA decode/playback backend, and libctru/Citro3D hardware backend remain open.

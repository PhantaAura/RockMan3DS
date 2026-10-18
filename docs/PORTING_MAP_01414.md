# 0.0.14.14 porting map and initial audit

Baseline: complete 0.0.14.13 ObjectResources source checkpoint, copied into a separately named working source tree. Neither the workspace nor the source checkpoint is a Git repository: branch/commit/uncommitted Git state are unavailable. Original sources and archives remain reference material. Inherited Release build and all 25 CTest suites pass before implementation.

Reuse: native ARMv6K/libctru/Citro3D renderer with the 0143 fallback; existing map/render/material/model/animation decoders; L3EB v2 byte retention and bounded resource lookup; tutorial/session registries; source-backed player compatibility/contact helpers; current bottom-map/UI controls; native diagnostics; deterministic replay and evidence infrastructure; verified owner-only cooked pack. Camera projection measurements and the prior PCSX-Redux reference configuration are retained.

Change for Old 3DS: connect evidenced actor resources to bounded native lifecycle APIs; connect contact and vertical motion to normal gameplay where format and formula evidence permit; prevent duplicate immutable EBD source storage; keep bottom-screen work inexpensive; extend trace comparison without confusing native replay with retail measurement. Platform-specific drawing/input remain isolated. The 0143 shader and fallback path must remain bootable.

Unverified: complete actor resource formats, model/material/animation bindings, contact geometry interpretation and whole physics parity; original HUD page/CLUT mapping; enemy/event semantics; real hardware performance and touch behavior. Use SOURCE_BACKED_PARTIAL/MATCHES_MEASURED_FIELDS only for the fields actually supported; PHYSICS_1TO1 stays false until comparisons justify otherwise. Original Old 3DS testing awaits the owner. Azahar Old-model testing cannot establish physical-device headroom.

First testable milestone: preserve the passing baseline, capture repeatable PS1 input/position/state observations, execute normal runtime contact/jump tests, and prove one bounded actor resource consumer chain. Progress to real actor drawing only after actual types/extents are established. Record exact blockers rather than inventing a model or behavior. Bottom-screen companion improvements follow these gameplay gates and preserve original top-screen framing/HUD semantics.

Release: actual host/native builds and genuine emulator captures; complete cumulative source-only ZIP, SHA-256, readiness/evidence/changed-file reports. Owner payloads and generated binaries remain outside distributable source. No usage reset.

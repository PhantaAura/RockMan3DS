# Terrain placement pass 0.0.14.10

The complete 0.0.14.9 tree was copied after all 421 parent manifest identities passed. It has no Git repository, branch or commit. The frozen parent passed Release compilation and all 13 inherited CTest suites before edits. The active native frontend is `platform/3ds/source/main.cpp`. Map/render/package readers, materials, renderer gates and movement are reused. Most separate P2 research remains host-only.

The audit identified positive grid entries as visibility aliases of full IDX identities. The original positive draw loop visits each identity once at its authored XYZ. The four inspected positive primitive families use signed X/Z and unsigned Y subtraction. Whole-field raw6 selects the bank. The inherited host visual bridge has the same legacy placement mistakes as the old frontend, so agreement between them does not prove fidelity.

Reuse the pointer-free L3MP/L3RG package and its retained geometry bytes, PS1 VRAM load order, UV/material identity, area-local resource lifetime, the 0143 fallback, owner-data installer/cache and ARMv6K toolchain. Preserve the original flattened gather and shaders for that fallback.

Change the enhanced native path through a small shared positive placement API, a bounded count before VBO allocation and a separate positive geometry decoder. Placement runs during area loading, with no new per-frame allocations. Assets need no recook: existing data retains every required byte, and material caches already scan both banks.

This milestone loads all five diagnostic areas through that shared path, exercises current native compilation and host validation, captures genuine Old-model Azahar results, and records remaining failures. Negative-cell geometry/material bindings, original collision/grounding/camera behavior, dynamic LOD and clipping, and physical performance remain unresolved. PHYSICS_1TO1 remains false. Retail visual parity must not be claimed from decode or compilation alone.

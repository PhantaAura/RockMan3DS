# Enhanced renderer compatibility audit

Physical 01442 evidence: shader-free cyan transfer works; X displays 0143 terrain, player and bottom UI. ANIMATION and BATCHING / TEX OFF both report a gameplay-frame GPU queue timeout. The generic expected-data path on that error screen is not a missing-file diagnosis.

01443 retains the same five-float terrain vertex and position/UV values. Buffered attribute slots become position → shader v0, UV → v2, fixed colour → v1. The buffer permutation is 0x10; the fixed colour setter uses attribute slot 2. Immediate draws restore dynamic position, colour and UV slots in that order. Layout assertions and checked buffer admission reject CPU-side mismatches. The 0143 draw path and shader remain unchanged.

The UV shader now writes its entire texture-coordinate output, matching the official example. Only XY have TEXCOORD0 semantics; this is not evidence that texture W was required. No authoritative rule was found declaring the previous interleaved fixed-slot layout invalid. These two changes therefore remain candidates until a physical retest. They do not establish which GPU command caused the fault.

Primary references inspected:

- [Citro3D 1.7.1 attribute setup](https://github.com/devkitPro/citro3d/blob/v1.7.1/source/attribs.c): sequential attribute slots and shader-register permutation are separate.
- [Citro3D 1.7.1 buffer setup](https://github.com/devkitPro/citro3d/blob/v1.7.1/source/buffers.c): buffer mapping, stride and admission results.
- [Official textured shader](https://github.com/devkitPro/3ds-examples/blob/master/graphics/gpu/textured_cube/source/vshader.v.pica): full texture-coordinate MOV.
- [Picasso manual](https://github.com/devkitPro/picasso/blob/master/Manual.md): TEXCOORD0 and separate texture-W semantics.

The enhanced shader’s deliberate revision is pinned in the source verifier. The original 0143 shader hash stays `fd242f3023a04cb03ba80ee20ea716daff2f9921f54502f5eeefa1d3a2b373bb`. Existing protection was updated explicitly rather than disabling it.

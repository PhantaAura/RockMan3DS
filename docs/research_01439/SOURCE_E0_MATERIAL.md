# E0 semitransparent material path

Original packet classification is `SOURCE_BACKED_PARTIAL`; PICA pixels, source camera/depth/order and physical Old3DS validation remain `CANDIDATE` / `HARDWARE_PENDING`.

The existing diagnostic draw skipped any E0 record whose modulation had bit25. This hid the source grab result02808080. Original8002A6B8..CC masks the word with02FFFFFF;8002A6E8..738 and8002A908..920 construct flat textured24/2C commands. The grab therefore selects26/2E, with neutral sourceRGB808080. It does not mean hidden, raw texture, an alpha-byte opacity, or a Gouraud packet. The existing shared `source_flat_textured_color_02ffffff` helper supplies this exact construction.

The source uses raw partTPAGE without the unrelated class20OR20 rule. Bits5..6 select the blend operation. For textured semitransparent primitives, only sourceSTP texels blend; ordinary nonzero texels remain opaque, and colorzero is discarded. ABR0 averages background/foreground,1 adds,2 subtracts foreground,3 adds a quarter foreground. These GPU contracts are documented in the [PSX GPU polygon fields](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#gpu-render-polygon-commands) and [semitransparency/TPAGE reference](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#semi-transparency).

`plan_scene_e0_material()` extends the existing E0 material module rather than replacing the renderer. An opaque record keeps its one-pass contract. An ABE record requires the independent alpha gate, a current owner texture retaining alpha0/128/255, and NEAREST sampling. It produces two passes for this same mesh:

1. Opaque texels: alpha greater than128, replacement blending.
2. STP texels: alpha equal to128, the rawTPAGE ABR equation.

Both semitransparent-record passes use alphaTEVscale1 and vertex alpha1 so the STP marker is preserved. RGB uses the existing sourceRGB/256 with RGBTEVscale2. Opaque textured records retain alphaTEVscale2 to treat every nonzero source texel as opaque. Never apply an arbitrary per-object opacity or render an unavailable texturedSTP mask as though every texel were semitransparent. The helper returns zero passes on missing gate/mask/filter premises.

`scene_e0_texel_in_pass()` admits only retained alpha0/128/255. Tests reject every other byte value as an unsupported alpha encoding rather than synthesizing STP from filtered values. Existing uploads use GPU_RGBA8 and NEAREST; no new texture, shader or source palette is required. The local inherited seven-material snapshot has six resolved pages containing332687STP and60529opaque texels, with no rawTPAGE/metadata ABR mismatch. This is an initial-upload snapshot audit, not a current-frame liveVRAM claim.

Backend mapping uses One/One addition or destination-minus-source for ABR1/2. ABR0/3 derive Half/Quarter factors from the source equations; nearest8-bit PICA constants128/64 are approximations whose rounding/clamping must remain explicit. They are not proof of PS1 RGB5 raster equivalence. Reset blend, alpha-test and depth/write state after every E0 draw. Keep each record's two passes adjacent and preserve the existing record traversal; do not introduce a global opaque/translucent regrouping.

## Remaining source boundaries

Original packetCLUT insertion also ORs scratch1F8000E0 into the firstUV word at8002A8D8/8002AB10. Its live owner/value is not retained here. The current exact-identity initial snapshot remains a diagnostic binding; no alternate palette is borrowed. Original DPCS is conditional on source flags, while original projection, camera-space depth, culling and shade ownership are still separate.

The unchanged scalar ordering slice8002A880..8AC prepends each packet into an original depth-derived ordering-table bucket, so packets sharing a bucket are linked in reverse insertion order. The source fragment has no native depth-buffer equivalence. Two masked PICA passes under the current native depth test are a candidate presentation improvement, not a claim that original sorting or intersecting transparent geometry is reproduced.

## Verification

`source_e0_material_probe.py` ran256 authored packet/material cases plus16 ordering cases, executing11872 unchanged original instructions. It checks original mask/opcodes, rawTPAGE insertion, explicit authoredCLUT-OR inputs, and same-bucket linked-list order. No original bytes are exported; no GTE/GPU execution, helper hook, instruction patch or save access occurs. The authored fixture matches the native helper in all256 cases.

The extended `test_scene_e0_material` passes768 mask-class checks, all fourABR plans, gate/mask/nearest holds, STP-black preservation and inherited package identity/CRC/budget/malformed checks. Current module/test compiled with hostClang warnings as errors; the isolated test linked the unchanged frozen01438 support archive while another module was being edited. Actual devkitARM GCC16.1 ARMv6K module compilation with warnings as errors passed. Full current-core/native/emulator results belong to the enclosing pass.

```sh
python3 -B docs/research_01439/source_e0_material_probe.py --exe /ABS/ROCK_NEO.EXE --out /NEW/ABS/e0-material-oracle
test_scene_e0_material /ABS/e0-material-oracle/source-e0-material.tsv
```

`source-e0-material-proof.json` retains scalar counts, hashes and limits. The local owner snapshot and code/texture payloads remain outside the code-only release. No physical Old3DS run or complete-game parity is claimed.

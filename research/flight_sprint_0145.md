# Flight Sprint 0.0.14.5 — GPU Texture Safety / PICA Layout

Real Old 3DS testing found a hard black-screen regression in 0.0.14.4 after the first textured renderer was introduced; 0.0.14.3 remained hardware-stable with the same cooked owner data.

## Correctness fixes
- Convert linear owner-derived RGBA pages to the actual PICA200 `GPU_RGBA8` layout before upload: 8x8 Morton-tiled texels, byte order A/B/G/R.
- Flush uploaded linear texture memory with `C3D_TexFlush` before GPU sampling.
- Add boot recovery: hold SELECT while launching to disable every texture upload and use the known-working untextured path.
- Preserve 256 KiB stack and current libctru compatibility fixes.

## Evidence boundary
Host tests prove the layout transform itself. Real Old 3DS hardware must still validate the Citro3D path and visual orientation. If textured mode still fails, safe mode is designed to preserve a bootable diagnostic path.

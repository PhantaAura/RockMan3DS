# Source color and depth cue audit

Status: **SOURCE_BACKED_PARTIAL**. The shader audit found no missing global
texture gain correction in the current player or neutral terrain sample.
It did find an unsupported-color-format admission in L3MD, now rejected.

The PS1 GPU combines a modulated texel with its primitive color using
`texel * color / 128`; raw textured commands ignore that color. Gouraud adds
per-vertex color words, including when raw texturing makes their values unused.
These are distinct packet and shading contracts. The references are the
[GPU polygon flags](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#gpu-render-polygon-commands),
[GPU modulation](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#modulation-also-known-as-texture-blending),
[GTE color commands](https://psx-spx.consoledev.net/geometrytransformationenginegte/#gte-color-calculation-commands),
and the installed SDK's matching
[Citro3D texture environment implementation](https://github.com/devkitPro/citro3d/blob/master/include/c3d/texenv.h).

## Current native draws

- Enhanced player draws use source RGB divided by256 and RGB TEV scale2.
  This produces the required `/128` gain. Raw textured codes use0.5 input
  with scale2, preserving their texels independently of source RGB. Alpha/STP
  scaling is separate. `configure_texenv_uv` resets the environment before
  every draw contract, so that scale does not propagate to later terrain draws.
- Enhanced terrain uses fixed white with scale1. All2502 primitive prefixes
  in the current retained-byte L3RG sample have neutralRGB808080. Fixed white
  therefore matches their *unfogged* neutral modulation. This does not establish
  original depth shading, draw order, clipping, semitransparency or raster parity.
  A global scale2 change would incorrectly brighten these diagnostic texels.
- Existing class20 actor drawing is explicitly a white unlit inspection path.
  Its neutral decoded RGB is not recovered original actor lighting. Scale1
  with white is a diagnostic unity gain, not evidence of shaded actor parity.
- The UV shader forwards color and transforms positions. It does not implement
  the original GTE depth-cue color calculation.
- The new E0 view retains its actual work+40 RGB and source ABE bit. For a
  textured diagnostic, source24/2C colors require the same `/128` gain as player
  drawing. For an explicitly untextured inspection, RGB/255 and scale1 display
  the retained scalar; that display is not an original FT3/FT4 raster result.
  Source flag08 skips the depth-shade write; without that flag, retained RGB is
  only the input to the missing shade consumer. An untextured fullscreen source
  subtraction tile also uses ordinary RGB/255, independently of texture gain.

## Packet format fix

The raw player PBD importer already accepts flat textured24/2C packets only.
L3MDv1 also stores just one RGB triplet per face, but its cooker and runtime
reader previously admitted Gouraud30/38/34/3C opcodes without any per-vertex
colors. That could silently treat unsupported data as flat geometry.

The cooker and reader now require flat triangle20..27 or flat quad28..2F
opcodes. Flat untextured forms and raw/semitransparency bits remain accepted.
Gouraud forms, unrelated commands, and triangle/quad opcode mismatches reject
even when a payload has a valid CRC or CRC checking is disabled. This does not
add Gouraud support or change the existing owner's player packets.

## Proven original scalar owners

`8007B0F8` shifts each argument left4 and writes GTE control21/22/23, FarColor.
`8007B0D8` performs the same scalar shift into control13/14/15, BackColor.
The two register owners are deliberately represented by different native types.

The original40-byte area configuration writer `800371A0` loads unsigned bytes
at+32/+33/+34 and invokes FarColor. Its call does not modify BackColor. Supported
ST00 caller `80100528..80100564` chooses `8011AC54 + area*40`; no later live
writer or current-frame ownership is inferred. Actual owner scalars stay in the
private proof. They were measured from that writer, not replaced with an assumed
black fog color.

The positive terrain flat constructor `800251D4..800251EC` preserves the
primitive RGB24 and source bit25, then installs2C. Source prefix metadata is
therefore not a raw-texture or Gouraud GP0 opcode. Conditional consumer
`800252F4..80025330` uses DPCS0780010 and writes the resulting RGB/code word.
Its source branch eligibility, per-primitive IR0 and projection chronology must
remain independently established before enabling runtime depth cue.

`source_depth_cue.h` provides the leaf setter scalars, the proved flat textured
color constructor and a bounded DPCS0780010 calculation. DPCS admits only
FarColor controls derived from unsigned owner bytes and IR0 within0..4096;
invalid values leave the output untouched. Within that domain, signed IR/MAC
saturation is unreachable and the final byte conversion retains arithmetic
floor rounding, including descending blends. The source owner and known-state
gate remain the caller's responsibility; numeric zero is not proof of ownership.

This helper does **not** bind the native renderer. Original H, camera matrix,
transformed depth, GTE division/IR0, opcode eligibility and current FarColor
ownership are still required. Original DPCS modifies a primitive's modulation
color before texture sampling; a generic post-texture GPU fog equation cannot
be substituted without proving equivalence. No shader fog was enabled here.

## Verification

The bounded oracle executed37580 unchanged original instructions with zero
helper hooks and zero instruction patches:128 FarColor setters,128 BackColor
setters,80 authored aligned/unaligned area writers,5 actual owner area writers,
128 flat packet constructions and256 DPCS call-site cases. DPCS itself uses an
explicit restricted arithmetic model from the referenced GTE specification.
These are CPU ownership comparisons and modeled arithmetic, not an independent
physical GTE test, live retail parity or visual acceptance.

The release fixture is authored data only. Its SHA256 is
`f8a79f345648ce71ae99b14eec8fdb6a87c9d0667c1ab989ccff80aeef70a833`.
The public proof contains source hashes, aggregate counts and limits; original
config records, vertices, textures, executable instructions and owner scalars
are excluded. Reproduce in a new private directory:

```sh
python3 -B docs/research_01430/render_color_source_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE --st00 /path/to/owner/ST00.BIN \
  --render /path/to/owner/st00.l3render --out /new/private/render-color
./test_source_render_color tests/fixtures/source_render_color_01430.tsv
```

Host test compilation passed with `-Wall -Wextra -Wpedantic -Werror`. The fixture
comparison passed, together with252 independently weighted scalar interpolation
cases,64 flat-opcode round trips,20 malformed opcode cook/load checks and atomic
invalid-domain holds. The five current local player component packages also
passed the guarded runtime reader. Native build/emulator results belong to the
parent pass report; no physical Old3DS validation happened in this audit.

The helper uses fixed arrays and no allocations. Opcode validation occurs at
cook/load time. Terrain VBO layout, geometry budget, draw calls, per-frame
allocations, safe0143 shader and renderer feature gates are unchanged.

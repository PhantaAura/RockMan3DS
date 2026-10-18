# Tutorial flow checkpoint 0.0.5

This report contains only derived structure/measurements. It does not contain Capcom dialogue, audio, images, models, maps, or other game payloads.

## MSG / dialogue transport

The two ST00 message banks are now parsed by the native host core and can be cooked into pointer-free `L3MS` runtime files.

Measured on the user-owned USA data:

| bank | blocks | glyph bytes | newline bytes | controls | unknown high bytes | AUDIO | WAIT | NEXT | SEL | MULTI | CAM |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| st00_01 | 15 | 930 | 41 | 156 | 6 | 21 | 16 | 13 | 4 | 2 | 0 |
| st00_02 | 21 | 1902 | 80 | 281 | 15 | 28 | 33 | 41 | 10 | 5 | 0 |

`AUDIO` has four argument bytes. In this tutorial pair, bytes 1..3 are zero and byte 0 covers the contiguous ID range 0..45 across the two banks; st00_01 repeats IDs 7..9. This is strong evidence for an indexed tutorial audio/voice command stream, but the final mapping from those indices to XA sectors/files is not yet assigned.

Neither tutorial MSG bank contains opcode `A5` (`CAM` in the community MSG decoder), so the tutorial camera choreography must not be assumed to live in these two dialogue bytecode files. Stage/script code remains the camera source to trace.

Cooked sizes during validation:

- st00_01.l3msg: 9,614 bytes
- st00_02.l3msg: 19,079 bytes

## Player pose bridge

A backend-neutral float matrix bridge is now implemented for all 15 transformed player parts.

Evidence already established by the USA executable/data:

- 40 packed 12-bit animation channels per 60-byte frame.
- channel-to-part rotation mapping for transform IDs 0..14.
- 15-part parent hierarchy.
- action-0 base/local translation vectors.
- part 0 uses the animated root translation path.

Rotation construction uses X * Y * Z. This order is independently consistent with the public PSn00bSDK `RotMatrix` implementation. The 3DS bridge intentionally uses float matrices; PS1 Q12 rounding/overflow is not claimed to be bit-identical.

## Stage runtime object records

A safe raw parser now exists for the 20-byte list format consumed by the common stage-object setup routine. It stops only on the observed `0xFF` record sentinel and preserves every source byte.

The parser exposes, without semantic guessing:

- byte +0x00 ID field
- byte +0x01 class field
- five raw little-endian words covering +0x02..+0x0B
- signed position-like values at +0x0C/+0x0E/+0x10
- raw tail word at +0x12

Enemy/boss names are deliberately not assigned until per-area lists are linked to EBD behavior.

## Local tutorial-pack cooker

`tools/build_tutorial_pack.py` now performs an end-to-end local conversion from an extracted user-owned `CDDATA/DAT` directory to the pointer-free tutorial runtime package.

Core validation generated:

- ST00 L3MP map
- ST00 L3RG render graph
- five baseline player L3MD component models
- L3PC player composition
- main/right/left L3AN animation banks
- L3SK skeleton
- both tutorial L3MS message banks

With `--with-indexed`, it also successfully cooked all 69 type-1/type-4/type-9 indexed resources found in the tutorial-loaded container set used by this checkpoint. The resulting local test pack contained 84 files including its generated manifest. Generated cooked assets are user data and are never included in the source archive.

The existing material audit remains 388/393 references complete. Five references (three unique material words) still require the runtime palette-copy trace rather than a guessed cross-stage CLUT match.

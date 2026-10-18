# Flight Sprint 0.0.14.3 — Old 3DS stack-safety fix

## Hardware evidence
Real Old 3DS XL boot of 0.0.14.2 reached an ARM11 data abort with a write fault.
The captured exception reported SP=0x07FFAB28 and FAR=0x07FFAB18, a write 16 bytes below SP.
That pattern is consistent with app-stack exhaustion.

## Root cause
The hardware-probe `main()` owns large by-value runtime objects, notably:
- `ReferenceTables`: 16,384 bytes on the host ABI
- `RuntimeSession`: 19,288 bytes on the host ABI

Those two alone exceed ~35 KiB before the rest of `main()` locals/call frames.
The prior build did not declare an explicit libctru application stack size.

## Fix
Declare `u32 __stacksize__ = 256 * 1024;` at global scope. This reserves a conservative
256 KiB application stack while leaving linear GPU allocations unchanged.

This checkpoint also preserves the current-libctru fixes from 0.0.14.2 for
`C3D_TexEnvSrc` and `DVLB_ParseFile`.

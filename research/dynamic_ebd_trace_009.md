# Dynamic EBD selector evidence — 0.0.9

This checkpoint continues the disc-first rule from 0.0.8. All addresses below were checked against the exact user-owned USA `ROCK_NEO.EXE` accepted by `verify_mml_us_reference.py`; memory is not evidence.

## Exact initializer selector branch

The routine at USA executable `0x80047318` dispatches on runtime work byte `+0x08`. Its state-0 initializer at `0x80047354` chooses an optional EBD directory variant selector before calling the common lookup at `0x8002F9C4`.

The observed selector policy is:

1. if runtime work byte `+3` has bit `0x80`, selector = `0`;
2. otherwise, if it has bit `0x20`, selector = `2`, then two class-`0x80` helper work slots are allocated through `0x80031618` and seeded with byte `+3` values `5` and `6`;
3. otherwise, low nibble `4`, `5`, or `6` is passed directly as selector `4`, `5`, or `6`;
4. otherwise `0x8004920C(work+0x38)` is called and its signed result is compared with signed work halfword `+0x3A`: `(probe - work_3a) < 30` selects `0`, else `1`.

Branch precedence matters: `0x80` wins over `0x20` and low-nibble cases.

`ebd_dynamic_selector` translates only this proved selector decision into portable code. It does **not** claim that every class-E0/subtype-08 object is owned by this handler. That dispatcher mapping remains a separate proof obligation.

## Six tutorial dynamic records

The six area-2 source records that remained ambiguous in 0.0.7 carry byte-3 values:

`61, 81, C1, 60, A0, E0`.

If — and only if — the exact handler mapping later proves these records use `0x80047318`, the selector branch would produce:

| source byte +3 | selector branch |
|---:|---|
| `61` | selector `2` + helper pair |
| `81` | selector `0` |
| `C1` | selector `0` |
| `60` | selector `2` + helper pair |
| `A0` | selector `0` |
| `E0` | selector `0` |

The local area-2 EBD directory itself has only class-E0/subtype-08 variants `0` and `1`. Therefore a selector-2 result is evidence of a different lookup/side-effect role, not permission to invent a missing “variant 2” asset.

## E0 pool/update cross-check

The common parser at `0x8002EA50` allocates class E0 through `0x80031734` and copies the observed 20-byte stage-record fields into its 0x50-byte work slot exactly as mirrored by `stage_runtime`.

The executable also contains a 32-slot E0 pool walk using base `0x800BF118` and 0x50-byte stride. This supports keeping the native compatibility work mirror byte-addressable while behavior translation proceeds incrementally.

## Current boundary

This is a verified behavior-selector bridge, not complete enemy AI and not permission to bind the six records speculatively. Next proof target: identify the exact dispatcher ownership for the six area-2 class-E0/subtype-08 work slots, then integrate the selector and helper-spawn side effects into per-frame native behavior.

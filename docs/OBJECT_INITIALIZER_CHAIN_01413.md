# Object initializer chain — 0.0.14.13

**SOURCE_BACKED_PARTIAL.** The first imported tutorial record has a proved script initializer. A separate class-20 record has a proved initializer and EBD resource assignment chain. These are static traces of the supplied USA executable and ST00 overlay, checked against selected owner-data pointers and hashes. They are not evidence that either imported object has been executed or drawn by the native port. The original emulator stayed paused before the first encounter.

Evidence: [machine-readable chain](validation_01413/object-initializer-chain.json) and [reproduction probe](research_01413/object_initializer_chain_probe.py). The probe emits selected addresses, scalar selectors and hashes; it emits no original code, asset bytes or original function tables.

## First imported record: area 0, index 0

The actual first record is at `0x8011A704`: source identifier `1`, class `C0`, subtype `03`, parameter byte `+3 = 0`. It is not a class-00 player record. The five lists selected through `0x8011AB3C` contain `[1,16,20,11,1]` records.

1. Parser `0x8002EA50`, class-C0 call site `0x8002ED6C`, calls allocator `0x800316BC`. The pool is `0x800A2D08`, 16 slots of `0x24` bytes. A fresh cleared pool assigns this record slot 0. Pool initialization at `0x8002E980..0x8002E9CC` supplies `work[1] = C0 | slot`; allocation sets `work[6] = 3` and clears the state word at `+8`. Parser copies identifier `1` to `work[0]`, subtype/parameter to `+2/+3`, and the compact record tail to `+0x14..+0x1B`.
2. Update walker `0x80017478` accepts `(work[0] & work[6] & 1) != 0`. This fresh record passes. This is a different gate from the previously recorded E0 **draw** gate using bit 2.
3. ST00 installation at `0x80100364..0x80100370` writes table `0x8011A644` to scratch pointer `0x1F800068`. The C0 walker indexes it by `work[2] * 4`; subtype 3 selects entry `0x8011A650 → 0x80119398`.
4. Callback `0x80119398` indexes state table `0x8011CF48` by `work[8] * 4`. Fresh state 0 selects **initializer `0x801193D4`**.
5. The initializer indexes script-pointer table `0x8011CF28` by `work[3] * 4`. Parameter 0 selects **script `0x8011CEC8`**, stored at work `+0x1C/+0x20`. It uses a script u16 index to form a command-storage pointer `0x800AD168 + 2 * index` at work `+0x18`, then calls script-step helper `0x8011974C`. The measured initial index is 4672; the first advance uses 4688. The corresponding pointers are `0x800AF5E8` and `0x800AF608`. The initializer sets work state `+8` to 1 and has a conditional call to C0 release helper `0x80031988` when the script-step result is nonzero.

There is **no EBD lookup in this initializer** and no `C0/03` row in the loaded area-0 EBD. That is not evidence of a missing model: the reached path is a script/command path. The command-storage consumer and any presentation consequences remain undecoded. No geometry, object name or gameplay role is assigned to this record.

## Separate first unique class-20 record: actor record A

Area 2, record 18, at `0x8011A9E8` has class `20`, subtype `35`, parameter `+3 = 0`, dispatch byte `+4 = 0`. Record 19 shares those keys. This report follows record 18 only, with a neutral label.

1. Parser call site `0x8002EC14` reaches allocator `0x80031418`: pool `0x8009F108`, 16 slots of `0x3C0` bytes. Record 18 is the first class-20 record in this list, so a fresh cleared pool assigns slot 0. Pool initialization supplies `work[1] = 20 | slot`. Fresh `work[0]=1`, `work[6]=3`, `work[8]=0` pass the class-20 update gate at `0x80016FD4`.
2. ST00 installation writes `0x8011A4D0` to `0x1F800058`. The walker performs **two** lookups: subtype `35` selects `0x8011A5A4 → 0x8011A4A4`; work byte `+4 = 0` then selects `0x8011A4A4 → 0x801068A8`.
3. Callback `0x801068A8` indexes `0x8011C020` by work byte `+3`. Parameter 0 selects `0x80106B10`. That routine indexes its state jump table `0x801000D4` by work byte `+8`; fresh state 0 selects block `0x80106B5C`, which directly calls **initializer `0x80106974`**. This extra state step is necessary; adjacency of the initializer alone would not prove ownership.
4. Initializer call `0x801069A0` invokes EBD wrapper `0x8002F9C4` with `a0=work`, `a1=0` (**null variant selector**). The wrapper calls lookup `0x8002FA38` using loaded base `0x8016C000`. The lookup normalizes work byte `+1`, compares subtype at work `+2`, and accepts the first class/subtype row because the variant pointer is null. It does not use raw byte `+3` as an EBD variant for this actor.
5. ST00_02's EBD is byte-identical to ST00_01's, 117,360 bytes, SHA256 `8c1dd36fbd7b5e3a48b1b45be0c232f39f8e6c9e19245f3f7de93b2905ba4ba1`. There is exactly one `20/35` row: **row 3**, variant 0, extra byte 0.

The class-20 branch of lookup `0x8002FA38` consumes that row as follows. Slot names below are directory order, not inferred model/animation names.

| EBD pointer slot | Offset from EBD base | Proved use |
|---|---:|---|
| 0 | `0x6AB4` | Pointer stored at work `+0x88`; 24 bytes from pointer `+0x10` copied to work `+0xF8` through helper `0x80017684` |
| 1 | `0x7224` | First u32 stored at work `+0x9C`; pointer `+4` stored at work `+0xA0` |
| 2 | `0x7528` | Pointer stored at work `+0x90`; pointed format not decoded here |

Hashes of the exact 24-byte slot-0 consumed window and the slot-1 first u32 are in the JSON evidence. These windows are not copied into the source. The caller does not check the lookup return before continuing its remaining initializer stores; `0x801069D8..0x801069EC` subsequently installs overlay pointers at work `+0x78` and `+0x10`.

## Remaining boundary

The prior `0x80089A78 → 0x80047318` pointer anchor is not the active ST00 dispatch table proved above. It should not be assigned to these tutorial objects by consecutive pointer position.

The 0.0.14.13 EBD v2 retention work makes pointed source bytes available; the older 0.0.14.11 directory-only retention blocker is historical. Byte availability still does not decode the three resources or establish native actor presentation. Next proof needed: bounded resource format/consumer decoding for the above actual fields, initialized actor transforms/animation and the native draw binding. The C0 record separately needs its command consumer traced. No enemy AI, object identity, full initializer translation or object-render parity is claimed.

Reproduce from the project workspace, using owner-local inputs:

```sh
python3 work/Legends3DS_0.0.14.13_ObjectResources/docs/research_01413/object_initializer_chain_probe.py \
  --exe owner-data/extracted/ROCK_NEO.EXE \
  --dat owner-data/extracted/CDDATA/DAT \
  --out work/Legends3DS_0.0.14.13_ObjectResources/docs/validation_01413/object-initializer-chain.json
```

The exact EXE SHA256 is `10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`. The **overlay payload** SHA256 is `9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`; this is not the containing ST00.BIN hash. The JSON records both hashes separately and the code-region hashes supporting the trace.

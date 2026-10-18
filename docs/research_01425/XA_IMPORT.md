# Owner-local full-sector XA importer — 0.0.14.25

Status: **SOURCE_BACKED_PARTIAL**. `tools/cook_xa_stream.py` reads the authenticated owner CUE/BIN and command metadata, filters the requested full XA sectors, and emits a bounded PCM package. Audio decoding and package integrity are exercised. This does not establish the PlayStation hardware decoder's quantization, resampling, prior-session predictor state, retail CD chronology, or Old 3DS output parity.

The input files remain read-only. The importer refuses audio output inside the source tree or any synced `sources/` directory. The actual package is private at `work/pass-20261010-worldalive-01425/private/audio/stream_245.l3xa`, outside this cumulative source checkpoint. No owner sectors, sound groups, PCM, or original samples are in the source release. The committed tests synthesize their own sectors.

## Source and disc admission

The supported executable is 825344 bytes, SHA-256 `10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`. The importer reuses the already established request `20000003` lookup: bank/entry tables at `80088F9C` and `80088EF8`, request pair index 3, then the file record at `80082CD0 + id*12`. It reads the packet and file metadata from the hash-guarded LOAD rather than emitting a retail table into native code. This request yields id 245, file LBA 91731, file length 55760 full Form2 sectors, offset 0, end offset 15584, file filter 1, channel 3. Only this measured request is currently admitted.

The supported full BIN is 524601840 bytes and SHA-256 `b544b9f09f8d2fca435c433069da8726ec06eca5f131bd1599598f0d6f1ed908`. The CUE must describe one sibling binary file, track 1 `MODE2/2352`, track 1 index 1 at `00:00:00`, track 2 AUDIO index 0 at `46:01:70`, and index 1 at `46:03:70`. Relative traversal, absolute paths, additional tracks, 2048 projections, other sector widths, offsets, image sizes, and image hashes are rejected. File and selected span bounds must remain inside the data track and source file.

The complete source interval is **91731 ≤ LBA < 107315**, 15584 physical sectors. Every sector's sync, BCD MSF address (`LBA+150`), Mode2 byte, and both four-byte subheaders are inspected. The filtered audio must occur at `91734 + i*8` for `0 ≤ i < 1948`. Nonselected sectors do not advance the selected decoder's history. Unsupported channel/coding/submode combinations fail the import; there is no fallback interpretation of an extracted 2048-byte file.

## Full sector findings

The selected channel has 1947 sectors with submode 64 and one terminal sector with submode E4 at LBA 107310. All selected sectors have file1/channel 3/coding 01: 4-bit stereo at 37800 Hz, emphasis off. Each selected sector contains 18 sound groups and 2016 stereo PCM frames, yielding **3927168 frames**, **103.893333 seconds**, and 15708672 PCM bytes. Eight nonaudio fillers in the full physical span have file 1/channel 0/submode 0/coding 0. Other channels can end and restart within the interval; their histories are never decoded or combined with channel 3.

The wider audit corrects the first64-header-only observation from 0.0.14.24: nine terminal audio sectors do **not** have identical duplicate subheaders. Their first copy is `01 cc E4 01`; their second copy is `01 (cc|80) 64 01`. The nine LBAs are 96548, 98249, 98744, 103388, 105444, 105837, 105921, 105984, 107310. This exact terminal pattern is admitted only after the complete BIN hash matches the supported owner image. Every other mismatch is rejected. The first copy supplies the selected filter/coding fields. This is a measured owner-profile exception, not a general corruption repair rule or a claim about how retail hardware resolves disagreeing copies.

All 15575 remaining duplicate subheaders match. All selected sound-parameter duplicates match, their predictor filters are 0..3 and ranges 0..12, and each audio sector's 20 padding bytes are zero. Its four EDC bytes are also zero. Zero EDC is treated as omitted; **EDC validation is not claimed**. A nonzero unverified audio EDC is rejected in this focused profile. Full-span hashes still authenticate every sector byte.

## Decoder basis and exact local arithmetic

The independent primary format reference is Philips' **CD-I Full Functional Specification, May 1994 release 2**, Chapter IV, specifically IV.3.3/IV.3.6, IV.4.3, and IV.5.3. The [primary document mirror](https://www.icdia.co.uk/docs/cdi_may94_r2.pdf) and the [transcription of that primary chapter](https://github.com/meeq/cd-i-n64/blob/main/distilled/greenbook/Chapter-IV-Audio-Data-Representations.md) are references; no third-party decoder implementation was copied. The PDF mirror did not fetch in the browsing service; the chapter transcription supplied the exact layout, rational gains, scale and clipping description. [Sony's hardware reference](https://psx.arthus.net/sdk/Psy-Q/DOCS/Devrefs/Hardware.pdf), Sound System/CD-ROM Decoder, independently identifies the disc XA decoder path.

The sound-group parameter layout is `p0 p1 p2 p3 p0 p1 p2 p3 p4 p5 p6 p7 p4 p5 p6 p7`. Twenty-eight sample rows each contain four bytes; each byte's low nibble belongs to the left sound unit and high nibble to the right. Pairs of units play sequentially. The gain numerators over 64 are `(0,0)`, `(60,0)`, `(115,-52)`, `(98,-55)`. Signed input is scaled by `2^(12-range)`; the predictor uses the preceding two samples from the same side and clipped results become subsequent history.

The local quantizer is specified exactly as `(gain0*newer + gain1*older + 32)//64`, followed by addition of the scaled input and signed16 saturation. Division uses floor semantics, so half-way values round toward positive infinity. The primary decoder model leaves this integer rounding choice unstated. The package and proof therefore identify the arithmetic and **do not label it hardware bit-exact**. Independent FFmpeg comparison has not been performed: no existing FFmpeg executable was available, and no dependencies were installed. An optional authored comparison test runs when `ffmpeg` is already in PATH.

The first decoder state is explicitly `[[0,0],[0,0]]`; the package flag identifies this initialization. History then persists across sound units, sound groups and selected sectors. The measured final history is also zero. Cold-zero cooking does not establish what history retail CD hardware retains across a seek, pause, loop, reset, continued session or save restore. The package contains already decoded PCM, so seeking a native package never reinitializes ADPCM history mid-stream. Native seek mapping remains within this particular cooking result.

Mono, 18900 Hz, 8-bit XA, emphasis, reserved coding, other images/requests, and arbitrary subheader discrepancies are explicit unsupported modes. No PlayStation 44100 Hz interpolation, emphasis filter or emulator is added.

## L3XA v1 package contract

All numeric values are little-endian. Header format is Python `'<8s12I5Q8I32s32s32s32s'`, exactly 256 bytes. Index format is `'<IIQII'`, exactly 24 bytes. The package has no trailing data: header, selected-sector index, then interleaved signed16 left/right PCM. Capacity is 4096 selected sectors. Sizes/offsets are redundant for strict reader validation.

| Offset | Type | Value or meaning |
| ---: | --- | --- |
| 0 | 8 bytes | `L3XA001\0` |
| 8 | u32 | Header bytes 256 |
| 12 | u32 | Version 1 |
| 16 | u32 | Flags 1: explicit cold-zero decoder history |
| 20 | u32 | Source stream id 245 |
| 24 | u32 | Source start LBA 91731 |
| 28 | u32 | Source end LBA 107315, exclusive |
| 32 | u32 | PCM sample rate 37800 |
| 36 | u32 | Channels 2 |
| 40 | u32 | PCM bits 16 |
| 44 | u32 | Original XA coding 01 |
| 48 | u32 | Selected sector count 1948 |
| 52 | u32 | Index entry bytes 24 |
| 56 | u64 | Total stereo frames 3927168 |
| 64 | u64 | Index offset 256 |
| 72 | u64 | PCM offset 47008 |
| 80 | u64 | PCM bytes 15708672 |
| 88 | u64 | Package bytes 15755680 |
| 96 | u32 | Header CRC32 |
| 100 | u32 | Complete index CRC32 |
| 104 | u32 | Complete PCM CRC32 |
| 108 | u32 | Complete 15584-sector raw 2352 span CRC32 |
| 112 | u32 | File filter 1 |
| 116 | u32 | Channel filter 3 |
| 120 | u32 | Physical interleave stride 8 |
| 124 | u32 | Reserved 0 |
| 128 | 32 bytes | Raw full2352 span SHA-256 |
| 160 | 32 bytes | Full owner BIN SHA-256 |
| 192 | 32 bytes | Complete PCM SHA-256 |
| 224 | 32 bytes | Supported source executable SHA-256 |

Header CRC32 is standard IEEE/zlib CRC32 over all 256 bytes with bytes 96..99 set to zero. Other CRC32 fields are zlib CRC32 over their complete named regions, with initial value 0. SHA-256 fields contain raw 32-byte digests. The proof additionally includes the concatenated 24-byte sector-header digest/CRC, CUE digest, and final package SHA-256; those do not consume native PCM memory.

| Index offset | Type | Meaning |
| ---: | --- | --- |
| 0 | u32 | Physical source LBA for this selected audio sector |
| 4 | u32 | Stereo frame count 2016 |
| 8 | u64 | First PCM stereo frame `i*2016` |
| 16 | u32 | CRC32 of this 8064-byte PCM sector |
| 20 | u32 | Reserved 0 |

For a frame within index entry `i`, file position is `pcm_offset + 4*frame`. The physical LBA labels the source sector which produced those frames. A final entry ends at PCM EOF; source end 107315 is retained separately because physical channels continue through the command's end-exclusive boundary. No timer-created location or drive-status bit is stored in this package. Native queued and played buffers can carry these sector labels; only actual backend consumption can supply playback progress.

The importer streams one 2352-byte source sector and one 8064-byte PCM sector at a time and retains the bounded index. It publishes with an exclusive hard link after completing validation/fsync, leaving no partial package on an import failure. Native code can validate all package CRCs with fixed scratch and retain only the index plus its stream buffers; whole-audio preload is unnecessary.

## Validation and reproducibility

`python3 -m unittest discover -s tests -p test_cook_xa_stream.py -v` passes 13 authored tests; one optional FFmpeg comparison is skipped. The tests cover nibble sign, channel order, all unit parameter positions, positive/negative half-way rounding, clipping, predictor continuity across sectors, malformed late sound groups without partial history mutation, short sectors, sync/MSF/Mode2 faults, unsupported modes, padding/EDC faults, strict CUE/size/hash/bounds/filter/interleave guards, the exact EOF-profile exception, output placement, exclusive publication, and header/index/PCM CRC/SHA consistency. An exact `Fraction` oracle independently expands 52 authored groups, comparing 11648 sample values and final history. It is a mathematical oracle for the declared local quantizer, not an external hardware decoder comparison.

Owner import:

```sh
python3 tools/cook_xa_stream.py \
  --cue '/path/to/owner/Mega Man Legends (USA).cue' \
  --exe '/path/to/owner/extracted/ROCK_NEO.EXE' \
  --out '/path/outside/source/private/audio/stream_245.l3xa' \
  --proof '/path/outside/source/xa-import-owner-proof.json'
```

The importer refuses an existing package or proof. The metadata-only measured proof is `docs/validation_01425/xa-import-owner-proof.json`; authored test output is `docs/validation_01425/xa-import-authored-tests.log`. Source-span SHA-256 is `34ed3aa39e9da1837cf4793a72c7462d26de0e37ea8c38dd04f54e9bd18fc9d6`; PCM SHA-256 is `f9484896e8c31ccb86dd4b69deea7201f0eab89f3dc43312bb2341adbd9dd0d6`; package SHA-256 is `ed93324cd6c9dcba3b88e7d825a3c627725f02897c65e957c4d9e40085c52bcd`. The private owner package is not part of the source manifest or release.

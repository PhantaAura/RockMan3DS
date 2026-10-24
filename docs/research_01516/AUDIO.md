# Tutorial audio: effects and Roll's voice (0.0.15.16)

Status: **SOURCE_BACKED_PARTIAL**. Routing, gating, volumes, pitch and the
envelope follow the original code below. The user listened in Azahar and
reports that the effects and Roll's voice sound right. There is no
sample-level comparison with a PS1 capture.

## Roll's voice (XA)
- Message opcode **0x8F AUDIO** (handler 0x800548C4) reads a u32 id and calls
  **0x8001D58C(id)**: bank `(id>>28)&3`, entry `(id>>16)&0xFFF`, index
  `id&0xFFFF` through 0x80088F9C / 0x80088EF8. The pair gives the start offset
  (low 24 bits, channel in bits 24..28) and the end. The file record is
  0x80082CD0 + stream*12.
- The handler sets message bit 0x40000 and waits only until the drive reports
  playback (0x8001D878: 0x80098AB8 & 0x20). Then the text continues alongside the
  voice.
- Tutorial clips 0x00..0x2D are all stream 209 (LBA 49171): file 1, coding
  0x04 (mono, 18900 Hz, 4-bit), 32 channels interleaved. There are 46 clips,
  121.6 s in all.
- The trigger call `0x8001D648(n)` (CD command 7 → 0x8001C824, SeekP) only
  pre-seeks the next clip. Nothing plays there.
- `tools/cook_xa_voice.py` decodes the clips into an L3VB bank (cold-zero
  predictor per clip, the same arithmetic as `cook_xa_stream.py`).
  `VoiceBankReader` + `NativeVoice` play a clip on NDSP channel 1. A new clip
  replaces the old one.

## Sound effects (SPU)
- Producers 0x8001A008 (type 0: id, pan, attenuation) and 0x8001A050 (type 1:
  id, x|y<<16, z) fill the queue. **0x8001A110** consumes it after the main
  update and dispatches by `0x8008241C[id]` byte 0 >> 6 through 0x80082C64. Every
  tutorial id uses handler 2, **0x8001A968**.
- Row: bank tag (byte 0 & 0x3F), program, tone<<4 | priority, (tones-1)<<5 |
  first voice. The tag matches a loaded VAB (DAT resource type 5: slot, tag,
  SPU address, VH size). In the tutorial the banks are:
  - tag 0 `common.vab` (DEMO_DAT/EXIT_SUP, identical copies);
  - tag 1 `t_op01b`, tag 2 `e_op01b`, tag 20 `m_op01b` (all in ST00).
- The VB starts at the next 0x800 sector after the VH, not right after it.
- **Gate 0x8001A834:**
  - A busy voice (key status on) rejects requests whose priority is below its
    stored one (0x80098B48[v]).
  - For type 1 the distance from the player is computed (0x8007A578). At 5000
    or more the request is dropped. On a busy voice of equal priority it is
    also dropped unless the distance is under 3001.
- **Key-on 0x8001A744:**
  - volume = tone vol × 0x81, then 0x8001A274 (below);
  - start = the VAG address;
  - pitch = 0x8001A6DC(min − centre, fine): note table 0x80082C70/0x80082CA0,
    linear by fine/128;
  - ADSR from the tone.
- **Volume 0x8001A274:**
  - Type 0 pans by a1 and scales by (0x80 − a2)/128.
  - Type 1 at distance 0 is full on both sides. Otherwise it pans by
    `|cos(heading − angle)|` (0x8001E83C, 0x8007A230 CORDIC cosine) and scales
    by (5000 − d)/5000.
  - Then both effect volume settings (0x800AD148/0x800AD150) apply.
- Listener: the player work 0x800B51B0 (+0x14/+0x18 x/z, +0x56 heading).
- There is no per-sound key-off. 0x8001B314 keys off voices 15..23 together.
  A loop runs until its voice is re-keyed, as on the SPU.
- Message typing blip: each glyph requests **0x84** (0x8001A008(0x84, 0, 100)
  at 0x80053D74). It is skipped when 0x20000000 is set (0xA1 sets it, 0xA2
  clears it) or 0x40000 is set (a voiced message).
- E0 objects call 0x8001A050(id, this + 0x38):
  - E0/05: 0xA3/0xA0/0xA2;
  - E0/07: 0x119/0x118/0x11E;
  - E0/08: 0xA0/0xA2, the target beep 0x11C and the door pulses 0x11A/0x11B.
  The target beep reloads its timer from rand() (RA 0x80116244): 2 or 18 frames.

## Port
- `tools/cook_sfx_bank.py` → L3SB: the handler-2 rows whose program exists in
  the loaded bank (297 requests, 66 decoded VAG samples, 4 looped).
- `source_sound` (core): the L3SB loader, `consume_source_audio_8001a110`
  (the gate, volumes and key-on list) and `SourceSpuEnvelope` (SPU ADSR at
  44100 Hz).
- `NativeSfx` (3DS):
  - SPU voice v plays on NDSP channel 2 + v % 22;
  - rate = 44100 × pitch / 4096;
  - a looped sample is an attack buffer plus a looping buffer;
  - the envelope is stepped per gameplay tick (1470 samples) into the
    channel mix.

## CANDIDATE / not ported
- Positional atan/cosine are evaluated in floating point.
- The common bank source file (DEMO_DAT vs EXIT_SUP) is a guess; both copies
  are byte-identical.
- Effect volume settings are assumed at 0x7F, mono off.
- SPU reverb, sweep/pitch modulation, noise, the SPU main volume and
  interpolation are not modelled. Voices 22/23 share NDSP channels with voices
  0/1 (BGM), which is not ported.
- Sequenced music (SEP, handlers 0/1) and the CD mixer volumes are not ported.
- The DSP needs `sdmc:/3ds/dspfirm.cdc` from the user's own console.
- The E0/08 door pulse rand() draws are not on the shared stream yet.

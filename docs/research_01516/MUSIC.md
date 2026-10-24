# Tutorial music: the area sequence (0.0.15.16)

Status: **SOURCE_BACKED_PARTIAL**. The request path, the libsnd sequencer, the
voice manager and the reverb preset follow the original code below. The SPU
mix is a software model. There is no sample-level comparison with a PS1
capture.

## What plays
- The ST00 area music is a **sequence (SEP)**, not XA.
  - `CDDATA/XA/XA37S00E/XA37S01E` (streams 245/246, banks 1/2 of 0x8001D58C)
    hold the cutscene music.
  - In ST00 those are started only by events 1–3 (`0x801010C4`, `0x8010194C`,
    `0x80102058`), with ids `0x20000003/2/1`. None of these ids has the loop bit
    0x40000000 that `0x8001C95C` would replay.
  - Message AUDIO ids in ST00 are all bank 0 (Roll's voice).
- **0x80019D18** (area load tail, called from 0x800159C8 / 0x80016400):
  - first the stage hook 0x800823A0[stage];
  - then u16 `0x80082320[stage][area]`: 0xFFFF → 0x8001B33C (stop the
    sequences), otherwise `0x8001A008(id, 0, 0)`.
  - Stage 0 row (0x80082200): `FFFF 0 0 1 FFFF 0` → areas 1/2/5 = request 0,
    area 3 = request 1, areas 0/4 silent. **VERIFIED_1TO1** (table read).
- **0x80019C20** (transition start): when the destination's id is not
  0xFFFF and differs from the current area's, `0x8001D854(0x10)` fades out
  0x800AD152 (0x8001AE6C, −4 per pass). Area 1 ↔ 2 keep request 0, so
  the music continues.
- Request rows 0/1 (`0x8008241C`): `54 00 03 00/01`:
  - handler 1, VAB tag 20 (`m_op01b.vab`, ST00.BIN 0xC2800);
  - SEP tag 0 (`m_op01b.sep`, ST00.BIN 0xF5000, two sequences);
  - slot 0, priority 3, sequence 0/1, no loop bit (l_count 1).
  - The song loops itself with NRPN 20/30 (CC 99), count 127 = forever.
  - Sequence 0: 48 ticks per beat, 150 bpm; at most 14 notes at once. In
    the render the loop wraps about every 51 s.

## Game side (`source_music`)
- **0x8001A110**:
  - 0x8001AEF0 runs first: the fade, then SsSepSetVol(0x800AD14A = 100 ×
    0x800AD152 / 0x7F) per slot;
  - then 0x8001A0A8 copies SsIsEos (+0x14) into 0x80098AFD;
  - then each queued record is dispatched by handler.
  - 0x80098958 bit 0 is set at sound init (0x80019F78), so the pass always
    runs.
- **0x8001AC58** (handler 1):
  - It looks up the VAB tag (0x8001A1FC) and the SEP tag (0x8001A238), then
    uses slot `row[2] >> 4` (0x80098AF8 + slot × 8).
  - If the request repeats the slot's playing id with word +4 = 0, it only
    sets 0x800AD152 = 0x7F.
  - Otherwise, when the slot's priority ≤ the new one, it stops and closes
    the old sequence. Then SsSepOpen (0x8006B664), and
    SsSepPlay(…, 1, l_count = loop bit ? 0 : 1).
- **0x8001B33C**: SsSepStop on every slot, without a close.

## libsnd (decompiled from ROCK_NEO.EXE)
- **Init**: SsSetTickMode(1) → 60 Hz (0x800C3520). SsSetReservedVoice(15) →
  libsnd voices 0..14. The effects keep 15..23.
- **SEP header (0x8006D8AC)**:
  - layout: id, resolution (BE16), tempo (3 bytes, µs), rhythm, size (BE32);
  - bpm = 60e6 / tempo, rounded;
  - fast mode: step = res × bpm × 10 / 3600, rounded at 1800 (0x54);
  - slow mode (0x52) when res × bpm × 10 < 3600.
  - Deltas are stored ×10 (0x8006D7FC).
- **0x8006F744**: subtracts the step per call; at or below 0 it reads events
  until the remaining delta exceeds the step.
- **0x8006FA90**:
  - running status for 0x90/0xB0/0xC0/0xE0/0xFF;
  - note-on with velocity 0 is the note-off (0x80 is not parsed);
  - E0 skips the LSB;
  - any FF xx other than 2F reads three tempo bytes (0x8006D4E8).
- **Controllers (0x8006D2B0)**:
  - 0: VAB id; 7: volume (0x80075464); 10: pan; 11: expression (ProgAtr mvol);
  - 64: damper (+2 on ADSR2 of new notes); 91: reverb depth;
  - 98/99: NRPN, with loop start 20 and loop end 30; 6: data entry / loop
    count; 100/101: RPN; 121: reset.
- **End of track (0x8006F84C)**: repeats while play count < l_count (0 =
  forever). Otherwise it stops and keys off the sequence's voices.
- **Key on (0x80073100)**:
  - velocity × channel volume / 0x7F;
  - every tone of the program whose key range holds the note (0x800753B0);
  - one voice per tone from 0x800738F8: a free voice (state 0 and ENVX 0),
    else the lowest priority at or below the tone's, the quietest envelope,
    then the oldest.
- **Pitch (0x80073B84 / 0x80073C48)**:
  - n = note + 60 − centre;
  - table 0x80090C80[(n % 12) × 16 + fine/8], shifted by n/12 − 5.
  - Pitch bend uses the tone's pbmin/pbmax. They are 0 in m_op01b, so a bend
    below 64 lowers the note by 1/16 semitone (ported as written).
- **Volume (0x80074398 / 0x80074CE4 / 0x80075464)**:
  - v = vel × VAB mvol × 0x3FFF / 0x3F01 × program mvol × tone vol / 0x3F01;
  - × sequence L/R / 0x7F, then three pans (tone, program, channel), then
    squared / 0x3FFF.
- **Flush (0x800725E0, start of each tick)**:
  - samples ENVX into +0x06;
  - a 16-entry ring retires voices whose envelope stayed 0 for 15 ticks;
  - writes volume/pitch/address/ADSR, then key-offs, then key-ons, then the
    reverb mask (tone mode bit 2).

## Reverb
- Stage 0 calls `0x80019F94(0x800896D4[0] = 1)` → entry 0x800821FC =
  `02 40 40 00`.
- 0x8001AEF0 then:
  - calls SsUtSetReverbType(2) and SpuSetReverb(1);
  - 30 passes later, calls SsUtSetReverbDepth(0x40) then SsSetRVol(0x40).
    Both write the reverb output volume, so it ends at 0x4080.
- Preset (libspu table 0x80090988 + 2 × 0x44): "Studio Small", dAPF1=0x33 …
  vRIN=0x8000. Work area 0x80090938[2] = 0xFC18 (0x1F40 bytes).
- Most m_op01b tones have mode 4 (reverb on).

## Port
- `tools/cook_music_bank.py` writes L3MU, kept in the private pack:
  - the area row, the request rows;
  - the raw SEP and VH, the 14 decoded VAGs;
  - the pitch table, and the reverb entry, base and registers.
- `source_music` (core):
  - the game side and libsnd as above;
  - a software SPU for voices 0..14: pitch 0x1000 = 44100 Hz, ADSR per
    sample (SourceSpuEnvelope), volume 0x3FFF = full scale;
  - reverb with the SPU formula at 22050 Hz.
- `NativeMusic` (3DS):
  - The stereo mix streams on NDSP channel 4. NDSP has 24 channels, and the
    24 SPU voices plus XA and Roll's voice do not fit, so the music voices
    are mixed on the CPU.
  - One 735-frame buffer is one 60 Hz tick, kept four ahead. The sequencer
    runs on the audio clock.

## CANDIDATE / gaps
- Interpolation is linear (the SPU uses a 4-tap Gaussian).
- The reverb runs on averaged sample pairs with a held output. The SPU uses a
  39-tap FIR around its 22050 Hz core.
- The output volume is 0 and the work area is cleared on a mode change.
- The tone block index (ProgAtr +8 written by SsVabOpenHead) is taken as the
  program's rank among programs with tones (identity for m_op01b).
- RPN/NRPN data (pitch-bend range writes, vibrato/portamento) and noise
  voices are not ported; m_op01b does not use them.
- The effects (NativeSfx) get no reverb.
- The music pauses while an area load blocks the main loop. On PS1 the
  sequencer keeps running from the VSync interrupt.
- The 0x80019C20 fade has no frames to run during the synchronous native load.
- No PCSX-Redux capture compares tempo, levels or the reverb tail.

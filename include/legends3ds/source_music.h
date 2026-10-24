#pragma once
#include "legends3ds/source_audio_queue.h"
#include "legends3ds/source_sound.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// L3MU1 (tools/cook_music_bank.py): the stage's area music row (0x80082320),
// the handler-1 rows of 0x8008241C it names, the raw SEP and VH, the VB
// decoded to PCM16 at the pitch-0x1000 rate (44100 Hz) and the libsnd pitch
// table 0x80090C80. The bank keeps pointers into the caller's bytes.
struct SourceMusicRequest { uint16_t id = 0; std::array<uint8_t, 4> row{}; };
struct SourceMusicSep { uint8_t tag = 0, sequences = 0; const uint8_t* data = nullptr; uint32_t bytes = 0; };
struct SourceMusicVab { uint8_t tag = 0; const uint8_t* vh = nullptr; uint32_t vh_bytes = 0; uint16_t first_sample = 0, samples = 0; };
struct SourceMusicSample { uint32_t frames = 0, loop_start = 0xFFFFFFFFu; const int16_t* pcm = nullptr; };
struct SourceMusicBank {
    std::array<uint16_t, 192> pitch_table{};
    std::vector<uint16_t> area_music;
    std::vector<SourceMusicRequest> requests;
    std::vector<SourceMusicSep> seps;
    std::vector<SourceMusicVab> vabs;
    std::vector<SourceMusicSample> samples;
    const SourceMusicRequest* find(uint16_t id) const noexcept;
};
// Validates header, table CRC, SEP/VH identities and every sample's PCM CRC.
// `bytes` must stay alive and 2-byte aligned while the bank is used.
SourceMusicBank load_l3music_bank(const uint8_t* bytes, size_t size);

// 0x80019D18 tail: the request the stage's area queues (0xFFFF = 0x8001B33C).
uint16_t source_area_music_80019d18(const SourceMusicBank& bank, size_t area) noexcept;

constexpr size_t kSourceMusicVoices = 15;           // SsSetReservedVoice(0x800AD158 = 15)
constexpr uint32_t kSourceMusicTickRate = 60;       // SsSetTickMode(1), NTSC
constexpr uint32_t kSourceMusicFramesPerTick = 735; // 44100 / 60

// The game's sequence control (0x8001A110 handler 1 = 0x8001AC58, the
// 0x8001AEF0 volumes and fade, 0x8001B33C stop) over a libsnd SEP model
// (SsSeqCalledTbyT, the event handlers 0x8006D2B0.., the voice manager
// 0x80073100/0x800738F8/0x800725E0) and a software SPU mix of voices 0..14.
class SourceMusicPlayer {
public:
    void bind(const SourceMusicBank* bank) noexcept;
    // 0x8001A110 for the records whose request is a cooked handler-1 row,
    // after 0x8001AEF0 and the 0x8001A0A8 SsIsEos pass. Returns how many
    // records it dispatched; the caller still resets the queue.
    size_t consume_8001a110(const SourceAudioQueue& queue) noexcept;
    void stop_all_8001b33c() noexcept;
    // 0x8001D854(0x10) from the 0x80019C20 transition test: fade 0x800AD152 out.
    void request_fade_out_8001d854() noexcept { if (!fade_mode_) fade_mode_ = 1; }
    // One SsSeqCalledTbyT tick: flush the voice registers, then the sequences.
    void tick_60hz() noexcept;
    // Mixes `frames` stereo frames (interleaved L,R) of the music voices.
    void render(int16_t* stereo, size_t frames) noexcept;
    void render_tick(int16_t* stereo) noexcept { tick_60hz(); render(stereo, kSourceMusicFramesPerTick); }

    struct Slot { int16_t access = -1; uint8_t type = 0xFF; int8_t priority = 0; uint8_t sequence = 0, playing = 0; uint16_t request = 0; };
    const Slot& slot(size_t i) const noexcept { return slots_[i]; }
    uint8_t volume_ad152() const noexcept { return volume_ad152_; }
    size_t sounding_voices() const noexcept;
    uint32_t key_ons() const noexcept { return key_ons_; }
    uint32_t loops() const noexcept { return loops_; }
    uint32_t errors() const noexcept { return errors_; }

private:
    struct Seq {             // libsnd sequence attribute (0x800C4CF0[access] + seq*0xB0)
        const uint8_t* ptr = nullptr;      // +0x00
        const uint8_t* start = nullptr;    // +0x04
        const uint8_t* loop_ptr = nullptr; // +0x08
        const uint8_t* end = nullptr;
        uint32_t flags_98 = 0;
        uint8_t playing_14 = 0, loop_active_15 = 0, status_16 = 0, channel_17 = 0;
        uint8_t rpn_18 = 0, rpn_19 = 0, nrpn_1a = 0, nrpn_1b = 0, loop_marker_1c = 0, loop_count_1d = 0;
        uint8_t rpn_count_1e = 0, nrpn_count_1f = 0, l_count_20 = 1, play_count_21 = 0, vab_26 = 0;
        std::array<uint8_t, 16> pan_27{}, program_37{};
        int16_t resolution_50 = 0, slow_52 = 1, step_54 = 0, step_init_56 = 0;
        uint16_t vol_l_58 = 0, vol_r_5a = 0, mute_80 = 0;
        std::array<int16_t, 16> volume_60{};
        int32_t first_delta_84 = 0, total_88 = 0, bpm_8c = 0, remain_90 = 0, bpm_94 = 0;
    };
    struct Voice {           // libsnd voice (0x800A33E0 + v*0x36) and its SPU voice
        uint16_t vag_e0 = 0, age_e2 = 0, pitch_e4 = 0, env_e6 = 0, velocity_e8 = 0;
        uint8_t pan_ea = 0, state_fd = 0;
        uint16_t channel_ec = 0, note_ee = 0, priority_fa = 0;
        int16_t owner_f0 = -1, block_f2 = 0, program_f4 = 0, tone_f6 = 0, vab_f8 = -1;
        uint8_t dirty_3940 = 0;   // 1|2 volume, 4 pitch, 8 address, 0x30 ADSR
        bool key_on = false, key_off = false;
        uint16_t reg_vol_l = 0, reg_vol_r = 0, reg_pitch = 0, reg_adsr1 = 0, reg_adsr2 = 0, reg_sample = 0xFFFF;
        // live SPU voice
        uint16_t vol_l = 0, vol_r = 0, pitch = 0, adsr1 = 0, adsr2 = 0, sample = 0xFFFF;
        uint32_t position = 0, phase = 0;
        bool sounding = false;
        SourceSpuEnvelope envelope;
    };
    struct Note {            // 0x800C0BE0.. scratch of one key-on
        uint8_t tones = 0, note = 0, velocity = 0, pan = 0, prog_vol = 0, prog_pan = 0, block = 0;
        uint8_t tone = 0, tone_vol = 0, tone_pan = 0, center = 0, shift = 0, mode = 0, priority = 0;
        int16_t owner = 0;
        uint8_t vab = 0;
    };

    void handler_8001ac58(const SourceAudioQueueRecord& record) noexcept;
    void volumes_8001aef0() noexcept;
    Seq* seq(int16_t owner) noexcept;
    int16_t sep_open_8006b664(size_t sep, uint8_t vab) noexcept;
    void sep_play_8006e370(int16_t access, uint8_t sequence, uint8_t l_count) noexcept;
    void sep_stop_8006ff90(int16_t owner) noexcept;
    void sep_set_vol_80070694(int16_t owner, uint16_t l, uint16_t r) noexcept;
    void seq_vol_80074ce4(int16_t owner, uint16_t l, uint16_t r) noexcept;
    void snd_play_8006f744(int16_t owner) noexcept;
    void get_seq_data_8006fa90(int16_t owner) noexcept;
    int32_t read_delta_8006d7fc(Seq& s) noexcept;
    uint8_t next(Seq& s) noexcept;
    void end_of_track_8006f84c(int16_t owner) noexcept;
    void tempo_8006d4e8(Seq& s, uint32_t tempo) noexcept;
    void control_change_8006d2b0(int16_t owner, uint8_t control) noexcept;
    void key_on_80073100(int16_t owner, uint8_t vab, int16_t program, uint8_t note, uint8_t velocity, uint8_t pan) noexcept;
    void key_off_80073684(int16_t owner, uint8_t vab, int16_t program, uint8_t note) noexcept;
    void keys_off_80075314(int16_t owner) noexcept;
    void set_volume_80075464(int16_t owner, uint8_t vab, int16_t program, uint16_t volume, uint16_t pan) noexcept;
    void pitch_bend_80074a64(int16_t owner, uint8_t vab, int16_t program, uint8_t bend) noexcept;
    uint8_t allocate_800738f8(uint16_t priority) noexcept;
    uint16_t pitch_80073c48(int32_t note, uint32_t fine, const uint8_t* tone) const noexcept;
    void voice_volume(Voice& v, uint32_t velocity, uint16_t seq_l, uint16_t seq_r, uint8_t channel_pan) noexcept;
    void voice_key_off_800742c8(size_t v) noexcept;
    void flush_800725e0() noexcept;
    void render_voice(Voice& v, size_t frames) noexcept;
    const SourceMusicVab* vab(uint8_t id) const noexcept;
    const uint8_t* program_attr(uint8_t vab, int16_t program) const noexcept;
    int16_t block(uint8_t vab, int16_t program) const noexcept;
    const uint8_t* tone_attr(uint8_t vab, int16_t block, uint16_t tone) const noexcept;

    const SourceMusicBank* bank_ = nullptr;
    std::array<Slot, 4> slots_{};                   // 0x80098AF8 + slot*8
    std::array<bool, 4> access_open_{};             // 0x800BFB18 bits
    std::array<std::array<Seq, 4>, 4> seqs_{};      // SsSetTableSize(.., 4, 4)
    std::array<uint8_t, 4> access_sep_{};
    std::array<Voice, kSourceMusicVoices> voices_{};
    std::array<uint32_t, 16> env_zero_c5640_{};
    uint8_t env_zero_index_ = 0;
    uint8_t volume_ad152_ = 0x7F, fade_mode_ = 0;   // 0x800AD152, 0x80098788 bits 4..5
    uint8_t damper_b5168_ = 0;
    std::array<std::array<uint8_t, 128>, 4> program_volume_{}; // VH ProgAtr.mvol, written by CC 11
    std::array<int32_t, kSourceMusicFramesPerTick * 2> mix_{};
    uint32_t key_ons_ = 0, loops_ = 0, errors_ = 0;
};
} // namespace l3

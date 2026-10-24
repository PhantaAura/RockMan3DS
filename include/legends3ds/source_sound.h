#pragma once
#include "legends3ds/source_audio_queue.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// L3SB1 (tools/cook_sfx_bank.py): the handler-2 rows of the request table
// 0x8008241C whose VAB bank is loaded for the stage, their VAB tones and the
// decoded VAG samples (PCM16 at the pitch-0x1000 rate, 44100 Hz).
struct SourceSoundRequest { uint16_t id = 0; uint8_t voice = 0, tones = 0, priority = 0, tag = 0; uint16_t first_tone = 0; };
struct SourceSoundTone { uint16_t pitch = 0, adsr1 = 0, adsr2 = 0; uint8_t volume = 0; uint16_t sample = 0xFFFF; };
struct SourceSoundSample { uint32_t frames = 0, loop_start = 0xFFFFFFFFu, offset = 0, crc = 0; };
struct SourceSoundBank {
    uint32_t sample_rate = 0, file_bytes = 0;
    std::vector<SourceSoundRequest> requests;
    std::vector<SourceSoundTone> tones;
    std::vector<SourceSoundSample> samples;
    const SourceSoundRequest* find(uint16_t id) const noexcept;
};
// Validates header, tables, CRCs of the table and of every sample's PCM.
SourceSoundBank load_l3sound_bank(const uint8_t* bytes, size_t size);

constexpr size_t kSourceSpuVoices = 24;
// Player work 0x800B51B0 (pointer 0x80098A50): +0x14/+0x18 x/z, +0x56 heading.
struct SourceSoundListener { int16_t x = 0, z = 0; uint16_t heading = 0; };
// Volume settings 0x800AD148/0x800AD150 (0..0x7F) and mono 0x800C1B68.
struct SourceSoundSettings { uint8_t effect_volume_148 = 0x7F, effect_volume_150 = 0x7F; bool mono_c1b68 = false; };
struct SourceSoundVoiceState {
    std::array<uint8_t, kSourceSpuVoices> priority_b48{};
    std::array<uint16_t, kSourceSpuVoices> distance_c1cf0{};
};
struct SourceSoundKeyOn {
    uint8_t voice = 0;
    uint16_t request = 0, pitch = 0, adsr1 = 0, adsr2 = 0, sample = 0xFFFF;
    uint16_t volume_left = 0, volume_right = 0;   // SPU 0..0x3FFF
};
struct SourceSoundConsumeResult { size_t key_ons = 0, unknown = 0, rejected = 0, unsupported_handler = 0; };
// 0x8001A110 for handler 2 (0x8001A968 -> 0x8001A834 gate -> 0x8001A744):
// every queued record in order, then the caller resets the queue (0x8001A1D0).
// `voice_busy` is SpuGetKeyStatus == 1 for the current native voices.
// CANDIDATE: 0x8001E83C atan and 0x8007A230 CORDIC cosine are evaluated in
// floating point (heading convention of the player), 0x8007A578 is an exact
// integer square root.
SourceSoundConsumeResult consume_source_audio_8001a110(const SourceAudioQueue& queue,
    const SourceSoundBank& bank, const SourceSoundListener& listener, const SourceSoundSettings& settings,
    const std::array<bool, kSourceSpuVoices>& voice_busy, SourceSoundVoiceState& state,
    std::vector<SourceSoundKeyOn>& out);

// SPU ADSR envelope (ADSR1/ADSR2 words), ticked at 44100 Hz. Key-on starts
// the attack at level 0; key-off enters release.
class SourceSpuEnvelope {
public:
    void key_on(uint16_t adsr1, uint16_t adsr2) noexcept;
    void key_off() noexcept;
    // Loop End without Repeat: release with the level forced to zero.
    void mute() noexcept { phase_ = Phase::Off; level_ = 0; }
    void advance(uint32_t samples) noexcept;
    int16_t level() const noexcept { return level_; }
    bool finished() const noexcept { return phase_ == Phase::Off; }
private:
    enum class Phase : uint8_t { Off, Attack, Decay, Sustain, Release };
    void tick() noexcept;
    Phase phase_ = Phase::Off;
    uint16_t adsr1_ = 0, adsr2_ = 0;
    int16_t level_ = 0;
    int32_t counter_ = 0;
};
} // namespace l3

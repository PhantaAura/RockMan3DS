#pragma once
#include <3ds.h>
#include "legends3ds/source_sound.h"
#include <array>
#include <vector>

// SPU voices for the 0x8001A110 handler-2 requests. Voice v plays on NDSP
// channel 2 + v % 22 (0 and 1 are the XA stream and the message voice). The
// envelope is stepped per gameplay tick and applied through the channel mix.
class NativeSfx {
public:
    ~NativeSfx(){close();}
    bool open(const char* path);
    void close() noexcept;
    bool ready() const noexcept{return bank_bytes_!=nullptr;}
    // Consumes and resets the queue (0x8001A110 then 0x8001A1D0).
    l3::SourceSoundConsumeResult consume(l3::SourceAudioQueue& queue,const l3::SourceSoundListener& listener);
    void tick(uint32_t samples_44100) noexcept;
    const char* error() const noexcept{return error_;}
    uint32_t linear_bytes() const noexcept{return bank_size_;}
    size_t requests() const noexcept{return bank_.requests.size();}
private:
    struct Voice {
        std::array<ndspWaveBuf,2> waves{};
        l3::SourceSpuEnvelope envelope;
        uint16_t left=0,right=0;
        bool active=false;
    };
    static int channel(size_t voice) noexcept{return int(2+voice%22);}
    void key_on(const l3::SourceSoundKeyOn& key) noexcept;
    void apply_mix(size_t v) noexcept;
    l3::SourceSoundBank bank_;
    l3::SourceSoundVoiceState state_{};
    l3::SourceSoundSettings settings_{};
    std::array<Voice,l3::kSourceSpuVoices> voices_{};
    std::vector<l3::SourceSoundKeyOn> key_ons_;
    uint8_t* bank_bytes_=nullptr;
    uint32_t bank_size_=0;
    bool dsp_ready_=false;
    const char* error_="disabled";
};

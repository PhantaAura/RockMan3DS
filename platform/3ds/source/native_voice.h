#pragma once
#include <3ds.h>
#include "legends3ds/voice_bank.h"

// Message voice clips (0x8F AUDIO -> 0x8001D58C) on NDSP channel 1. A new
// request replaces the clip still playing (one XA stream on the PS1 drive).
class NativeVoice {
public:
    ~NativeVoice(){close();}
    bool open(const char* path);
    void close() noexcept;
    bool play(uint32_t request) noexcept;
    void stop() noexcept;
    bool playing() const noexcept;
    const char* error() const noexcept{return error_;}
    uint32_t linear_bytes() const noexcept{return uint32_t(capacity_*2);}
    uint32_t clips() const noexcept{return uint32_t(bank_.entries().size());}
private:
    static constexpr int kChannel=1;
    l3::VoiceBankReader bank_;
    int16_t* pcm_=nullptr;
    size_t capacity_=0;
    ndspWaveBuf wave_{};
    bool dsp_ready_=false;
    const char* error_="disabled";
};

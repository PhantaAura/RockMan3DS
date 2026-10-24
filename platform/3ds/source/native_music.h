#pragma once
#include <3ds.h>
#include "legends3ds/source_music.h"
#include <array>
#include <vector>

// Sequenced music (0x8008241C handler 1, libsnd SEP). The core mixes the 15
// music SPU voices; the stereo mix streams on NDSP channel 4 (channels 2..23
// map SPU effect voices 15..23 as 2 + v % 22, so 4..16 are free). One 735-frame
// buffer is one 60 Hz SsSeqCalledTbyT tick; pump() keeps kAhead ticks queued,
// so the sequencer runs on the audio clock.
class NativeMusic {
public:
    ~NativeMusic(){close();}
    bool open(const char* path);
    void close() noexcept;
    bool ready() const noexcept{return dsp_ready_;}
    // 0x8001A110 handler-1 records; the caller still resets the queue.
    size_t consume(const l3::SourceAudioQueue& queue) noexcept{return dsp_ready_?player_.consume_8001a110(queue):0;}
    void stop_all() noexcept{if(dsp_ready_)player_.stop_all_8001b33c();}
    void fade_out() noexcept{if(dsp_ready_)player_.request_fade_out_8001d854();}
    void stage_loaded() noexcept{if(dsp_ready_)player_.set_stage_reverb_80019f94();}
    uint16_t area_request(size_t area) const noexcept{return l3::source_area_music_80019d18(bank_,area);}
    void pump() noexcept;
    const char* error() const noexcept{return error_;}
    uint32_t bank_bytes() const noexcept{return uint32_t(bytes_.size()*2);}
    const l3::SourceMusicPlayer& player() const noexcept{return player_;}
    uint32_t underruns() const noexcept{return underruns_;}
private:
    static constexpr int kChannel=4;
    static constexpr size_t kBuffers=8,kAhead=4;
    std::vector<uint16_t> bytes_;
    l3::SourceMusicBank bank_;
    l3::SourceMusicPlayer player_;
    std::array<ndspWaveBuf,kBuffers> waves_{};
    int16_t* pcm_=nullptr;
    uint32_t underruns_=0;
    bool dsp_ready_=false,started_=false;
    const char* error_="disabled";
};

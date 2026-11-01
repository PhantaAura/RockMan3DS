#pragma once
#include <3ds.h>
#include "legends3ds/xa_transport.h"

class NativeXaAudio {
public:
    ~NativeXaAudio(){close();}
    bool open(const char* path);
    void close() noexcept;
    bool start() noexcept;
    bool pause() noexcept;
    void pump() noexcept;
    bool apply_diagonal_mix(const std::array<uint8_t,4>& values) noexcept;
    // 3DS-only (cutscene skip): silence the channel; the stream keeps playing.
    void set_muted(bool muted) noexcept;
    l3::XaPlaybackObservation observation() const noexcept;
    // Sample-accurate DSP position (stream frames at 37800 Hz) while a buffer
    // plays; otherwise the completed-buffer count. Monotonic.
    uint64_t position_frames() const noexcept;
    const l3::XaStreamReader& stream() const noexcept{return stream_;}
    const char* error() const noexcept{return error_;}
    Result dsp_result() const noexcept{return dsp_result_;}
    uint32_t linear_bytes() const noexcept{return pcm_?2*l3::kXaSectorPcmBytes:0;}
    uint32_t underruns() const noexcept{return underruns_;}
    uint32_t max_read_us() const noexcept{return max_read_us_;}
private:
    bool queue(size_t slot) noexcept;
    l3::XaStreamReader stream_;
    std::array<ndspWaveBuf,2> waves_{};
    std::array<uint64_t,2> first_frame_{};
    int16_t* pcm_=nullptr;
    size_t next_sector_=0;
    uint64_t completed_frames_=0;
    float gain_=1.0f;
    bool muted_=false;
    bool dsp_ready_=false,start_queued_=false,ever_started_=false,failed_=false;
    uint32_t underruns_=0,max_read_us_=0;
    Result dsp_result_=0;
    const char* error_="disabled";
};

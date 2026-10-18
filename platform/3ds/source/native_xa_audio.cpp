#include "native_xa_audio.h"
#include <algorithm>
#include <exception>

bool NativeXaAudio::open(const char* path) {
    close();failed_=false;dsp_result_=0;
    try {if(!stream_.open(path)){error_=stream_.error();return false;}}
    catch(const std::exception&){stream_.close();error_="XA bounded allocation failed";return false;}
    dsp_result_=ndspInit();
    if(R_FAILED(dsp_result_)){
        stream_.close();
        error_=dsp_result_==MAKERESULT(RL_PERMANENT,RS_NOTFOUND,RM_DSP,RD_NOT_FOUND)
            ?"ndspInit: no /3ds/dspfirm.cdc or hb:ndsp component"
            :"ndspInit failed (DSP service/component error)";
        return false;
    }
    dsp_ready_=true;
    pcm_=static_cast<int16_t*>(linearAlloc(2*l3::kXaSectorPcmBytes));
    if(!pcm_){close();error_="XA linear buffer allocation failed";return false;}
    ndspChnReset(0);ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetFormat(0,NDSP_FORMAT_STEREO_PCM16);ndspChnSetRate(0,37800);
    ndspChnSetInterp(0,NDSP_INTERP_LINEAR);
    float mix[12]{};mix[0]=mix[1]=1.0f;ndspChnSetMix(0,mix);
    // Initial unity is a native diagnostic setting, not inferred PS1 CD mixer
    // ownership. Subsequent supported source diagonal intents replace it.
    waves_={};next_sector_=0;completed_frames_=0;underruns_=max_read_us_=0;
    start_queued_=ever_started_=false;error_="ready";return true;
}
void NativeXaAudio::close() noexcept {
    if(dsp_ready_){ndspChnWaveBufClear(0);ndspExit();}
    dsp_ready_=false;
    if(pcm_)linearFree(pcm_);
    pcm_=nullptr;stream_.close();waves_={};start_queued_=ever_started_=false;
    next_sector_=0;completed_frames_=0;error_="disabled";
}
bool NativeXaAudio::queue(size_t slot) noexcept {
    if(next_sector_>=stream_.index().size())return false;
    auto* pcm=pcm_+slot*l3::kXaFramesPerSector*2;
    const auto start=svcGetSystemTick();
    if(!stream_.read_sector(next_sector_,pcm,l3::kXaFramesPerSector)){
        failed_=true;error_=stream_.error();ndspChnWaveBufClear(0);return false;
    }
    const auto us=(svcGetSystemTick()-start)/CPU_TICKS_PER_USEC;
    max_read_us_=std::max(max_read_us_,uint32_t(us));
    first_frame_[slot]=stream_.index()[next_sector_].first_frame;++next_sector_;
    auto& wave=waves_[slot];wave={};wave.data_pcm16=pcm;wave.nsamples=l3::kXaFramesPerSector;
    DSP_FlushDataCache(pcm,l3::kXaSectorPcmBytes);ndspChnWaveBufAdd(0,&wave);return true;
}
bool NativeXaAudio::start() noexcept {
    if(!dsp_ready_||failed_||start_queued_)return false;
    start_queued_=true;
    if(!queue(0))return false;
    if(next_sector_<stream_.index().size()&&!queue(1))return false;
    error_="queued; awaiting actual DSP playback";return true;
}
bool NativeXaAudio::pause() noexcept {
    if(!dsp_ready_||failed_||!pcm_||!stream_.ready())return false;
    ndspChnSetPaused(0,true);
    return ndspChnIsPaused(0);
}
void NativeXaAudio::pump() noexcept {
    if(!dsp_ready_||failed_||!start_queued_)return;
    if(ndspChnIsPaused(0)){error_="paused";return;}
    const bool playing=ndspChnIsPlaying(0);if(playing)ever_started_=true;
    for(size_t slot=0;slot<waves_.size();++slot){
        const volatile ndspWaveBuf* wave=&waves_[slot];
        if(wave->status==NDSP_WBUF_DONE&&wave->nsamples){
            completed_frames_=std::max(completed_frames_,first_frame_[slot]+l3::kXaFramesPerSector);
            waves_[slot].nsamples=0;
        }
    }
    if(ever_started_&&!playing&&completed_frames_<stream_.info().total_frames)++underruns_;
    for(size_t slot=0;slot<waves_.size()&&!failed_;++slot)
        if(waves_[slot].nsamples==0&&next_sector_<stream_.index().size())(void)queue(slot);
    if(failed_)return;
    error_=completed_frames_>=stream_.info().total_frames?"completed":playing?"playing":"queued; awaiting DSP playback";
}
bool NativeXaAudio::apply_diagonal_mix(const std::array<uint8_t,4>& values) noexcept {
    if(!dsp_ready_||failed_||values[1]||values[3]||values[0]!=values[2])return false;
    float mix[12]{};mix[0]=mix[1]=float(values[0])/128.0f;ndspChnSetMix(0,mix);return true;
}
l3::XaPlaybackObservation NativeXaAudio::observation() const noexcept {
    const bool paused=dsp_ready_&&!failed_&&ndspChnIsPaused(0);
    return {dsp_ready_&&pcm_&&stream_.ready(),dsp_ready_&&!failed_&&!paused&&ndspChnIsPlaying(0),ever_started_,failed_,completed_frames_,paused};
}

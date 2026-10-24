#include "native_voice.h"
#include <algorithm>
#include <exception>

bool NativeVoice::open(const char* path) {
    close();
    try{if(!bank_.open(path)){error_=bank_.error();return false;}}
    catch(const std::exception&){bank_.close();error_="voice bank allocation failed";return false;}
    if(R_FAILED(ndspInit())){bank_.close();error_="ndspInit failed";return false;}
    dsp_ready_=true;
    for(const auto& e:bank_.entries())capacity_=std::max<size_t>(capacity_,e.frames);
    pcm_=static_cast<int16_t*>(linearAlloc(capacity_*2));
    if(!pcm_){close();error_="voice linear buffer allocation failed";return false;}
    ndspChnReset(kChannel);ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetFormat(kChannel,NDSP_FORMAT_MONO_PCM16);ndspChnSetRate(kChannel,float(bank_.sample_rate()));
    ndspChnSetInterp(kChannel,NDSP_INTERP_LINEAR);
    // Unity centre mix: the CD mixer volume path (0x80077DE4) is not ported.
    float mix[12]{};mix[0]=mix[1]=1.0f;ndspChnSetMix(kChannel,mix);
    error_="ready";return true;
}
void NativeVoice::close() noexcept {
    if(dsp_ready_){ndspChnWaveBufClear(kChannel);ndspExit();}
    dsp_ready_=false;
    if(pcm_)linearFree(pcm_);
    pcm_=nullptr;capacity_=0;wave_={};bank_.close();error_="closed";
}
void NativeVoice::stop() noexcept {
    if(dsp_ready_)ndspChnWaveBufClear(kChannel);
    wave_={};
}
bool NativeVoice::play(uint32_t request) noexcept {
    if(!dsp_ready_||!pcm_){error_="voice not ready";return false;}
    const auto* entry=bank_.find(request);
    if(!entry){error_="voice request not cooked";return false;}
    // Clear first: the DSP must not read the buffer while it is replaced.
    stop();
    if(!bank_.read_clip(*entry,pcm_,capacity_)){error_=bank_.error();return false;}
    DSP_FlushDataCache(pcm_,entry->frames*2);
    wave_.data_vaddr=pcm_;wave_.nsamples=entry->frames;wave_.looping=false;
    ndspChnWaveBufAdd(kChannel,&wave_);
    error_="playing";return true;
}
bool NativeVoice::playing() const noexcept {
    return dsp_ready_&&ndspChnIsPlaying(kChannel);
}

#include "native_music.h"
#include <cstdio>
#include <cstring>
#include <exception>

bool NativeMusic::open(const char* path) {
    close();
    std::FILE* f=std::fopen(path,"rb");
    if(!f){error_="music bank missing";return false;}
    long size=-1;
    if(!std::fseek(f,0,SEEK_END))size=std::ftell(f);
    if(size<=0||size>(4l<<20)||std::fseek(f,0,SEEK_SET)){std::fclose(f);error_="music bank size";return false;}
    bytes_.assign((size_t(size)+1)/2,0);
    const bool read=std::fread(bytes_.data(),1,size_t(size),f)==size_t(size);std::fclose(f);
    if(!read){close();error_="short music bank";return false;}
    try{bank_=l3::load_l3music_bank(reinterpret_cast<const uint8_t*>(bytes_.data()),size_t(size));}
    catch(const std::exception&){close();error_="music bank rejected";return false;}
    pcm_=static_cast<int16_t*>(linearAlloc(kBuffers*l3::kSourceMusicFramesPerTick*4));
    if(!pcm_){close();error_="music linear allocation";return false;}
    if(R_FAILED(ndspInit())){linearFree(pcm_);pcm_=nullptr;close();error_="ndspInit failed";return false;}
    dsp_ready_=true;
    ndspChnReset(kChannel);ndspChnSetFormat(kChannel,NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetInterp(kChannel,NDSP_INTERP_LINEAR);ndspChnSetRate(kChannel,44100.f);
    float mix[12]{};mix[0]=mix[1]=1.f;ndspChnSetMix(kChannel,mix);
    for(size_t i=0;i<kBuffers;++i){
        waves_[i]={};waves_[i].data_vaddr=pcm_+i*l3::kSourceMusicFramesPerTick*2;
        waves_[i].nsamples=l3::kSourceMusicFramesPerTick;waves_[i].status=NDSP_WBUF_DONE;
    }
    player_.bind(&bank_);
    error_="ready";return true;
}
void NativeMusic::close() noexcept {
    if(dsp_ready_){ndspChnWaveBufClear(kChannel);ndspExit();}
    dsp_ready_=started_=false;player_.bind(nullptr);bank_={};
    std::vector<uint16_t>().swap(bytes_);
    if(pcm_)linearFree(pcm_);
    pcm_=nullptr;error_="closed";
}
void NativeMusic::pump() noexcept {
    if(!dsp_ready_)return;
    size_t queued=0;
    for(const auto& w:waves_)queued+=w.status==NDSP_WBUF_QUEUED||w.status==NDSP_WBUF_PLAYING;
    if(started_&&!queued)++underruns_;
    for(auto& w:waves_){
        if(queued>=kAhead)break;
        if(w.status!=NDSP_WBUF_DONE&&w.status!=NDSP_WBUF_FREE)continue;
        int16_t* pcm=const_cast<int16_t*>(static_cast<const int16_t*>(w.data_vaddr));
        player_.render_tick(pcm);
        DSP_FlushDataCache(pcm,l3::kSourceMusicFramesPerTick*4);
        ndspChnWaveBufAdd(kChannel,&w);
        ++queued;started_=true;
    }
}

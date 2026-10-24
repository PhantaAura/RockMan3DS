#include "native_sfx.h"
#include <cstdio>
#include <exception>

bool NativeSfx::open(const char* path) {
    close();
    std::FILE* f=std::fopen(path,"rb");
    if(!f){error_="sound bank missing";return false;}
    long size=-1;
    if(!std::fseek(f,0,SEEK_END))size=std::ftell(f);
    if(size<=0||size>(8l<<20)||std::fseek(f,0,SEEK_SET)){std::fclose(f);error_="sound bank size";return false;}
    bank_bytes_=static_cast<uint8_t*>(linearAlloc(size_t(size)));
    if(!bank_bytes_){std::fclose(f);error_="sound bank linear allocation";return false;}
    bank_size_=uint32_t(size);
    const bool read=std::fread(bank_bytes_,1,size_t(size),f)==size_t(size);std::fclose(f);
    if(!read){close();error_="short sound bank";return false;}
    try{bank_=l3::load_l3sound_bank(bank_bytes_,bank_size_);}
    catch(const std::exception&){close();error_="sound bank rejected";return false;}
    DSP_FlushDataCache(bank_bytes_,bank_size_);
    if(R_FAILED(ndspInit())){close();error_="ndspInit failed";return false;}
    dsp_ready_=true;ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    for(size_t v=0;v<voices_.size();++v){
        const int ch=channel(v);
        ndspChnReset(ch);ndspChnSetFormat(ch,NDSP_FORMAT_MONO_PCM16);ndspChnSetInterp(ch,NDSP_INTERP_LINEAR);
    }
    key_ons_.reserve(l3::kSourceAudioQueueCapacity*4);
    error_="ready";return true;
}
void NativeSfx::close() noexcept {
    if(dsp_ready_){for(size_t v=0;v<voices_.size();++v)ndspChnWaveBufClear(channel(v));ndspExit();}
    dsp_ready_=false;voices_={};state_={};
    if(bank_bytes_)linearFree(bank_bytes_);
    bank_bytes_=nullptr;bank_size_=0;bank_={};error_="closed";
}
void NativeSfx::apply_mix(size_t v) noexcept {
    auto& voice=voices_[v];
    // SPU voice volume 0..0x3FFF times the envelope 0..0x7FFF.
    const float env=float(voice.envelope.level())/32768.f;
    float mix[12]{};mix[0]=float(voice.left)/16384.f*env;mix[1]=float(voice.right)/16384.f*env;
    ndspChnSetMix(channel(v),mix);
}
void NativeSfx::key_on(const l3::SourceSoundKeyOn& key) noexcept {
    auto& voice=voices_[key.voice];const int ch=channel(key.voice);
    ndspChnWaveBufClear(ch);voice.waves={};voice.active=false;
    if(key.sample==0xFFFF||key.sample>=bank_.samples.size())return;   // empty tone: silence
    const auto& s=bank_.samples[key.sample];
    const int16_t* pcm=reinterpret_cast<const int16_t*>(bank_bytes_+s.offset);
    ndspChnSetRate(ch,44100.f*float(key.pitch)/4096.f);
    voice.left=key.volume_left;voice.right=key.volume_right;
    voice.envelope.key_on(key.adsr1,key.adsr2);
    if(s.loop_start==0xFFFFFFFFu){
        voice.waves[0].data_vaddr=pcm;voice.waves[0].nsamples=s.frames;voice.waves[0].looping=false;
        ndspChnWaveBufAdd(ch,&voice.waves[0]);
    }else{
        if(s.loop_start){
            voice.waves[0].data_vaddr=pcm;voice.waves[0].nsamples=s.loop_start;voice.waves[0].looping=false;
            ndspChnWaveBufAdd(ch,&voice.waves[0]);
        }
        voice.waves[1].data_vaddr=pcm+s.loop_start;voice.waves[1].nsamples=s.frames-s.loop_start;voice.waves[1].looping=true;
        ndspChnWaveBufAdd(ch,&voice.waves[1]);
    }
    voice.active=true;apply_mix(key.voice);
}
l3::SourceSoundConsumeResult NativeSfx::consume(l3::SourceAudioQueue& queue,const l3::SourceSoundListener& listener) {
    l3::SourceSoundConsumeResult result;
    if(!dsp_ready_){l3::finish_source_audio_queue_batch_8001a1d0(queue);return result;}
    std::array<bool,l3::kSourceSpuVoices> busy{};
    for(size_t v=0;v<voices_.size();++v)busy[v]=voices_[v].active&&ndspChnIsPlaying(channel(v));
    key_ons_.clear();
    result=l3::consume_source_audio_8001a110(queue,bank_,listener,settings_,busy,state_,key_ons_);
    for(const auto& key:key_ons_)key_on(key);
    l3::finish_source_audio_queue_batch_8001a1d0(queue);
    return result;
}
void NativeSfx::tick(uint32_t samples) noexcept {
    if(!dsp_ready_)return;
    for(size_t v=0;v<voices_.size();++v){
        auto& voice=voices_[v];
        if(!voice.active)continue;
        if(!ndspChnIsPlaying(channel(v))){voice.active=false;continue;}
        voice.envelope.advance(samples);apply_mix(v);
    }
}

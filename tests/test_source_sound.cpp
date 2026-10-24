// L3SB parsing, the 0x8001A110/0x8001A968 consumer (gate 0x8001A834, volumes
// 0x8001A274) and the SPU ADSR stepper. Authored bank only.
#include "legends3ds/source_sound.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace l3;
namespace {
uint32_t crc(const uint8_t* p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(int k=0;k<8;++k)c=(c>>1)^(0xEDB88320u&(0u-(c&1u)));}return ~c;}
void put16(std::vector<uint8_t>& b,uint16_t v){b.push_back(uint8_t(v));b.push_back(uint8_t(v>>8));}
void put32(std::vector<uint8_t>& b,uint32_t v){put16(b,uint16_t(v));put16(b,uint16_t(v>>16));}
std::vector<uint8_t> bank(){
    // requests 0xA0 (voice 21, prio 4, tag 0, tone 0) and 0x11A (voice 15, prio 3, two tones)
    std::vector<uint8_t> t;
    put16(t,0xA0);t.push_back(21);t.push_back(1);t.push_back(4);t.push_back(0);put16(t,0);
    put16(t,0x11A);t.push_back(15);t.push_back(2);t.push_back(3);t.push_back(2);put16(t,1);
    for(int i=0;i<3;++i){put16(t,0x5CE);put16(t,0x80FF);put16(t,0x1FC0);t.push_back(i==0?72:44);t.push_back(0);put16(t,i==2?0xFFFF:0);}
    const std::vector<int16_t> pcm{0,1000,-1000,500};
    const uint32_t offset=36+uint32_t(t.size())+16;
    put32(t,uint32_t(pcm.size()));put32(t,0xFFFFFFFFu);put32(t,offset);
    put32(t,crc(reinterpret_cast<const uint8_t*>(pcm.data()),pcm.size()*2));
    std::vector<uint8_t> out={'L','3','S','B','0','0','1',0};
    put32(out,1);put32(out,2);put32(out,3);put32(out,1);put32(out,44100);
    put32(out,offset+uint32_t(pcm.size()*2));put32(out,crc(t.data(),t.size()));
    out.insert(out.end(),t.begin(),t.end());
    const auto* p=reinterpret_cast<const uint8_t*>(pcm.data());out.insert(out.end(),p,p+pcm.size()*2);
    return out;
}
SourceAudioQueue queue(){SourceAudioQueue q;initialize_source_audio_queue_80019e14(q);return q;}
}
int main(){
    const auto bytes=bank();
    const auto b=load_l3sound_bank(bytes.data(),bytes.size());
    assert(b.requests.size()==2&&b.tones.size()==3&&b.samples.size()==1&&b.find(0x11A)->tones==2);
    {auto bad=bytes;bad.back()^=1;bool threw=false;try{(void)load_l3sound_bank(bad.data(),bad.size());}catch(const std::exception&){threw=true;}assert(threw);}
    {auto bad=bytes;bad[37]^=1;bool threw=false;try{(void)load_l3sound_bank(bad.data(),bad.size());}catch(const std::exception&){threw=true;}assert(threw);}

    SourceSoundListener l{0,0,0};SourceSoundSettings s;SourceSoundVoiceState st{};std::array<bool,24> busy{};
    std::vector<SourceSoundKeyOn> out;
    // Plain request 0xA0, centre: 72*0x81 = 9288 both sides.
    auto q=queue();(void)enqueue_source_audio_8001a008(q,0xA0,0,0);
    auto r=consume_source_audio_8001a110(q,b,l,s,busy,st,out);
    assert(r.key_ons==1&&out[0].voice==21&&out[0].volume_left==9288&&out[0].volume_right==9288&&st.priority_b48[21]==4);
    // Pan +0x40 halves the left; attenuation 0x40 halves both.
    out.clear();q=queue();(void)enqueue_source_audio_8001a008(q,0xA0,0x40,0x40);
    (void)consume_source_audio_8001a110(q,b,l,s,busy,st,out);
    assert(out[0].volume_left==2322&&out[0].volume_right==4644);
    // Busy voice with a higher stored priority rejects; unknown ids are counted.
    out.clear();busy[21]=true;st.priority_b48[21]=9;q=queue();
    (void)enqueue_source_audio_8001a008(q,0xA0,0,0);(void)enqueue_source_audio_8001a008(q,0x7FF,0,0);
    r=consume_source_audio_8001a110(q,b,l,s,busy,st,out);assert(r.key_ons==0&&r.rejected==1&&r.unknown==1);
    // Positional: straight ahead (heading 0 faces -z) at 2500 -> both sides at half.
    out.clear();busy={};q=queue();
    (void)enqueue_source_audio_8001a050(q,0x11A,{{uint32_t(uint16_t(0)),uint32_t(uint16_t(int16_t(-2500)))}});
    r=consume_source_audio_8001a110(q,b,l,s,busy,st,out);
    assert(r.key_ons==2&&out[0].voice==15&&out[1].voice==16&&out[1].sample==0xFFFF);
    assert(out[0].volume_left==2838&&out[0].volume_right==2838&&st.distance_c1cf0[15]==2500);
    // At 5000 or beyond the positional request is dropped.
    out.clear();q=queue();
    (void)enqueue_source_audio_8001a050(q,0x11A,{{uint32_t(uint16_t(5000)),0u}});
    r=consume_source_audio_8001a110(q,b,l,s,busy,st,out);assert(r.key_ons==0&&r.rejected==2);
    // ADSR 0x80FF/0x1FC0: fastest exponential attack, sustain at maximum.
    SourceSpuEnvelope e;e.key_on(0x80FF,0x1FC0);e.advance(4000);assert(e.level()==0x7FFF&&!e.finished());
    e.key_off();e.advance(1u<<20);assert(e.finished()&&e.level()==0);
    std::cout<<"PASS: L3SB, 0x8001A110 handler-2 gate/volume/positional, SPU ADSR\n";
}

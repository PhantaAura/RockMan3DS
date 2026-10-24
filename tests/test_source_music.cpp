// L3MU parsing, the 0x8001AC58 slot rules, the libsnd SEP timing/loop/voice
// model and the software SPU mix. Authored bank only.
#include "legends3ds/source_music.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace l3;
namespace {
uint32_t crc(const uint8_t* p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(int k=0;k<8;++k)c=(c>>1)^(0xEDB88320u&(0u-(c&1u)));}return ~c;}
void put16(std::vector<uint8_t>& b,uint16_t v){b.push_back(uint8_t(v));b.push_back(uint8_t(v>>8));}
void put32(std::vector<uint8_t>& b,uint32_t v){put16(b,uint16_t(v));put16(b,uint16_t(v>>16));}
void pad4(std::vector<uint8_t>& b){while(b.size()&3)b.push_back(0);}

// One sequence: program 0, loop start (NRPN 20, count 127), note 60 on for 24
// ticks, off for 24 ticks, loop end (NRPN 30), end of track. 48 ticks per
// quarter at 150 bpm = 2 MIDI ticks per 60 Hz call (step 20).
std::vector<uint8_t> sep(){
    const std::vector<uint8_t> data{0x00, 0xC0,0x00, 0x00, 0xB0,0x63,0x14, 0x00, 0xB0,0x06,0x7F, 0x00,
        0x90,0x3C,0x64, 0x18, 0x90,0x3C,0x00, 0x18, 0xB0,0x63,0x1E, 0x00, 0xFF,0x2F, 0x00};
    std::vector<uint8_t> s{'p','Q','E','S',0,0, 0,0, 0x00,0x30, 0x06,0x1A,0x80, 0x04,0x02};
    put32(s,0);const uint32_t n=uint32_t(data.size());s[15]=uint8_t(n>>24);s[16]=uint8_t(n>>16);s[17]=uint8_t(n>>8);s[18]=uint8_t(n);
    s.insert(s.end(),data.begin(),data.end());
    return s;
}
std::vector<uint8_t> vh(){
    std::vector<uint8_t> v(0x820+0x200+0x200,0);
    std::memcpy(v.data(),"pBAV",4);v[4]=7;v[0x12]=1;v[0x14]=1;v[0x16]=1;v[0x18]=127;v[0x19]=64;
    uint8_t* prog=v.data()+0x20;prog[0]=1;prog[1]=127;prog[2]=3;prog[4]=64;
    uint8_t* tone=v.data()+0x820;tone[0]=0;tone[1]=4;tone[2]=127;tone[3]=64;tone[4]=60;tone[5]=0;tone[6]=0;tone[7]=127;
    tone[0x10]=0xFF;tone[0x11]=0x80;tone[0x12]=0xCB;tone[0x13]=0x5F;tone[0x16]=1;
    v[0xA20+2]=13;   // VAG 1: 13 blocks
    return v;
}
std::vector<uint8_t> bank(){
    std::vector<uint8_t> t;
    for(int i=0;i<192;++i)put16(t,uint16_t(0x1000+i));
    // stage reverb entry [type 2, RVol 0x40, depth 0x20], work area 0xFF00 (0x400 halfwords),
    // a small authored preset: one comb tap and short all-pass delays.
    t.push_back(2);t.push_back(0x40);t.push_back(0x20);t.push_back(0);put32(t,0xFF00);
    const uint16_t regs[32]={1,1,0x4000,0x4000,0,0,0,0,0,0, 0x40,0x40, 0x40,0x40,0,0, 0x20,0x20,0,0,0,0,0,0,0,0, 8,8,4,4, 0x4000,0x4000};
    for(uint16_t r:regs)put16(t,r);
    for(uint16_t a:{uint16_t(0xFFFF),uint16_t(0),uint16_t(0)})put16(t,a);
    pad4(t);
    put16(t,0);t.push_back(0x54);t.push_back(0x00);t.push_back(0x03);t.push_back(0x00);put16(t,0);  // request 0
    const auto s=sep();const auto h=vh();
    const std::vector<int16_t> pcm(100,8000);
    const uint32_t header=44, table=uint32_t(t.size())+12+16+16;
    uint32_t off=header+table;
    t.push_back(0);t.push_back(1);put16(t,0);put32(t,uint32_t(s.size()));put32(t,off);off+=(uint32_t(s.size())+3)&~3u;
    t.push_back(20);t.push_back(0);put16(t,0);put32(t,uint32_t(h.size()));put32(t,off);put16(t,0);put16(t,1);off+=uint32_t(h.size());
    put32(t,uint32_t(pcm.size()));put32(t,0);put32(t,off);
    put32(t,crc(reinterpret_cast<const uint8_t*>(pcm.data()),pcm.size()*2));off+=uint32_t(pcm.size()*2);
    std::vector<uint8_t> out={'L','3','M','U','0','0','1',0};
    for(uint32_t w:{1u,3u,1u,1u,1u,1u,44100u,off,crc(t.data(),t.size())})put32(out,w);
    out.insert(out.end(),t.begin(),t.end());
    out.insert(out.end(),s.begin(),s.end());pad4(out);
    out.insert(out.end(),h.begin(),h.end());
    const auto* p=reinterpret_cast<const uint8_t*>(pcm.data());out.insert(out.end(),p,p+pcm.size()*2);
    assert(out.size()==off);
    return out;
}
SourceAudioQueue request(uint16_t id){
    SourceAudioQueue q;initialize_source_audio_queue_80019e14(q);
    (void)enqueue_source_audio_8001a008(q,id,0,0);
    return q;
}
SourceAudioQueue empty(){SourceAudioQueue q;initialize_source_audio_queue_80019e14(q);return q;}
}

int main(){
    const auto bytes=bank();
    const auto b=load_l3music_bank(bytes.data(),bytes.size());
    assert(b.requests.size()==1&&b.seps.size()==1&&b.vabs.size()==1&&b.samples.size()==1);
    assert(source_area_music_80019d18(b,0)==0xFFFF&&source_area_music_80019d18(b,1)==0&&source_area_music_80019d18(b,9)==0xFFFF);
    {auto bad=bytes;bad.back()^=1;bool threw=false;try{(void)load_l3music_bank(bad.data(),bad.size());}catch(const std::exception&){threw=true;}assert(threw);}
    {auto bad=bytes;bad[50]^=1;bool threw=false;try{(void)load_l3music_bank(bad.data(),bad.size());}catch(const std::exception&){threw=true;}assert(threw);}

    SourceMusicPlayer m;m.bind(&b);
    // Unknown ids are left to the effect consumer.
    assert(m.consume_8001a110(request(0x84))==0&&m.slot(0).access==-1);
    assert(m.consume_8001a110(request(0))==1);
    assert(m.slot(0).access==0&&m.slot(0).type==1&&m.slot(0).priority==3&&m.slot(0).request==0);
    // Tick 1 reads up to the first note (keyed at the next flush).
    std::vector<int16_t> pcm(kSourceMusicFramesPerTick*2);
    m.render_tick(pcm.data());
    assert(m.key_ons()==1&&m.sounding_voices()==0);
    // The next 0x8001A110 applies SsSepSetVol(100) to the playing sequence.
    assert(m.consume_8001a110(empty())==0&&m.slot(0).playing==1);
    m.render_tick(pcm.data());
    assert(m.sounding_voices()==1);
    // Volume law: velocity 100, volumes 127, sequence 100, centre pans:
    // base 12900, x100/127 = 10157, squared /0x3FFF = 6297 (0x3FFF = 1.0).
    int peak=0;for(size_t i=0;i<pcm.size();i+=2)peak=std::max(peak,int(pcm[i]));
    assert(peak>0&&peak<=8000*6297/16384+1);
    // The note lasts 24 MIDI ticks = 12 calls; the loop end then jumps back
    // to the loop start (count 127) and keys the note again.
    for(int i=0;i<10;++i)m.render_tick(pcm.data());
    assert(m.key_ons()==1&&m.loops()==0);
    m.render_tick(pcm.data());                      // tick 13: note off read
    for(int i=0;i<12;++i)m.render_tick(pcm.data()); // tick 25: loop end, note on
    assert(m.loops()==1&&m.key_ons()==2&&m.errors()==0);
    // Same request while playing: only 0x800AD152 = 0x7F, no restart.
    m.request_fade_out_8001d854();
    for(int i=0;i<3;++i)(void)m.consume_8001a110(empty());
    assert(m.volume_ad152()==0x7F-12);
    (void)m.consume_8001a110(request(0));
    assert(m.volume_ad152()==0x7F&&m.slot(0).access==0&&m.key_ons()==2);
    // The fade keeps stepping by 4 until 0.
    for(int i=0;i<40;++i)(void)m.consume_8001a110(empty());
    assert(m.volume_ad152()==0);
    // 0x8001B33C stops the sequence; the next request reopens and restarts it.
    m.stop_all_8001b33c();
    m.render_tick(pcm.data());
    (void)m.consume_8001a110(empty());
    assert(m.slot(0).playing==0);
    (void)m.consume_8001a110(request(0));
    assert(m.slot(0).access==0&&m.volume_ad152()==0x7F);
    m.render_tick(pcm.data());
    assert(m.key_ons()==3);

    // Reverb: type set by the first 0x8001AEF0 after the stage load, output
    // volume (RVol 0x40 -> 0x4080) 30 passes later; the wet mix then differs.
    SourceMusicPlayer dry,wet;dry.bind(&b);wet.bind(&b);wet.set_stage_reverb_80019f94();
    (void)dry.consume_8001a110(request(0));(void)wet.consume_8001a110(request(0));
    assert(wet.reverb_type()==2&&wet.reverb_out()==0&&dry.reverb_type()==0);
    for(int i=0;i<29;++i){(void)wet.consume_8001a110(empty());(void)dry.consume_8001a110(empty());}
    assert(wet.reverb_out()==0);
    (void)wet.consume_8001a110(empty());(void)dry.consume_8001a110(empty());
    assert(wet.reverb_out()==0x4080);
    std::vector<int16_t> a(pcm.size()),c(pcm.size());
    long diff=0;
    for(int i=0;i<20;++i){dry.render_tick(a.data());wet.render_tick(c.data());for(size_t k=0;k<a.size();++k)diff+=std::labs(long(a[k])-long(c[k]));}
    assert(diff>0&&wet.errors()==0);
    std::cout<<"source_music OK\n";
    return 0;
}

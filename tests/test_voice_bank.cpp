// L3VB voice bank reader: authored clips only (no owner audio).
#include "legends3ds/voice_bank.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace l3;
namespace {
uint32_t crc(const uint8_t* p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(int k=0;k<8;++k)c=(c>>1)^(0xEDB88320u&(0u-(c&1u)));}return ~c;}
void put(std::vector<uint8_t>& b,uint32_t v){for(int i=0;i<4;++i)b.push_back(uint8_t(v>>(8*i)));}
std::vector<uint8_t> bank(const std::vector<std::pair<uint32_t,std::vector<int16_t>>>& clips){
    std::vector<uint8_t> table,pcm;uint32_t off=32+16*uint32_t(clips.size());
    for(const auto& c:clips){const auto* p=reinterpret_cast<const uint8_t*>(c.second.data());
        put(table,c.first);put(table,uint32_t(c.second.size()));put(table,off);put(table,crc(p,c.second.size()*2));
        pcm.insert(pcm.end(),p,p+c.second.size()*2);off+=uint32_t(c.second.size()*2);}
    std::vector<uint8_t> out={'L','3','V','B','0','0','1',0};
    put(out,1);put(out,uint32_t(clips.size()));put(out,18900);put(out,16);put(out,off);put(out,crc(table.data(),table.size()));
    out.insert(out.end(),table.begin(),table.end());out.insert(out.end(),pcm.begin(),pcm.end());return out;
}
std::string write(const std::vector<uint8_t>& b,const char* name){
    const std::string path=std::string("test_voice_bank_")+name+".l3vb";
    FILE* f=std::fopen(path.c_str(),"wb");std::fwrite(b.data(),1,b.size(),f);std::fclose(f);return path;
}
}
int main(){
    const auto good=bank({{0,{1,-2,3}},{5,{100,200,-300,400,-500}}});
    VoiceBankReader r;
    assert(r.open(write(good,"good").c_str())&&r.sample_rate()==18900&&r.entries().size()==2);
    const auto* e=r.find(5);assert(e&&e->frames==5&&!r.find(7));
    std::vector<int16_t> s(8);assert(r.read_clip(*e,s.data(),s.size())&&s[0]==100&&s[4]==-500);
    assert(!r.read_clip(*e,s.data(),4));                                  // capacity
    auto bad=good;bad[bad.size()-1]^=1;                                   // PCM CRC
    assert(r.open(write(bad,"pcm").c_str()));assert(!r.read_clip(*r.find(5),s.data(),s.size()));
    bad=good;bad[33]^=1;assert(!r.open(write(bad,"table").c_str()));      // table CRC
    bad=good;bad.pop_back();assert(!r.open(write(bad,"size").c_str()));   // size
    bad=good;bad[0]='X';assert(!r.open(write(bad,"magic").c_str()));
    assert(!r.open(write(bank({{1,{1}},{1,{2}}}),"dup").c_str()));        // duplicate request
    assert(!r.open("missing.l3vb"));
    std::cout<<"PASS: L3VB open/find/read, CRC, bounds, duplicate and size rejection\n";
}

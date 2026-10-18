#include "legends3ds/source_reaction_profile.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <stdexcept>
namespace l3 {
namespace {
constexpr char digest[]="9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c";
uint8_t hex(char c){return uint8_t(c<='9'?c-'0':c-'a'+10);}
}
SourceReactionProfile load_source_reaction_profile(const uint8_t* b,size_t n){
    if(!b||n!=100||b[0]!='L'||b[1]!='3'||b[2]!='R'||b[3]!='7')throw std::runtime_error("L3R7 magic/size");
    if(rd_u16(b+4)!=1||rd_u16(b+6)!=64||rd_u32(b+8)!=1||rd_u32(b+12)!=0||
       rd_u32(b+16)!=3||rd_u32(b+20)!=n||rd_u32(b+28)!=12||
       rd_u32(b+24)!=crc32(b+64,36))throw std::runtime_error("L3R7 header/CRC");
    for(size_t i=0;i<32;++i)if(b[32+i]!=uint8_t(hex(digest[i*2])*16+hex(digest[i*2+1])))
        throw std::runtime_error("L3R7 owner overlay identity");
    SourceReactionProfile p;
    for(size_t i=0;i<3;++i)for(size_t j=0;j<12;++j)p.timer_pairs[i][j]=b[64+i*12+j];
    return p;
}
std::vector<uint8_t> encode_source_reaction_profile(const SourceReactionProfile& p){
    std::vector<uint8_t> b{'L','3','R','7'};wr_u16(b,1);wr_u16(b,64);
    for(uint32_t v:{1u,0u,3u,100u,0u,12u})wr_u32(b,v);
    for(size_t i=0;i<32;++i)b.push_back(uint8_t(hex(digest[i*2])*16+hex(digest[i*2+1])));
    for(const auto& row:p.timer_pairs)for(auto v:row)b.push_back(v);
    patch_u32(b,24,crc32(b.data()+64,36));return b;
}
} // namespace l3

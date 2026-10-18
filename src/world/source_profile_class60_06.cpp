#include "legends3ds/source_profile_class60_06.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>
namespace l3 {
namespace {
constexpr char digest[]="9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c";
uint8_t hex(char c){return uint8_t(c<='9'?c-'0':c-'a'+10);}
}
Class60_06Profile load_source_profile_class60_06(const uint8_t* b,size_t n){
 if(!b||n!=112||b[0]!='L'||b[1]!='3'||b[2]!='C'||b[3]!='6')throw std::runtime_error("L3C6 magic/size");
 if(rd_u16(b+4)!=1||rd_u16(b+6)!=64||rd_u32(b+8)!=1||rd_u32(b+12)!=0||
    rd_u32(b+16)!=6||rd_u32(b+20)!=n||rd_u32(b+28)!=8||
    rd_u32(b+24)!=crc32(b+64,48))throw std::runtime_error("L3C6 unsupported profile/header/CRC");
 for(size_t i=0;i<32;++i)if(b[32+i]!=uint8_t(hex(digest[i*2])*16+hex(digest[i*2+1])))
    throw std::runtime_error("L3C6 owner overlay identity");
 Class60_06Profile out;
 for(size_t i=0;i<6;++i)for(size_t k=0;k<2;++k){
  out.normal_uv[i][k]=rd_u16(b+64+i*4+k*2);
  out.alternate_uv[i][k]=rd_u16(b+88+i*4+k*2);
 }
 return out;
}
std::vector<uint8_t> encode_source_profile_class60_06(const Class60_06Profile& p){
 if(p.source_stage!=0)throw std::runtime_error("L3C6 unsupported stage");
 std::vector<uint8_t> out{'L','3','C','6'};wr_u16(out,1);wr_u16(out,64);
 for(uint32_t v:{1u,0u,6u,112u,0u,8u})wr_u32(out,v);
 for(size_t i=0;i<32;++i)out.push_back(uint8_t(hex(digest[i*2])*16+hex(digest[i*2+1])));
 for(const auto* table:{&p.normal_uv,&p.alternate_uv})for(const auto& row:*table)for(auto v:row)wr_u16(out,v);
 patch_u32(out,24,crc32(out.data()+64,48));return out;
}
}

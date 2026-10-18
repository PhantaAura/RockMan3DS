#include "legends3ds/source_event_command_package.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace l3 {
SourceEventCommandPackage load_source_event_command_package(const std::vector<uint8_t>& bytes) {
    const std::array<uint8_t,32> identity{{
        0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,
        0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
    const auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    require(bytes.size()==kSourceEventCommandPackageBytes,"noncanonical command package size");
    const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='E'&&p[3]=='C',"command package magic rejected");
    require(rd_u16(p+4)==1&&rd_u16(p+6)==64,"command package version/header rejected");
    require(rd_u32(p+8)==1&&rd_u32(p+12)==0&&rd_u32(p+16)==48&&
        rd_u32(p+20)==bytes.size()&&rd_u32(p+28)==0,"command package decoder/coverage rejected");
    require(std::equal(identity.begin(),identity.end(),p+32),"command executable identity rejected");
    require(crc32(p+64,48)==rd_u32(p+24),"command package CRC rejected");
    require(rd_u32(p+64)<=65535,"command profile id domain rejected");
    // This supported identity's measured cold LOAD fields are zero. These are
    // not assumed scene-reset values and never substitute for a live response.
    require(std::all_of(p+72,p+112,[](uint8_t value){return value==0;}),
        "unsupported retained command cold LOAD fields");
    SourceEventCommandPackage out;
    out.profile={rd_u32(p+64),rd_u32(p+68),true};
    auto& c=out.cold_context;
    c.byte_800988ec=p[72];c.byte_8009896c=p[73];c.byte_800988c0=p[74];
    c.byte_80098b42=p[75];c.byte_80098ab8=p[76];
    c.half_800ad142=rd_u16(p+80);c.half_800ad14e=rd_u16(p+82);
    c.word_800989b0=rd_u32(p+84);c.callback_80091ffc=rd_u32(p+88);c.callback_80092000=rd_u32(p+92);
    c.word_800989d8=rd_u32(p+96);
    std::copy(p+100,p+104,c.mix_800ad154.begin());c.mix_known.fill(1);
    std::copy(p+104,p+112,c.response_80098a98.begin());c.response_known.fill(1);
    return out;
}
} // namespace l3

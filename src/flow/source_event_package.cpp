#include "legends3ds/source_event_package.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
constexpr std::array<uint8_t,32> kExecutableDigest{{
    0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,
    0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
}
SourceEventPhase2Package load_source_event_phase2_package(const std::vector<uint8_t>& bytes) {
    require(bytes.size()==kSourceEventPhase2PackageBytes,"noncanonical event phase2 package size");
    const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='E'&&p[3]=='Q',"event phase2 package magic rejected");
    require(rd_u16(p+4)==1&&rd_u16(p+6)==64,"event phase2 version/header rejected");
    require(rd_u32(p+8)==1&&rd_u32(p+12)==0&&rd_u32(p+16)==304&&
        rd_u32(p+20)==bytes.size()&&rd_u32(p+28)==0,"event phase2 coverage/decoder rejected");
    require(std::equal(kExecutableDigest.begin(),kExecutableDigest.end(),p+32),
        "event phase2 executable identity rejected");
    require(crc32(p+64,304)==rd_u32(p+24),"event phase2 payload CRC rejected");
    require(rd_u32(p+64)==0x20000003u,"unsupported phase2 command request");
    // The only supported cold-boot profile was verified inside the original
    // LOAD image by the cooker. Zero data is a premise here, not a BSS guess.
    require(std::all_of(p+80,p+368,[](uint8_t b){return b==0;}),
        "unsupported event cold-boot retained data");
    SourceEventPhase2Package out;
    out.profile.request_8001d58c=rd_u32(p+64);
    out.profile.id=rd_u32(p+68);
    out.profile.pair_words={{rd_u32(p+72),rd_u32(p+76)}};
    out.profile.initialized=true;
    std::copy(p+80,p+112,out.boot_descriptors.begin());
    std::copy(p+112,p+368,out.boot_command_records.begin());
    return out;
}
} // namespace l3

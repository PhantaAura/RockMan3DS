#include "legends3ds/actor_detection_profile.h"
#include "legends3ds/l3map.h"
#include "legends3ds/endian_io.h"
#include <stdexcept>

namespace l3 {
ActorDetectionProfiles load_l3actor_detection_profiles(const uint8_t* b, size_t size) {
    if (!b || size < 32 || size > 32 + 256 * 4 ||
        b[0]!='L' || b[1]!='3' || b[2]!='D' || b[3]!='P')
        throw std::runtime_error("invalid L3DP magic/size");
    const auto count=rd_u32(b+12);
    if (rd_u16(b+4)!=1 || rd_u16(b+6)!=32 || rd_u32(b+8)!=1 ||
        count==0 || count>256 || rd_u32(b+16)!=32 || rd_u32(b+20)!=size ||
        size!=32+size_t(count)*4 || rd_u32(b+28)!=0 ||
        crc32(b+32,size-32)!=rd_u32(b+24))
        throw std::runtime_error("invalid L3DP canonical header/CRC");
    ActorDetectionProfiles profiles{};
    int last=-1;
    for(size_t i=0;i<count;++i) {
        const auto* row=b+32+i*4;
        if(row[0]<=last)throw std::runtime_error("L3DP duplicate/unordered profile");
        last=row[0];profiles[row[0]]=ActorDetectionProfile{row[1],row[2],row[3]};
    }
    return profiles;
}
std::vector<uint8_t> encode_l3actor_detection_profiles(const ActorDetectionProfiles& profiles) {
    std::vector<uint8_t> payload;
    for(size_t id=0;id<profiles.size();++id)if(profiles[id]) {
        const auto& p=*profiles[id];
        payload.insert(payload.end(),{static_cast<uint8_t>(id),p.byte0,p.byte1,p.byte2});
    }
    if(payload.empty())throw std::runtime_error("L3DP needs at least one explicit profile");
    std::vector<uint8_t> out{'L','3','D','P'};
    wr_u16(out,1);wr_u16(out,32);wr_u32(out,1);wr_u32(out,static_cast<uint32_t>(payload.size()/4));
    wr_u32(out,32);wr_u32(out,static_cast<uint32_t>(32+payload.size()));
    wr_u32(out,crc32(payload.data(),payload.size()));wr_u32(out,0);
    out.insert(out.end(),payload.begin(),payload.end());return out;
}
} // namespace l3

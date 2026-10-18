#include "legends3ds/source_buster_profile.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
std::uint16_t read16(const std::uint8_t* p) noexcept {return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8u);}
std::uint32_t read32(const std::uint8_t* p) noexcept {return std::uint32_t(read16(p))|(std::uint32_t(read16(p+2))<<16u);}
}
SourceBusterProfile load_source_buster_profile(const std::vector<std::uint8_t>& bytes,bool verify_crc){
    constexpr std::size_t header=32,payload=kSourceBusterSelectionCount*4,file=header+payload;
    if(bytes.size()!=file||bytes[0]!='L'||bytes[1]!='3'||bytes[2]!='B'||bytes[3]!='P')
        throw std::runtime_error("not L3BP");
    if(read16(bytes.data()+4)!=kSourceBusterProfileVersion||read16(bytes.data()+6)!=header||
       read32(bytes.data()+8)!=file||read32(bytes.data()+12)!=kSourceBusterSelectionCount||
       read32(bytes.data()+16)!=payload)throw std::runtime_error("unsupported L3BP");
    for(std::size_t i=24;i<header;++i)if(bytes[i])throw std::runtime_error("L3BP reserved header nonzero");
    if(verify_crc&&crc32(bytes.data()+header,payload)!=read32(bytes.data()+20))
        throw std::runtime_error("L3BP CRC mismatch");
    SourceBusterProfile out;
    for(std::size_t i=0;i<out.selection.size();++i){const auto* p=bytes.data()+header+i*4;
        if(p[1])throw std::runtime_error("L3BP reserved row nonzero");
        out.selection[i].subtype_80089a8b=p[0];
        out.selection[i].initial_counter_4fa=static_cast<std::int16_t>(read16(p+2));
    }
    return out;
}
void initialize_source_buster_fresh_lanes_80037534(
    const SourceBusterProfile& profile,SourceBusterPlayer& player) noexcept {
    for(auto& lane:player.lanes){lane.selection_170_or_171=0;
        lane.subtype_80089a8b=profile.selection[0].subtype_80089a8b;
        lane.counter_4fa=profile.selection[0].initial_counter_4fa;}
}
} // namespace l3

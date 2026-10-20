#include "legends3ds/source_buster_profile.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
std::uint16_t read16(const std::uint8_t* p) noexcept {return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8u);}
std::uint32_t read32(const std::uint8_t* p) noexcept {return std::uint32_t(read16(p))|(std::uint32_t(read16(p+2))<<16u);}
}
SourceBusterProfile load_source_buster_profile(const std::vector<std::uint8_t>& bytes,bool verify_crc){
    constexpr std::size_t rows=kSourceBusterSelectionCount*4,levels=kSourceBusterSelectionCount*kSourceBusterLevelCount*8;
    constexpr std::size_t header=32,payload=rows+levels+2*kSourceBusterLevelCount+8,file=header+payload;
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
    const auto* level=bytes.data()+header+rows;
    for(auto& weapon:out.levels)for(auto& row:weapon){
        for(auto& word:row.words){word=static_cast<std::int16_t>(read16(level));level+=2;}
    }
    for(auto& v:out.shot_radius_80097dcc)v=*level++;
    for(auto& v:out.shot_speed_80097dd4)v=*level++;
    for(auto& v:out.impact82_timers_8008a240)v=*level++;
    return out;
}
void initialize_source_buster_fresh_lanes_80037534(
    const SourceBusterProfile& profile,SourceBusterPlayer& player) noexcept {
    for(auto& lane:player.lanes){lane.selection_170_or_171=0;
        lane.subtype_80089a8b=profile.selection[0].subtype_80089a8b;
        lane.counter_4fa=profile.selection[0].initial_counter_4fa;}
}
void initialize_source_buster_lanes_8003be6c(const SourceBusterProfile& profile,
    SourceBusterPlayer& player,std::uint8_t selection_170,std::uint8_t selection_171) noexcept {
    // lanes[0] = stack10 nonzero = +170/+3C8; lanes[1] = +171/+3E0 (0x80035724).
    const std::array<std::uint8_t,2> selected{{selection_170,selection_171}};
    for(std::size_t i=0;i<2;++i){
        auto& lane=player.lanes[i];const auto row=selected[i]&15u;
        lane.selection_170_or_171=static_cast<std::int8_t>(selected[i]);
        lane.subtype_80089a8b=profile.selection[row].subtype_80089a8b;
        lane.counter_4fa=profile.selection[row].initial_counter_4fa;
    }
}
std::optional<SourceBusterShot81Stats> source_buster_shot81_stats(const SourceBusterProfile& profile,
    std::int8_t level_4fc,std::int8_t level_4fe) noexcept {
    if(level_4fc<0||level_4fe<0||std::size_t(level_4fc)>=kSourceBusterLevelCount||
       std::size_t(level_4fe)>=kSourceBusterLevelCount)return std::nullopt;
    SourceBusterShot81Stats out;
    out.damage_800375d4=profile.levels[1][std::size_t(level_4fc)].words[0];
    out.range_80037634=profile.levels[1][std::size_t(level_4fe)].words[2];
    out.radius_98=profile.shot_radius_80097dcc[std::size_t(level_4fc)];
    out.speed=profile.shot_speed_80097dd4[std::size_t(level_4fe)];
    return out;
}
} // namespace l3

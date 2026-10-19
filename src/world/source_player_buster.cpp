#include "legends3ds/source_player_buster.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace l3 {
namespace {
std::uint16_t read16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0] | (static_cast<unsigned>(p[1])<<8U));
}
std::uint32_t read32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(read16(p)) |
           (static_cast<std::uint32_t>(read16(p+2))<<16U);
}
std::int16_t signed16(std::uint16_t value) noexcept {
    return static_cast<std::int16_t>(value<0x8000U ? static_cast<std::int32_t>(value) :
                                     static_cast<std::int32_t>(value)-65536);
}
std::int16_t half_sum(std::int16_t a,std::int16_t b) noexcept {
    const auto sum=static_cast<std::int32_t>(a)+static_cast<std::int32_t>(b);
    return static_cast<std::int16_t>(sum>=0 ? sum/2 : -((-sum+1)/2));
}
void load_vertex(SourceGteRtpsState& gte,const std::array<std::int16_t,3>& p) noexcept {
    gte.data[0]=static_cast<std::uint16_t>(p[0]) |
                (static_cast<std::uint32_t>(static_cast<std::uint16_t>(p[1]))<<16U);
    gte.data[1]=static_cast<std::uint16_t>(p[2]);
}
std::int16_t low_signed(std::uint32_t value) noexcept {
    return signed16(static_cast<std::uint16_t>(value));
}
std::int16_t high_signed(std::uint32_t value) noexcept {
    return signed16(static_cast<std::uint16_t>(value>>16U));
}
std::uint32_t horizontal_distance(std::int16_t x) noexcept {
    const auto d=static_cast<std::int32_t>(x)-160;
    return static_cast<std::uint32_t>(d<0?-d:d);
}
bool spawn_ok(SourceBusterStatus status) noexcept {
    return status==SourceBusterStatus::SourceBackedPartial;
}
} // namespace

bool decode_source_player_target_profile(
    const std::vector<std::uint8_t>& bytes,SourcePlayerTargetProfile& out) noexcept {
    // v1: 12-byte window payload. v2: window + image bytes 0x80097D02/03 + pad.
    if(bytes.size()<32 || bytes[0]!='L' || bytes[1]!='3' || bytes[2]!='T' || bytes[3]!='G')return false;
    const auto version=read16(bytes.data()+4);
    const std::size_t payload=version==1?12U:version==2?16U:0U;
    if(!payload || bytes.size()!=32+payload || read16(bytes.data()+6)!=32 ||
       read32(bytes.data()+8)!=32+payload || read32(bytes.data()+12)!=payload ||
       read32(bytes.data()+16)!=crc32(bytes.data()+32,payload) ||
       read32(bytes.data()+20)!=0 || read32(bytes.data()+24)!=0 || read32(bytes.data()+28)!=0 ||
       read16(bytes.data()+42)!=0)return false;
    if(version==2 && read16(bytes.data()+46)!=0)return false;
    SourcePlayerTargetProfile next;
    next.maximum_depth=signed16(read16(bytes.data()+32));
    next.minimum_x=signed16(read16(bytes.data()+34));
    next.maximum_x=signed16(read16(bytes.data()+36));
    next.minimum_y=signed16(read16(bytes.data()+38));
    next.maximum_y=signed16(read16(bytes.data()+40));
    if(version==2)next.image_target_mode_80097d02=bytes[44];
    if(next.maximum_depth<=0 || next.minimum_x>=next.maximum_x ||
       next.minimum_y>=next.maximum_y)return false;
    out=next;return true;
}

SourcePlayerBusterResult source_player_select_and_spawn_80034eb8(
    ChildRuntime& runtime,SourceGteRtpsState& gte,SourceBusterPlayer& player,
    std::optional<std::uint32_t>& mask,const SourcePlayerBusterInputs& input) noexcept {
    SourcePlayerBusterResult out;
    if(!input.active){out.status=SourcePlayerBusterStatus::Inactive;return out;}
    if(!input.target_profile_8008994c)return out;
    if(!input.player_point_14){out.status=SourcePlayerBusterStatus::NeedsPlayerPoint;return out;}
    if(!input.target_mode_49e){out.status=SourcePlayerBusterStatus::NeedsTargetMode;return out;}
    if(!input.fire_flags_174){out.status=SourcePlayerBusterStatus::NeedsFireFlags;return out;}
    const auto& bank=runtime.hit_proxy_bank();
    if(bank.count>bank.records.size()){
        out.status=SourcePlayerBusterStatus::NeedsProxyBank;return out;
    }
    const auto lease=runtime.consume_player_leases(gte,input.camera_1f800008,mask);
    out.proxy_status=lease.status;
    if(lease.status!=SourcePlayerProxyStatus::SourceBackedPartial){
        out.status=SourcePlayerBusterStatus::LeaseHeld;return out;
    }
    load_vertex(gte,*input.player_point_14);
    const auto player_projection=source_gte_rtps_0180001(gte);
    out.proxy_status=player_projection.status;
    if(player_projection.status!=SourcePlayerProxyStatus::SourceBackedPartial){
        out.status=SourcePlayerBusterStatus::ProjectionHeld;return out;
    }
    const auto player_threshold=static_cast<std::uint32_t>(player_projection.depth)+50U;
    out.player_depth_plus_50=static_cast<std::uint16_t>(std::min<std::uint32_t>(65535U,player_threshold));
    std::array<std::uint32_t,2> best{{std::numeric_limits<std::uint32_t>::max(),
                                      std::numeric_limits<std::uint32_t>::max()}};
    const auto& profile=*input.target_profile_8008994c;
    for(std::size_t i=0;i<bank.count;++i){
        const auto& first=bank.records[i];
        const auto mode=static_cast<unsigned>(first.encoded_extent>>13U);
        if(((static_cast<std::uint32_t>(*input.target_mode_49e)<<mode)&0x80U)==0)continue;
        std::array<std::int16_t,3> point=first.point;
        std::size_t selected=i;
        if(first.encoded_extent&0x0600U){
            if(i+1>=bank.count){out.status=SourcePlayerBusterStatus::NeedsProxyBank;return out;}
            const auto& second=bank.records[++i];
            for(std::size_t axis=0;axis<3;++axis)point[axis]=half_sum(first.point[axis],second.point[axis]);
            selected=i;
        }
        const auto& selected_record=bank.records[selected];
        if(!selected_record.target ||
           (selected_record.target->source_address&0x007fffffU)!=
               (selected_record.encoded_identity&0x007fffffU)){
            out.status=SourcePlayerBusterStatus::NeedsProxyBank;return out;
        }
        if(selected_record.target->flags_04&0x80U)continue;
        load_vertex(gte,point);
        const auto projection=source_gte_rtps_0180001(gte);
        out.proxy_status=projection.status;
        if(projection.status!=SourcePlayerProxyStatus::SourceBackedPartial){
            out.status=SourcePlayerBusterStatus::ProjectionHeld;return out;
        }
        ++out.projected_proxies;
        const auto x=low_signed(projection.screen_xy),y=high_signed(projection.screen_xy);
        const auto depth=static_cast<std::int32_t>(projection.depth);
        if(static_cast<std::uint32_t>(projection.depth)>player_threshold && depth<0x500 &&
           x>=0x83 && x<=0xbd && y>=1 && y<=0xef){
            const auto distance=horizontal_distance(x);
            if(distance<best[0]){best[0]=distance;out.selected_proxy[0]=static_cast<std::int32_t>(selected);}
        }
        if(static_cast<std::uint32_t>(projection.depth)>player_threshold &&
           depth<profile.maximum_depth && x>profile.minimum_x && x<profile.maximum_x &&
           y>profile.minimum_y && y<profile.maximum_y){
            const auto distance=horizontal_distance(x);
            if(distance<best[1]){best[1]=distance;out.selected_proxy[1]=static_cast<std::int32_t>(selected);}
        }
    }
    if(*input.fire_flags_174&0x20U){
        out.spawn_attempted[0]=true;
        out.spawns[0]=runtime.spawn_source_buster_80035724(player,0,
            out.selected_proxy[0],mask);
        if(!spawn_ok(out.spawns[0].status)){out.status=SourcePlayerBusterStatus::BusterHeld;return out;}
    }
    if(*input.fire_flags_174&0x10U){
        out.spawn_attempted[1]=true;
        out.spawns[1]=runtime.spawn_source_buster_80035724(player,1,
            out.selected_proxy[1],mask);
        if(!spawn_ok(out.spawns[1].status)){out.status=SourcePlayerBusterStatus::BusterHeld;return out;}
    }
    out.status=SourcePlayerBusterStatus::SourceBackedPartial;return out;
}

} // namespace l3

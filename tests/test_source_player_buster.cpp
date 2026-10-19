#include "legends3ds/source_player_buster.h"
#include "legends3ds/l3map.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <optional>
#include <vector>

namespace { bool count_allocations=false;std::size_t allocations=0; }
void* operator new(std::size_t n){if(count_allocations)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

namespace {
using namespace l3;
void put16(std::vector<std::uint8_t>& b,std::size_t at,std::uint16_t v){b[at]=static_cast<std::uint8_t>(v);b[at+1]=static_cast<std::uint8_t>(v>>8U);}
void put32(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t v){put16(b,at,static_cast<std::uint16_t>(v));put16(b,at+2,static_cast<std::uint16_t>(v>>16U));}
std::vector<std::uint8_t> profile_bytes(){
    std::vector<std::uint8_t> b(44);b[0]='L';b[1]='3';b[2]='T';b[3]='G';put16(b,4,1);put16(b,6,32);put32(b,8,44);put32(b,12,12);
    put16(b,32,1000);put16(b,34,130);put16(b,36,190);put16(b,38,0);put16(b,40,240);put32(b,16,crc32(b.data()+32,12));return b;
}
SourceBusterPlayer player(){
    SourceBusterPlayer p;p.class_01=2;p.heading_56=4095;
    for(auto& lane:p.lanes){lane.selection_170_or_171=0;lane.muzzle_3c8_or_3e0=std::array<std::uint16_t,3>{{1,2,3}};lane.subtype_80089a8b=3;lane.counter_4fa=10;}
    return p;
}
SourceGteRtpsState gte(){
    SourceGteRtpsState g;g.projection_controls_bound=true;g.control[24]=160U<<16U;g.control[25]=120U<<16U;g.control[26]=320;return g;
}
SourcePlayerBusterInputs input(const SourcePlayerTargetProfile& profile){
    SourcePlayerBusterInputs in;in.active=true;in.target_mode_49e=0x80;in.fire_flags_174=0x30;in.player_point_14=std::array<std::int16_t,3>{{0,0,100}};in.target_profile_8008994c=profile;
    in.camera_1f800008=std::array<std::uint32_t,8>{{4096,0,4096,0,4096,0,0,0}};return in;
}
void profile_decoder(){
    auto bytes=profile_bytes();SourcePlayerTargetProfile p;assert(decode_source_player_target_profile(bytes,p));
    assert(p.maximum_depth==1000&&p.minimum_x==130&&p.maximum_x==190&&p.minimum_y==0&&p.maximum_y==240);
    assert(!p.image_target_mode_80097d02);
    // L3TG v2 appends the image bytes 0x80097D02/03 and a zero pad.
    auto v2=bytes;v2.resize(48);put16(v2,4,2);put32(v2,8,48);put32(v2,12,16);v2[44]=0x49;v2[45]=0x89;put32(v2,16,crc32(v2.data()+32,16));
    SourcePlayerTargetProfile q;assert(decode_source_player_target_profile(v2,q)&&q.image_target_mode_80097d02==0x49&&q.maximum_depth==1000);
    auto padded=v2;padded[46]=1;put32(padded,16,crc32(padded.data()+32,16));assert(!decode_source_player_target_profile(padded,q));
    for(std::size_t off:{0U,4U,6U,8U,12U,16U,20U,24U,28U,32U,42U}){auto bad=bytes;bad[off]^=1;SourcePlayerTargetProfile sentinel{1,2,3,4,5,std::nullopt};assert(!decode_source_player_target_profile(bad,sentinel));assert(sentinel.maximum_depth==1&&sentinel.maximum_y==5);}
}
void bridge(){
    SourcePlayerTargetProfile profile;assert(decode_source_player_target_profile(profile_bytes(),profile));
    ChildRuntime runtime;std::array<SourceHitProxyTarget,2> targets{};auto& bank=runtime.hit_proxy_bank();bank.count=2;
    for(std::size_t i=0;i<2;++i){targets[i].source_address=0x80003000U+static_cast<std::uint32_t>(i*4);bank.records[i].target=&targets[i];bank.records[i].encoded_identity=targets[i].source_address&0x007fffffU;bank.records[i].point={{static_cast<std::int16_t>(i?0:20),0,500}};}
    auto p=player();auto state=gte();std::optional<std::uint32_t> mask=0;auto in=input(profile);
    const auto before=allocations;count_allocations=true;const auto result=source_player_select_and_spawn_80034eb8(runtime,state,p,mask,in);count_allocations=false;
    assert(allocations==before);assert(result.status==SourcePlayerBusterStatus::SourceBackedPartial);
    assert(result.projected_proxies==2&&result.selected_proxy[0]==1&&result.selected_proxy[1]==1);
    assert(result.spawn_attempted[0]&&result.spawn_attempted[1]);
    assert(result.spawns[0].source_identity==source_buster_base&&result.spawns[1].source_identity==source_buster_base+268);
    assert(runtime.buster_pool().allocated_800989f0==0xc0000000U&&mask==0xc0000000U);
    assert(p.lanes[1].counter_4fa==9&&p.lanes[0].counter_4fa==9);
}
void guards_and_composite(){
    SourcePlayerTargetProfile profile;assert(decode_source_player_target_profile(profile_bytes(),profile));
    ChildRuntime runtime;auto p=player();auto state=gte();std::optional<std::uint32_t> mask=0;auto in=input(profile);const auto pool=runtime.buster_pool();
    in.target_profile_8008994c.reset();assert(source_player_select_and_spawn_80034eb8(runtime,state,p,mask,in).status==SourcePlayerBusterStatus::NeedsTargetProfile);assert(runtime.buster_pool().slots==pool.slots&&*mask==0);
    in=input(profile);in.active=false;assert(source_player_select_and_spawn_80034eb8(runtime,state,p,mask,in).status==SourcePlayerBusterStatus::Inactive);
    std::array<SourceHitProxyTarget,2> targets{};auto& bank=runtime.hit_proxy_bank();bank.count=2;
    for(std::size_t i=0;i<2;++i){targets[i].source_address=0x80004000U;bank.records[i].target=&targets[i];bank.records[i].encoded_identity=0x4000;bank.records[i].encoded_extent=0x0200;bank.records[i].point={{static_cast<std::int16_t>(i?10:-10),0,500}};}
    in=input(profile);in.fire_flags_174=0;state=gte();const auto result=source_player_select_and_spawn_80034eb8(runtime,state,p,mask,in);
    assert(result.status==SourcePlayerBusterStatus::SourceBackedPartial&&result.projected_proxies==1&&result.selected_proxy[0]==1&&result.selected_proxy[1]==1);
}
}
int main(){profile_decoder();bridge();guards_and_composite();std::cout<<"PASS source80034EB8 target selection -> exact80035724 Buster producer\n";}

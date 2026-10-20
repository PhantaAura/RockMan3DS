#include "legends3ds/source_buster_profile.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::size_t kPayload=64+16*8*8+8+8+8,kFile=32+kPayload;
void put16(std::vector<std::uint8_t>& b,std::size_t at,std::uint16_t v){b[at]=std::uint8_t(v);b[at+1]=std::uint8_t(v>>8u);}
void put32(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t v){put16(b,at,std::uint16_t(v));put16(b,at+2,std::uint16_t(v>>16u));}
// Authored values, not the retail tables.
std::vector<std::uint8_t> bytes(){
    std::vector<std::uint8_t> b(kFile);b[0]='L';b[1]='3';b[2]='B';b[3]='P';put16(b,4,2);put16(b,6,32);
    put32(b,8,kFile);put32(b,12,16);put32(b,16,kPayload);
    for(std::size_t i=0;i<16;++i){b[32+i*4]=std::uint8_t(0x80+i);put16(b,34+i*4,std::uint16_t(i?i*3:32767));}
    for(std::size_t w=0;w<16;++w)for(std::size_t l=0;l<8;++l)for(std::size_t k=0;k<4;++k)
        put16(b,96+(w*8+l)*8+k*2,std::uint16_t(w*100+l*10+k));
    const std::size_t tail=96+16*64;
    for(std::size_t i=0;i<8;++i){b[tail+i]=std::uint8_t(20+i);b[tail+8+i]=std::uint8_t(90+i);b[tail+16+i]=std::uint8_t(1+i);}
    put32(b,20,l3::crc32(b.data()+32,kPayload));return b;
}
bool rejected(const std::vector<std::uint8_t>& b){try{(void)l3::load_source_buster_profile(b,true);return false;}catch(const std::runtime_error&){return true;}}
}
int main(){
    const auto encoded=bytes();const auto profile=l3::load_source_buster_profile(encoded,true);
    assert(profile.selection[0].subtype_80089a8b==0x80&&profile.selection[0].initial_counter_4fa==32767);
    assert(profile.selection[15].subtype_80089a8b==0x8f&&profile.selection[15].initial_counter_4fa==45);
    assert(profile.levels[1][3].words[0]==130&&profile.levels[1][3].words[2]==132&&profile.levels[15][7].words[3]==1573);
    assert(profile.shot_radius_80097dcc[2]==22&&profile.shot_speed_80097dd4[7]==97&&profile.impact82_timers_8008a240[5]==6);
    l3::SourceBusterPlayer player;player.class_01=7;player.heading_56=1234;
    l3::initialize_source_buster_fresh_lanes_80037534(profile,player);
    for(const auto& lane:player.lanes){assert(lane.selection_170_or_171==0);assert(lane.subtype_80089a8b==0x80);assert(lane.counter_4fa==32767);assert(!lane.muzzle_3c8_or_3e0);}
    // Restore branch: lanes[0] = +170, lanes[1] = +171 (the Square lane).
    l3::initialize_source_buster_lanes_8003be6c(profile,player,0,1);
    assert(player.lanes[0].subtype_80089a8b==0x80&&player.lanes[1].subtype_80089a8b==0x81);
    assert(player.lanes[1].selection_170_or_171==1&&player.lanes[1].counter_4fa==3);
    assert(player.class_01==7&&player.heading_56==1234);
    // Weapon row 1: damage at +4FC's level, range at +4FE's level.
    const auto stats=l3::source_buster_shot81_stats(profile,2,5);
    assert(stats&&stats->damage_800375d4==120&&stats->range_80037634==152&&stats->radius_98==22&&stats->speed==95);
    assert(!l3::source_buster_shot81_stats(profile,8,0)&&!l3::source_buster_shot81_stats(profile,0,-1));
    constexpr std::size_t corruptions[]={0,4,6,8,12,16,20,24,32,33,kFile-1};
    for(const std::size_t at:corruptions){auto bad=encoded;bad[at]^=1;assert(rejected(bad));}
    std::cout<<"PASS L3BP/2 profile -> Buster lanes, weapon rows, shot 0x81 stats\n";
}

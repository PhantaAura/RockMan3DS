#include "legends3ds/source_buster_profile.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b,std::size_t at,std::uint16_t v){b[at]=std::uint8_t(v);b[at+1]=std::uint8_t(v>>8u);}
void put32(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t v){put16(b,at,std::uint16_t(v));put16(b,at+2,std::uint16_t(v>>16u));}
std::vector<std::uint8_t> bytes(){
    std::vector<std::uint8_t> b(96);b[0]='L';b[1]='3';b[2]='B';b[3]='P';put16(b,4,1);put16(b,6,32);
    put32(b,8,96);put32(b,12,16);put32(b,16,64);
    for(std::size_t i=0;i<16;++i){b[32+i*4]=std::uint8_t(0x80+i);put16(b,34+i*4,std::uint16_t(i?i*3:32767));}
    put32(b,20,l3::crc32(b.data()+32,64));return b;
}
bool rejected(const std::vector<std::uint8_t>& b){try{(void)l3::load_source_buster_profile(b,true);return false;}catch(const std::runtime_error&){return true;}}
}
int main(){
    const auto encoded=bytes();const auto profile=l3::load_source_buster_profile(encoded,true);
    assert(profile.selection[0].subtype_80089a8b==0x80&&profile.selection[0].initial_counter_4fa==32767);
    assert(profile.selection[15].subtype_80089a8b==0x8f&&profile.selection[15].initial_counter_4fa==45);
    l3::SourceBusterPlayer player;player.class_01=7;player.heading_56=1234;
    l3::initialize_source_buster_fresh_lanes_80037534(profile,player);
    for(const auto& lane:player.lanes){assert(lane.selection_170_or_171==0);assert(lane.subtype_80089a8b==0x80);assert(lane.counter_4fa==32767);assert(!lane.muzzle_3c8_or_3e0);}
    assert(player.class_01==7&&player.heading_56==1234);
    constexpr std::size_t corruptions[]={0,4,6,8,12,16,20,24,32,33,95};
    for(const std::size_t at:corruptions){auto bad=encoded;bad[at]^=1;assert(rejected(bad));}
    std::cout<<"PASS L3BP profile -> exact fresh Buster lane subset\n";
}

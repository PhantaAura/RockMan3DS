#include "legends3ds/player_runtime.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

int main(){
    constexpr std::array<std::uint8_t,8> flags{{0x86,0x82,0x45,0x41,0x86,0x82,0x45,0x41}};
    for(std::uint16_t action=0;action<4;++action){
        L3PlayerCompatWork work;std::memset(&work,0x5a,sizeof(work));
        assert(l3_pl00_fire_state_80040e9c(&work,action,0,0));assert(work.bytes[0x174]==flags[action]);
        assert(work.bytes[0x178]==0x5a&&work.bytes[0x179]==0x5a&&work.bytes[0x104]==0x5a);
    }
    for(std::uint16_t action=4;action<8;++action){
        L3PlayerCompatWork work;std::memset(&work,0x5a,sizeof(work));const auto before=work;
        assert(!l3_pl00_fire_state_80040e9c(&work,action,0,0x33));
        assert(std::memcmp(&work,&before,sizeof(work))==0);
        work.bytes[0xAC]=9;work.bytes[0xAD]=9;
        assert(l3_pl00_fire_state_80040e9c(&work,action,1,0x33));assert(work.bytes[0x174]==flags[action]);
        assert(work.bytes[0x104]==0x33&&work.bytes[0x178]==0&&work.bytes[0x179]==0&&work.bytes[0x105]==0&&work.bytes[0x100]==0);
        if((flags[action]&4u)==0u)assert(work.bytes[0xAC]==0x33&&work.bytes[0xAD]==0xff);
        else assert(work.bytes[0xAC]==9&&work.bytes[0xAD]==9);
    }
    L3PlayerCompatWork guard;std::memset(&guard,0x71,sizeof(guard));const auto before=guard;
    assert(!l3_pl00_fire_state_80040e9c(&guard,8,1,2));assert(std::memcmp(&guard,&before,sizeof(guard))==0);
    std::cout<<"PASS source80040E9C fire-state mutation and helper holds\n";
}

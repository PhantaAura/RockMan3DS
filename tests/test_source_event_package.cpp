#include "legends3ds/source_event_package.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

int main() {
    std::vector<uint8_t> bytes(l3::kSourceEventPhase2PackageBytes,0);
    bytes[0]='L';bytes[1]='3';bytes[2]='E';bytes[3]='Q';bytes[4]=1;bytes[6]=64;
    l3::patch_u32(bytes,8,1);l3::patch_u32(bytes,16,304);
    l3::patch_u32(bytes,20,uint32_t(bytes.size()));
    constexpr uint8_t digest[]={0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,
        0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0};
    std::copy(std::begin(digest),std::end(digest),bytes.begin()+32);
    l3::patch_u32(bytes,64,0x20000003);l3::patch_u32(bytes,68,7);
    l3::patch_u32(bytes,72,0x11223344);l3::patch_u32(bytes,76,0xaabbccdd);
    const auto seal=[](auto& b){l3::patch_u32(b,24,l3::crc32(b.data()+64,b.size()-64));};
    seal(bytes);
    const auto package=l3::load_source_event_phase2_package(bytes);
    assert(package.profile.initialized&&package.profile.id==7);
    assert(package.profile.pair_words[0]==0x11223344&&package.profile.pair_words[1]==0xaabbccdd);
    const auto rejected=[](const auto& b){try{(void)l3::load_source_event_phase2_package(b);}catch(const std::runtime_error&){return true;}return false;};
    for(size_t i: {size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),size_t(20),size_t(24),size_t(28),size_t(32),size_t(64)}) {
        auto bad=bytes;bad[i]^=1;if(i==64)seal(bad);assert(rejected(bad));
    }
    for(size_t i: {size_t(80),size_t(111),size_t(112),size_t(367)}) {
        auto bad=bytes;bad[i]=1;seal(bad);assert(rejected(bad));
    }
    auto truncated=bytes;truncated.pop_back();assert(rejected(truncated));
    auto trailing=bytes;trailing.push_back(0);assert(rejected(trailing));
}

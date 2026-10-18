#include "legends3ds/source_event_command_package.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>

int main() {
    std::vector<uint8_t> bytes(l3::kSourceEventCommandPackageBytes,0);
    bytes[0]='L';bytes[1]='3';bytes[2]='E';bytes[3]='C';bytes[4]=1;bytes[6]=64;
    l3::patch_u32(bytes,8,1);l3::patch_u32(bytes,16,48);l3::patch_u32(bytes,20,112);
    const std::array<uint8_t,32> digest{{
        0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,
        0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
    std::copy(digest.begin(),digest.end(),bytes.begin()+32);
    l3::patch_u32(bytes,64,17);l3::patch_u32(bytes,68,900); // Authored lookup, no retail table.
    const auto seal=[](auto& b){l3::patch_u32(b,24,l3::crc32(b.data()+64,b.size()-64));};
    seal(bytes);const auto package=l3::load_source_event_command_package(bytes);
    assert(package.profile.initialized&&package.profile.id==17&&package.profile.file_sector_80082cd0==900);
    assert(package.cold_context.byte_80098ab8==0&&package.cold_context.word_800989d8==0);
    for(auto value:package.cold_context.response_known)assert(value==1);
    const auto rejected=[](const auto& b){try{(void)l3::load_source_event_command_package(b);}catch(const std::runtime_error&){return true;}return false;};
    for(size_t offset:{size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),size_t(20),size_t(24),size_t(28),size_t(32)}) {
        auto bad=bytes;bad[offset]^=1;assert(rejected(bad));
    }
    // CRC-valid changed retained fields are rejected too, including padding.
    for(size_t offset=72;offset<112;++offset) {
        auto bad=bytes;bad[offset]=1;seal(bad);assert(rejected(bad));
    }
    auto wide_id=bytes;l3::patch_u32(wide_id,64,65536);seal(wide_id);assert(rejected(wide_id));
    auto short_package=bytes;short_package.pop_back();assert(rejected(short_package));
    auto trailing=bytes;trailing.push_back(0);assert(rejected(trailing));
}

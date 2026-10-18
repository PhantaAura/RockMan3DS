#include "legends3ds/source_combat_profile.h"
#include "legends3ds/l3map.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>

namespace { size_t allocations = 0; bool count_allocations = false; }
void* operator new(std::size_t size) {
    if (count_allocations) ++allocations;
    if (void* pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
bool rejects(const uint8_t* bytes, size_t size) {
    try { (void)l3::load_l3source_combat_profiles(bytes, size); }
    catch (const std::runtime_error&) { return true; }
    return false;
}
bool rejects(const std::vector<uint8_t>& bytes) { return rejects(bytes.data(), bytes.size()); }
void repair_crc(std::vector<uint8_t>& bytes) {
    l3::patch_u32(bytes, 24, l3::crc32(bytes.data() + 32, bytes.size() - 32));
}
void package_tests() {
    l3::SourceCombatProfiles profiles;
    profiles.count = 2;
    profiles.format_version = 1; // Preserve the inherited canonical v1 contract.
    // Authored values deliberately unrelated to either owner record.
    profiles.entries[0] = {{0x20,3,1,0,0,0}, 7, 19, 6};
    profiles.entries[1] = {{0x20,3,2,1,0,0}, 255, 211, 200};
    const auto bytes = l3::encode_l3source_combat_profiles(profiles);
    assert(bytes.size() == 56);
    count_allocations = true;
    const auto parsed = l3::load_l3source_combat_profiles(bytes.data(), bytes.size());
    assert(parsed.format_version == 1);
    for (size_t tick = 0; tick < 1200; ++tick) {
        const auto* a = l3::find_source_combat_profile(parsed, 1, 0x20, 3, 0, 0);
        const auto* b = l3::find_source_combat_profile(parsed, 2, 0x20, 3, 0, 0);
        assert(a && b && a != b);
        assert(!a->unsigned_byte0c && !b->unsigned_byte0c && !a->unsigned_byte09 && !b->unsigned_byte09);
        assert(a->initial_hp() == 112 && a->action3_work330() == 19 && a->detection_profile() == 6);
        assert(b->initial_hp() == 4080 && b->action3_work330() == 211 && b->detection_profile() == 200);
        assert(!l3::find_source_combat_profile(parsed, 0, 0x20, 3, 0, 0));
        assert(!l3::find_source_combat_profile(parsed, 255, 0x20, 3, 0, 0));
        assert(!l3::find_source_combat_profile(parsed, 1, 0x20, 3, 1, 0));
        assert(!l3::find_source_combat_profile(parsed, 1, 0x20, 3, 0, 0x40));
        assert(!l3::find_source_combat_profile(parsed, 1, 0x20, 4, 0, 0));
        assert(!l3::find_source_combat_profile(parsed, 1, 0x40, 3, 0, 0));
    }
    count_allocations = false;
    assert(allocations == 0);
    for (size_t size = 0; size < bytes.size(); ++size) assert(rejects(bytes.data(), size));
    assert(rejects(nullptr, bytes.size()));
    for (const size_t offset : {size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),
                               size_t(20),size_t(24),size_t(28),size_t(38)}) {
        auto bad = bytes; bad[offset] ^= 1; assert(rejects(bad));
    }
    auto bad = bytes; bad.push_back(0); assert(rejects(bad));
    for (const size_t offset : {size_t(32),size_t(33),size_t(34),size_t(35),size_t(36),
                               size_t(37),size_t(41),size_t(42),size_t(43)}) {
        bad = bytes; bad[offset] ^= 0x80; repair_crc(bad); assert(rejects(bad));
    }
    bad = bytes; std::copy_n(bad.begin() + 32, 12, bad.begin() + 44); repair_crc(bad); assert(rejects(bad));
    bad = bytes; for (size_t i = 0; i < 12; ++i) std::swap(bad[32+i], bad[44+i]);
    repair_crc(bad); assert(rejects(bad));
    profiles.count = 1;
    const auto sparse = l3::load_l3source_combat_profiles(
        l3::encode_l3source_combat_profiles(profiles).data(), 44);
    assert(!l3::find_source_combat_profile(sparse, 2, 0x20, 3, 0, 0));
    for (const uint32_t count : {uint32_t(0), uint32_t(17), UINT32_MAX}) {
        profiles.count = count; bool rejected = false;
        try { (void)l3::encode_l3source_combat_profiles(profiles); }
        catch (const std::runtime_error&) { rejected = true; }
        assert(rejected && !l3::find_source_combat_profile(profiles, 1, 0x20, 3, 0, 0));
    }
    std::cout << "L3CB malformed/canonical/sparse/context guards PASS;1200 allocation-free lookups\n";
}
void version2_tests() {
    l3::SourceCombatProfiles profiles;
    profiles.count = 2;
    profiles.format_version = 2;
    profiles.entries[0] = {{0x20,3,1,0,0,0}, 7, 19, 6, uint8_t(0)};
    profiles.entries[1] = {{0x20,3,2,1,0,0}, 255, 211, 200, uint8_t(255)};
    const auto bytes = l3::encode_l3source_combat_profiles(profiles);
    assert(l3::rd_u16(bytes.data() + 4) == 2 && bytes.size() == 56);
    assert(bytes[41] == 0 && bytes[42] == 1 && bytes[53] == 255 && bytes[54] == 1);
    const size_t before = allocations;
    count_allocations = true;
    const auto parsed = l3::load_l3source_combat_profiles(bytes.data(), bytes.size());
    const auto* zero = l3::find_source_combat_profile(parsed, 1, 0x20, 3, 0, 0);
    const auto* maximum = l3::find_source_combat_profile(parsed, 2, 0x20, 3, 0, 0);
    assert(parsed.format_version == 2 && zero && maximum);
    assert(!zero->unsigned_byte09 && !maximum->unsigned_byte09);
    assert(zero->unsigned_byte0c.has_value() && *zero->unsigned_byte0c == 0);
    assert(maximum->unsigned_byte0c.has_value() && *maximum->unsigned_byte0c == 255);
    count_allocations = false;
    assert(allocations == before && l3::encode_l3source_combat_profiles(parsed) == bytes);
    for (const uint8_t value : {uint8_t(0),uint8_t(1),uint8_t(127),uint8_t(128),uint8_t(255)}) {
        profiles.entries[0].unsigned_byte0c = value;
        const auto encoded = l3::encode_l3source_combat_profiles(profiles);
        const auto roundtrip = l3::load_l3source_combat_profiles(encoded.data(), encoded.size());
        assert(roundtrip.entries[0].unsigned_byte0c == value);
    }
    profiles.entries[0].unsigned_byte0c.reset();
    const auto sparse = l3::encode_l3source_combat_profiles(profiles);
    const auto mixed = l3::load_l3source_combat_profiles(sparse.data(), sparse.size());
    assert(sparse[41] == 0 && sparse[42] == 0);
    assert(!mixed.entries[0].unsigned_byte0c && mixed.entries[1].unsigned_byte0c == 255);
    assert(l3::encode_l3source_combat_profiles(mixed) == sparse);
    for (const size_t offset : {size_t(42),size_t(43),size_t(54),size_t(55)}) {
        auto bad = bytes; bad[offset] = 2; repair_crc(bad); assert(rejects(bad));
    }
    auto bad = bytes; bad[54] = 0; repair_crc(bad); assert(rejects(bad)); // absent value must be zero.
    bad = bytes; bad[4] = 1; assert(rejects(bad)); // v2 data is not reserved-zero v1.
    bad = bytes; bad[4] = 4; assert(rejects(bad));
    profiles.format_version = 1;
    bool rejected = false;
    try { (void)l3::encode_l3source_combat_profiles(profiles); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected); // Explicit old-version encode must not silently discard known byteC.
    profiles.entries[1].unsigned_byte0c.reset();
    const auto legacy = l3::encode_l3source_combat_profiles(profiles);
    const auto legacy_parsed = l3::load_l3source_combat_profiles(legacy.data(), legacy.size());
    assert(legacy_parsed.format_version == 1 && !legacy_parsed.entries[0].unsigned_byte0c &&
           !legacy_parsed.entries[1].unsigned_byte0c);
    assert(l3::encode_l3source_combat_profiles(legacy_parsed) == legacy);
    std::cout << "L3CB v1 unknown byteC/v2 explicit zero/sparse presence/unsigned boundaries PASS\n";
}
void version3_tests() {
    l3::SourceCombatProfiles profiles;
    profiles.count = 2;
    profiles.format_version = 3;
    profiles.entries[0] = {{0x20,3,1,0,0,0}, 7, 19, 6, uint8_t(0), uint8_t(0)};
    profiles.entries[1] = {{0x20,3,2,1,0,0}, 255, 211, 200, uint8_t(255), uint8_t(255)};
    const auto bytes = l3::encode_l3source_combat_profiles(profiles);
    assert(l3::rd_u16(bytes.data() + 4) == 3 && bytes.size() == 56);
    assert(bytes[41] == 0 && bytes[42] == 3 && bytes[43] == 0);
    assert(bytes[53] == 255 && bytes[54] == 3 && bytes[55] == 255);
    const auto parsed = l3::load_l3source_combat_profiles(bytes.data(), bytes.size());
    assert(parsed.format_version == 3 && parsed.entries[0].unsigned_byte09 == 0 &&
           parsed.entries[1].unsigned_byte09 == 255);
    assert(l3::encode_l3source_combat_profiles(parsed) == bytes);
    for (const uint8_t value : {uint8_t(0),uint8_t(1),uint8_t(127),uint8_t(128),uint8_t(255)}) {
        profiles.entries[0].unsigned_byte09 = value;
        const auto encoded = l3::encode_l3source_combat_profiles(profiles);
        const auto roundtrip = l3::load_l3source_combat_profiles(encoded.data(), encoded.size());
        assert(roundtrip.entries[0].unsigned_byte09 == value);
    }
    profiles.entries[0].unsigned_byte0c.reset();
    profiles.entries[1].unsigned_byte09.reset();
    const auto sparse = l3::encode_l3source_combat_profiles(profiles);
    const auto mixed = l3::load_l3source_combat_profiles(sparse.data(), sparse.size());
    assert(sparse[42] == 2 && sparse[54] == 1 && sparse[41] == 0 && sparse[55] == 0);
    assert(!mixed.entries[0].unsigned_byte0c && mixed.entries[0].unsigned_byte09 == 255);
    assert(mixed.entries[1].unsigned_byte0c == 255 && !mixed.entries[1].unsigned_byte09);
    assert(l3::encode_l3source_combat_profiles(mixed) == sparse);
    for (const uint8_t presence : {uint8_t(4),uint8_t(8),uint8_t(128),uint8_t(255)}) {
        auto bad = bytes; bad[42] = presence; repair_crc(bad); assert(rejects(bad));
    }
    auto bad = bytes; bad[54] = 1; repair_crc(bad); assert(rejects(bad));
    bad = bytes; bad[54] = 2; repair_crc(bad); assert(rejects(bad));
    bad = bytes; bad[4] = 2; assert(rejects(bad));
    for (const uint16_t version : {uint16_t(1),uint16_t(2)}) {
        profiles.format_version = version; bool rejected = false;
        try { (void)l3::encode_l3source_combat_profiles(profiles); }
        catch (const std::runtime_error&) { rejected = true; }
        assert(rejected);
    }
    profiles.format_version=5;bool rejected=false;
    try{(void)l3::encode_l3source_combat_profiles(profiles);}catch(const std::runtime_error&){rejected=true;}
    assert(rejected);
    std::cout << "L3CB v3 byte09 explicit zero/unsigned boundaries/mixed presence/old-version loss guards PASS\n";
}
void version4_tests() {
    l3::SourceCombatProfiles profiles;profiles.count=2;profiles.format_version=4;
    profiles.entries[0]={{0x20,3,1,0,0,0},7,19,6,uint8_t(0),uint8_t(0),uint32_t(0)};
    profiles.entries[1]={{0x20,3,2,1,0,0},255,211,200,uint8_t(255),uint8_t(255),uint32_t(0xffffffffu)};
    const auto bytes=l3::encode_l3source_combat_profiles(profiles);
    assert(l3::rd_u16(bytes.data()+4)==4&&l3::rd_u32(bytes.data()+16)==16&&bytes.size()==64);
    assert(bytes[42]==7&&l3::rd_u32(bytes.data()+44)==0);
    assert(bytes[58]==7&&l3::rd_u32(bytes.data()+60)==0xffffffffu);
    const auto parsed=l3::load_l3source_combat_profiles(bytes.data(),bytes.size());
    assert(parsed.format_version==4&&parsed.entries[0].reward_word00==0&&
        parsed.entries[1].reward_word00==0xffffffffu&&l3::encode_l3source_combat_profiles(parsed)==bytes);
    auto sparse=profiles;sparse.entries[0].reward_word00.reset();
    const auto sparse_bytes=l3::encode_l3source_combat_profiles(sparse);
    const auto sparse_parsed=l3::load_l3source_combat_profiles(sparse_bytes.data(),sparse_bytes.size());
    assert(!(sparse_bytes[42]&4u)&&!sparse_parsed.entries[0].reward_word00);
    auto bad=bytes;bad[58]&=uint8_t(~4u);repair_crc(bad);assert(rejects(bad));
    bad=bytes;bad[16]=12;assert(rejects(bad));
    profiles.format_version=3;bool rejected=false;
    try{(void)l3::encode_l3source_combat_profiles(profiles);}catch(const std::runtime_error&){rejected=true;}
    assert(rejected);
    std::cout<<"L3CB v4 opaque reward word00 retention/loss guards PASS\n";
}
void owner_package(const char* path, const char* expected_path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("owner combat package unavailable");
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto profiles = l3::load_l3source_combat_profiles(bytes.data(), bytes.size());
    assert(profiles.count == 2);
    for (const uint8_t selector : {uint8_t(1),uint8_t(2)}) {
        const auto* row = l3::find_source_combat_profile(profiles, selector, 0x20, 3, 0, 0);
        assert(row && row->initial_hp() == int16_t(uint16_t(row->unsigned_byte08) * 16U));
    }
    assert(l3::encode_l3source_combat_profiles(profiles) == bytes);
    std::cout << "Owner-local L3CB identity/unsigned fields/roundtrip PASS\n";
    if (expected_path) {
        std::ifstream expected(expected_path);
        if (!expected) throw std::runtime_error("owner original combat expectations unavailable");
        size_t count = 0;
        unsigned selector = 0, hp = 0, work330 = 0, profile = 0;
        while (expected >> selector >> hp >> work330 >> profile) {
            assert(selector == count + 1 && hp <= 4080 && work330 <= 255 && profile <= 255);
            const auto* row = l3::find_source_combat_profile(profiles, static_cast<uint8_t>(selector), 0x20, 3, 0, 0);
            assert(row && static_cast<unsigned>(row->initial_hp()) == hp &&
                   static_cast<unsigned>(row->action3_work330()) == work330 &&
                   row->detection_profile() == profile);
            ++count;
        }
        assert(count == 2 && expected.eof());
        std::cout << "Original initializer HP/profile and action3 work330 versus native L3CB PASS (2 source records)\n";
    }
}
} // namespace
int main(int argc, char** argv) {
    package_tests();
    version2_tests();
    version3_tests();
    version4_tests();
    if (argc == 2 || argc == 3) owner_package(argv[1], argc == 3 ? argv[2] : nullptr);
    else if (argc != 1) throw std::runtime_error("expected optional owner L3CB package and original expected TSV paths");
}

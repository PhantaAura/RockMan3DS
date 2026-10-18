#include "legends3ds/map_initialization.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <utility>

static size_t allocation_count = 0;
void* operator new(std::size_t n) { ++allocation_count; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { ++allocation_count; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
template<class F> void rejects(F action) {
    bool rejected = false;
    try { action(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
void put16(std::vector<uint8_t>& bytes, size_t at, uint16_t value) {
    bytes.at(at) = static_cast<uint8_t>(value);
    bytes.at(at + 1) = static_cast<uint8_t>(value >> 8u);
}
struct Fixture {
    l3::St00Source source;
    std::vector<uint8_t> hed, stg, map_bytes, render_bytes;
    l3::RuntimeMap map;
    l3::St00RenderSource render;
    uint32_t map_crc = 0, render_crc = 0;

    explicit Fixture(uint16_t seed = 1) : hed(16, 0), stg(256, 0) {
        source.idx.resize(3);
        for (size_t i = 0; i < source.idx.size(); ++i) {
            source.idx[i].x = static_cast<int16_t>(100 + i);
            source.idx[i].y = static_cast<int16_t>(-200 - i);
            source.idx[i].z = static_cast<int16_t>(300 + i);
        }
        l3::SourceDescriptor a, b;
        a.source_ptr = l3::kSt00StgBase + 0x20;
        b.source_ptr = l3::kSt00StgBase + 0x50;
        a.render_ptrs[0] = l3::kSt00StgBase + 0x90;
        b.render_ptrs[0] = l3::kSt00StgBase + 0xA0;
        source.descriptors = {a, b};
        source.idx[0].descriptor_ptr = a.source_ptr; // Original admission includes populated identity0.
        source.idx[1].descriptor_ptr = b.source_ptr;
        source.idx[1].raw6 = 77;                      // Exists in descriptor union, absent from HED.
        source.idx[2].descriptor_ptr = a.source_ptr;
        l3::patch_u32(hed, 4, a.source_ptr);
        put16(stg, 0x92, seed);
        put16(stg, 0xA2, 0x9002);
        rebuild();
    }
    void rebuild() {
        map_bytes = l3::cook_st00_map(source);
        render_bytes = l3::cook_l3render(l3::parse_st00_render(source, stg));
        map = l3::load_l3map(map_bytes);
        render = l3::load_l3render(render_bytes);
        map_crc = l3::crc32(map_bytes.data(), map_bytes.size());
        render_crc = l3::crc32(render_bytes.data(), render_bytes.size());
    }
    l3::MapInitialization derive() const {
        return l3::derive_map_initialization(source, hed, stg, map, render, map_crc, render_crc,
                                            l3::MapInitializationProfile::Stage0Startup);
    }
    l3::MapInitializationDiagnostics apply(const l3::MapInitialization& init) {
        return l3::apply_map_initialization(map, render, init, map_crc, render_crc);
    }
};
void crc(std::vector<uint8_t>& b) {
    l3::patch_u32(b, 44, l3::crc32(b.data() + 48, b.size() - 48));
}
void equal_original_fields(const l3::RuntimeMap& original, const l3::RuntimeMap& after) {
    assert(original.idx.size() == after.idx.size());
    for (size_t i = 0; i < original.idx.size(); ++i) {
        const auto& a = original.idx[i]; const auto& b = after.idx[i];
        assert(a.x == b.x && a.y == b.y && a.z == b.z && a.descriptor_index == b.descriptor_index);
    }
    assert(original.cells == after.cells && original.areas.size() == after.areas.size());
    assert(original.descriptors.size() == after.descriptors.size());
    assert(original.contact_lists.size() == after.contact_lists.size());
}
void synthetic_source_edges() {
    for (const uint16_t seed : {0u, 1u, 255u, 256u, 0x1234u, 0xFFFFu}) {
        Fixture f(seed);
        const auto original = f.map;
        const auto init = f.derive();
        assert(init.entries.size() == 2 && init.entries[0].idx_index == 0 && init.entries[1].idx_index == 2);
        assert(init.entries[0].seed == seed && init.entries[0].descriptor_index == 0);
        const auto bytes = l3::cook_map_initialization(init);
        assert(bytes.size() == 48 + 2 * 16);
        const size_t inspect_before = allocation_count;
        const auto header = l3::inspect_map_initialization(bytes);
        assert(inspect_before == allocation_count);
        assert(header.entry_count == 2 && header.idx_count == 3 && header.descriptor_count == 2);
        const auto decoded = l3::load_map_initialization(bytes);
        assert(l3::cook_map_initialization(decoded) == bytes);
        const size_t apply_before = allocation_count;
        const auto planned = l3::validate_map_initialization(f.map, f.render, decoded, f.map_crc, f.render_crc);
        const auto applied = f.apply(decoded);
        assert(apply_before == allocation_count);
        assert(planned.write_count == 2 && applied.write_count == 2);
        assert(planned.decoded_bytes == header.decoded_bytes && decoded.decoded_bytes == header.decoded_bytes);
        assert(static_cast<uint16_t>(f.map.idx[0].raw6) == seed && static_cast<uint16_t>(f.map.idx[2].raw6) == seed);
        assert(f.map.idx[1].raw6 == 77);
        equal_original_fields(original, f.map);
        if (seed != 0) rejects([&] { f.apply(decoded); });
    }
    Fixture masked;
    l3::patch_u32(masked.hed, 4, 0x81000000u | (masked.source.descriptors[0].source_ptr & 0xFFFFFFu));
    assert(masked.derive().entries.size() == 2);

    Fixture later;
    later.hed.resize(24, 0);
    l3::patch_u32(later.hed, 4, later.source.descriptors[1].source_ptr);
    l3::patch_u32(later.hed, 12, later.source.descriptors[0].source_ptr);
    assert(later.derive().entries.size() == 3);

    Fixture terminated;
    l3::patch_u32(terminated.hed, 4, 0);
    l3::patch_u32(terminated.hed, 12, terminated.source.descriptors[0].source_ptr);
    const auto none = terminated.derive();
    assert(none.entries.empty());
    const auto before = terminated.map;
    assert(terminated.apply(none).write_count == 0);
    for (size_t i = 0; i < before.idx.size(); ++i) assert(before.idx[i].raw6 == terminated.map.idx[i].raw6);

    Fixture capped;
    capped.hed.resize(513 * 8, 0);
    for (size_t slot = 0; slot < 513; ++slot) l3::patch_u32(capped.hed, slot * 8 + 4, l3::kSt00StgBase + 0x1000);
    l3::patch_u32(capped.hed, 511 * 8 + 4, capped.source.descriptors[0].source_ptr);
    l3::patch_u32(capped.hed, 512 * 8 + 4, capped.source.descriptors[1].source_ptr);
    const auto cap = capped.derive();
    assert(cap.entries.size() == 2 && cap.entries[1].idx_index == 2);
    capped.hed.resize(8);
    rejects([&] { capped.derive(); }); // No supplied terminator before512 cap.

    Fixture last;
    last.source.idx.assign(2048, {});
    last.source.idx[2047].descriptor_ptr = last.source.descriptors[0].source_ptr;
    last.source.idx[2047].raw6 = static_cast<int16_t>(0x8123u);
    last.rebuild();
    const auto last_init = last.derive();
    assert(last_init.entries.size() == 1 && last_init.entries[0].idx_index == 2047);
    assert(last_init.entries[0].expected_raw6 == 0x8123u);
    assert(last.apply(last_init).write_count == 1 && last.map.idx[2047].raw6 == 1);
    last.source.idx.resize(2049); last.rebuild();
    rejects([&] { last.derive(); });
}
void malformed_metadata() {
    Fixture f;
    const auto init = f.derive();
    const auto valid = l3::cook_map_initialization(init);
    rejects([&] { l3::load_map_initialization(std::vector<uint8_t>(47, 0)); });
    auto oversized = valid;
    oversized.resize(l3::kMapInitializationMaxFileBytes + 1);
    rejects([&] { l3::inspect_map_initialization(oversized); });
    // Recompute payload CRCs for field faults so validation reaches each check.
    for (const auto& field : std::vector<std::pair<size_t, uint32_t>>{
             {0, 0}, {4, 0x00300002}, {8, 0}, {12, 2049}, {16, 65536}, {20, 65536},
             {24, 2049}, {24, 3}, {36, 49}, {40, 0}, {44, 1},
             {48, 0x00000003}, {50, 0x00000002}, {52, 0x00000002}, {58, 1}, {60, 1},
             {64, 0}, {64, 0x00000003}}) {
        auto bad = valid;
        l3::patch_u32(bad, field.first, field.second);
        if (field.first != 44) crc(bad);
        rejects([&] { l3::load_map_initialization(bad); });
    }
    auto trailing = valid; trailing.push_back(0);
    l3::patch_u32(trailing, 40, static_cast<uint32_t>(trailing.size())); crc(trailing);
    rejects([&] { l3::load_map_initialization(trailing); });
    auto checksum = valid; checksum[56] ^= 1;
    rejects([&] { l3::load_map_initialization(checksum); });
    // A caller may explicitly skip only checksum verification; finite structural checks remain.
    assert(l3::load_map_initialization(checksum, false).entries.size() == 2);
    auto duplicate = init; duplicate.entries[1].idx_index = duplicate.entries[0].idx_index;
    rejects([&] { l3::cook_map_initialization(duplicate); });
    auto unsorted = init; std::swap(unsorted.entries[0], unsorted.entries[1]);
    rejects([&] { l3::cook_map_initialization(unsorted); });
}
void transactional_bindings() {
    Fixture f;
    const auto valid = f.derive();
    auto reject_unchanged = [&](const l3::MapInitialization& bad, uint32_t map_crc, uint32_t render_crc) {
        const auto before = f.map;
        rejects([&] { l3::apply_map_initialization(f.map, f.render, bad, map_crc, render_crc); });
        for (size_t i = 0; i < before.idx.size(); ++i) assert(before.idx[i].raw6 == f.map.idx[i].raw6);
        equal_original_fields(before, f.map);
    };
    reject_unchanged(valid, f.map_crc ^ 1u, f.render_crc);
    reject_unchanged(valid, f.map_crc, f.render_crc ^ 1u);
    auto bad = valid; bad.entries.back().expected_raw6 = 2; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.entries.back().descriptor_index = 1; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.entries.back().render_record_index = 1; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.entries.back().seed = 2; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.idx_count = 2; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.descriptor_count = 1; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.render_record_count = 1; reject_unchanged(bad, f.map_crc, f.render_crc);
    bad = valid; bad.profile = static_cast<l3::MapInitializationProfile>(2); reject_unchanged(bad, f.map_crc, f.render_crc);
    f.render.records[0].byte3 = 1;
    reject_unchanged(valid, f.map_crc, f.render_crc);

    Fixture source;
    source.map.idx[2].z = 99;
    rejects([&] { source.derive(); });
    source.rebuild(); source.map.idx[2].descriptor_index = 1;
    rejects([&] { source.derive(); });
    source.rebuild(); source.map.descriptors[0].render_offsets[0] ^= 4;
    rejects([&] { source.derive(); });
    source.rebuild(); std::swap(source.render.descriptors[0], source.render.descriptors[1]);
    rejects([&] { source.derive(); });
    source.rebuild(); source.render.records[0].byte2 ^= 1;
    rejects([&] { source.derive(); });
    source.rebuild(); source.hed.push_back(0);
    rejects([&] { source.derive(); });
}
}

int main() {
    synthetic_source_edges();
    malformed_metadata();
    transactional_bindings();
    std::cout << "map initialization source edges, format rejection, bindings and transactional apply passed\n";
}

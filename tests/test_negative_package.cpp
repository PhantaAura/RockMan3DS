#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
template <typename Fn> void rejects(Fn&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
struct Fixture {
    std::vector<uint8_t> mdt, idx, stg, hed;
};
Fixture fixture() {
    Fixture f;
    l3::wr_u32(f.mdt, 1);
    f.mdt.insert(f.mdt.end(), {63, 63, 4, 1});
    l3::wr_u32(f.mdt, l3::kSt00MdtBase + 12);
    l3::wr_s16(f.mdt, static_cast<int16_t>(0x8000u));
    l3::wr_s16(f.mdt, 1);
    l3::wr_s16(f.mdt, 0);
    l3::wr_s16(f.mdt, static_cast<int16_t>(0x8002u));
    f.idx.resize(24);
    l3::patch_u32(f.idx, 20, l3::kSt00StgBase + 48);
    // Both descriptors use ordinary STG streams. The default descriptor is
    // intentionally absent from HED and IDX, matching the audited omission.
    f.stg.resize(192);
    for (size_t descriptor = 0; descriptor < 2; ++descriptor) {
        const uint32_t record = l3::kSt00StgBase + 96 + static_cast<uint32_t>(descriptor) * 16;
        for (size_t slot = 0; slot < 6; ++slot)
            l3::patch_u32(f.stg, descriptor * 48 + 24 + slot * 4, record);
        const size_t ro = record - l3::kSt00StgBase;
        f.stg[ro] = 1; f.stg[ro + 1] = 1;
        l3::patch_u32(f.stg, ro + 4, 0x00123456u + static_cast<uint32_t>(descriptor));
        const uint32_t uv = l3::kSt00StgBase + 128 + static_cast<uint32_t>(descriptor) * 8;
        const uint32_t geom = l3::kSt00StgBase + 144 + static_cast<uint32_t>(descriptor) * 16;
        l3::patch_u32(f.stg, ro + 8, (uv & 0x00FFFFFFu) | 0x30000000u);
        l3::patch_u32(f.stg, ro + 12, (geom & 0x00FFFFFFu) | 0x01000000u);
        for (size_t v = 0; v < 12; ++v) f.stg[geom - l3::kSt00StgBase + 4 + v] = static_cast<uint8_t>(v * 11);
    }
    return f;
}
} // namespace

int main() {
    auto f = fixture();
    const auto source = l3::parse_st00(f.mdt, f.idx, f.stg, f.hed);
    assert(source.descriptors.size() == 2);
    assert(source.descriptors[0].source_ptr == l3::kSt00StgBase);
    assert(source.descriptors[1].source_ptr == l3::kSt00StgBase + 48);
    const auto render = l3::parse_st00_render(source, f.stg);
    assert(render.default_negative_descriptor == 0);
    assert(render.records.size() == 2 && render.streams.size() == 2);
    const auto map = l3::load_l3map(l3::cook_st00_map(source));
    assert(map.descriptors.size() == render.descriptors.size());
    assert(map.idx[1].descriptor_index == 1);

    const auto cooked = l3::cook_l3render(render);
    const auto header = l3::inspect_l3render(cooked);
    assert(header.version == 3 && header.header_bytes == 80 && header.flags == 15);
    assert(header.default_negative_descriptor == 0 && l3::rd_u32(&cooked[68]) == 0);
    const auto runtime = l3::load_l3render(cooked);
    assert(runtime.default_negative_descriptor == 0);
    assert(runtime.records[0].materials == render.records[0].materials);
    assert(runtime.streams[0].primitives.size() == 1);

    // A retained v3 package without the optional binding remains readable and
    // explicitly reports the unknown/default-unavailable identity.
    auto legacy = render;
    legacy.default_negative_descriptor = 0xFFFFu;
    const auto old_cooked = l3::cook_l3render(legacy);
    assert(l3::rd_u32(&old_cooked[8]) == 7 && l3::rd_u32(&old_cooked[68]) == 0);
    assert(l3::load_l3render(old_cooked).default_negative_descriptor == 0xFFFFu);

    auto invalid = cooked;
    l3::patch_u32(invalid, 68, 2);
    rejects([&] { l3::inspect_l3render(invalid); });
    l3::patch_u32(invalid, 68, 0xFFFFu);
    rejects([&] { l3::load_l3render(invalid); });
    invalid = old_cooked; l3::patch_u32(invalid, 68, 1);
    rejects([&] { l3::inspect_l3render(invalid); });
    for (size_t offset : {72u, 76u}) {
        invalid = cooked; l3::patch_u32(invalid, offset, 1);
        rejects([&] { l3::inspect_l3render(invalid); });
    }
    invalid = cooked; l3::patch_u32(invalid, 8, 63);
    rejects([&] { l3::inspect_l3render(invalid); });
    auto invalid_source = render; invalid_source.default_negative_descriptor = 2;
    rejects([&] { l3::cook_l3render(invalid_source); });

    // Bit7-set cells never cause a guessed descriptor binding. A source with
    // no default-negative cells cooks exactly the inherited descriptor set.
    auto special = f;
    special.mdt[12] = 0x80; special.mdt[18] = 0x80;
    const auto special_source = l3::parse_st00(special.mdt, special.idx, special.stg, special.hed);
    assert(special_source.descriptors.size() == 1);
    assert(l3::parse_st00_render(special_source, special.stg).default_negative_descriptor == 0xFFFFu);
    auto missing = source;
    missing.descriptors.erase(missing.descriptors.begin());
    rejects([&] { l3::parse_st00_render(missing, f.stg); });
    auto short_stg = f.stg;
    short_stg.resize(47);
    std::vector<uint8_t> no_idx;
    rejects([&] { l3::parse_st00(f.mdt, no_idx, short_stg, f.hed); });
    auto bad_stg = f.stg;
    l3::patch_u32(bad_stg, 24, l3::kSt00StgBase + static_cast<uint32_t>(bad_stg.size()));
    const auto bad_source = l3::parse_st00(f.mdt, f.idx, bad_stg, f.hed);
    rejects([&] { l3::parse_st00_render(bad_source, bad_stg); });

    // Referenced HD2 row1 supplies a third descriptor that is also absent
    // from HED/IDX. Its opaque tag is not used to infer any gameplay meaning.
    auto with_hd2 = f;
    with_hd2.mdt[12] = 0x84;
    with_hd2.stg.resize(240);
    for (size_t slot = 0; slot < 6; ++slot)
        l3::patch_u32(with_hd2.stg, 192 + 24 + slot * 4, l3::kSt00StgBase + 112);
    std::vector<uint8_t> hd2(16);
    l3::patch_u32(hd2, 8, 0x12345678u);
    l3::patch_u32(hd2, 12, l3::kSt00StgBase + 192);
    const auto hd2_source = l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed, hd2);
    assert(hd2_source.descriptors.size() == 3);
    assert(hd2_source.special_negative_descriptor_ptrs[1] == l3::kSt00StgBase + 192);
    const auto hd2_render = l3::parse_st00_render(hd2_source, with_hd2.stg);
    assert(hd2_render.default_negative_descriptor == 0 && hd2_render.special_negative_descriptors[1] == 2);
    assert(hd2_render.special_negative_descriptors[0] == 0xFFFFu);
    const auto hd2_map = l3::load_l3map(l3::cook_st00_map(hd2_source));
    assert(hd2_map.descriptors.size() == hd2_render.descriptors.size() && hd2_map.idx[1].descriptor_index == 1);
    const auto hd2_cooked = l3::cook_l3render(hd2_render);
    const auto hd2_header = l3::inspect_l3render(hd2_cooked);
    assert(hd2_header.flags == 31 && hd2_header.special_negative_descriptor_count == 32);
    assert(hd2_header.off_special_negative_descriptors == hd2_header.off_descriptors + 3 * 12);
    assert(hd2_header.off_records == hd2_header.off_special_negative_descriptors + 64);
    const auto hd2_runtime = l3::load_l3render(hd2_cooked);
    assert(hd2_runtime.special_negative_descriptors == hd2_render.special_negative_descriptors);
    const auto absent_hd2 = l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed);
    assert(absent_hd2.descriptors.size() == 2 && absent_hd2.special_negative_descriptor_ptrs[1] == 0);
    auto zero_hd2 = hd2; l3::patch_u32(zero_hd2, 12, 0);
    assert(l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed, zero_hd2).descriptors.size() == 2);
    auto unused_bad_row = hd2; l3::patch_u32(unused_bad_row, 4, 1);
    assert(l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed, unused_bad_row).descriptors.size() == 3);
    auto bad_hd2 = hd2; l3::patch_u32(bad_hd2, 12, 1);
    rejects([&] { l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed, bad_hd2); });
    bad_hd2 = hd2; bad_hd2.resize(8);
    rejects([&] { l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed, bad_hd2); });
    bad_hd2 = hd2; bad_hd2.push_back(0);
    rejects([&] { l3::parse_st00(with_hd2.mdt, with_hd2.idx, with_hd2.stg, with_hd2.hed, bad_hd2); });
    invalid = hd2_cooked;
    invalid[hd2_header.off_special_negative_descriptors + 2] = 3;
    invalid[hd2_header.off_special_negative_descriptors + 3] = 0;
    rejects([&] { l3::inspect_l3render(invalid, false); });
    invalid = hd2_cooked; l3::patch_u32(invalid, 72, hd2_header.off_special_negative_descriptors + 2);
    rejects([&] { l3::inspect_l3render(invalid); });
    invalid = hd2_cooked; l3::patch_u32(invalid, 76, 31);
    rejects([&] { l3::load_l3render(invalid); });
    auto bad_special_identity = hd2_render; bad_special_identity.special_negative_descriptors[1] = 3;
    rejects([&] { l3::cook_l3render(bad_special_identity); });
    auto missing_hd2_source = hd2_source; missing_hd2_source.descriptors.pop_back();
    rejects([&] { l3::parse_st00_render(missing_hd2_source, with_hd2.stg); });
    std::cout << "negative descriptor admission, default/HD2 neutral bindings, old-v3 compatibility and malformed rejection PASS\n";
}

#include "legends3ds/render/negative_materials.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
template <typename Fn> void rejects(Fn&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    l3::NegativeMaterialTables source(2);
    source[0][2] = {0x1200001du, 12, true, {{1, 2, 9, 63}}, true};
    source[1][2] = {0x1240001du, 63, true};
    const auto cooked = l3::cook_l3negative_materials(source);
    assert(cooked.size() == 16u + 2u * 32u * 12u && cooked[4] == 2);
    const auto loaded = l3::load_l3negative_materials(cooked);
    assert(loaded.size() == 2 && loaded[0][2].known && !loaded[0][1].known);
    assert(loaded[0][2].material_word == source[0][2].material_word);
    for (uint16_t rotation = 0; rotation < 4; ++rotation) {
        const auto* binding = l3::find_negative_material_binding(loaded, 0, uint16_t(0x8008u | rotation));
        assert(binding && binding->atlas_tile == 12);
        const auto uv = l3::negative_material_quad_uv(*binding);
        assert((uv == std::array<uint8_t, 8>{{128, 32, 159, 32, 128, 63, 159, 63}}));
    }
    const auto last_uv = l3::negative_material_quad_uv(loaded[1][2]);
    assert((last_uv == std::array<uint8_t, 8>{{224, 224, 255, 224, 224, 255, 255, 255}}));
    assert(!l3::find_negative_material_binding(loaded, 2, 0x8008));
    assert(!l3::find_negative_material_binding(loaded, 0, 0x0088));
    assert(!l3::find_negative_material_binding(loaded, 0, 0x8088));
    assert(!l3::find_negative_material_binding(loaded, 0, 0x8004));
    rejects([&] { l3::negative_material_quad_uv(loaded[0][0]); });
    // Near LOD (0x80021B14 / 0x8002235C): stream UV + tile offset; faces with
    // more than three after them use near_tiles[face + 8 - count].
    assert(loaded[0][2].near_known && loaded[0][2].near_tiles[3] == 63 && !loaded[1][2].near_known);
    const std::array<uint8_t, 8> stream{{0, 0, 31, 0, 0, 15, 31, 15}};
    auto nuv = l3::negative_material_near_uv(loaded[0][2], stream, 0, 8);    // near_tiles[0] = 1
    assert((nuv == std::array<uint8_t, 8>{{32, 0, 63, 0, 32, 15, 63, 15}}));
    nuv = l3::negative_material_near_uv(loaded[0][2], stream, 1, 6);         // near_tiles[3] = 63
    assert((nuv == std::array<uint8_t, 8>{{224, 224, 255, 224, 224, 239, 255, 239}}));
    nuv = l3::negative_material_near_uv(loaded[0][2], stream, 4, 8);         // atlas_tile 12
    assert(nuv[0] == 128 && nuv[1] == 32);
    rejects([&] { l3::negative_material_near_uv(loaded[1][2], stream, 0, 1); });
    rejects([&] { l3::negative_material_near_uv(loaded[0][2], stream, 3, 3); });
    // A v1 pack (8-byte rows, no near tiles) still loads.
    {
        std::vector<uint8_t> v1 = {'L', '3', 'N', 'M'};
        l3::wr_u16(v1, 1); l3::wr_u16(v1, 1); l3::wr_u32(v1, 16u + 32u * 8u); l3::wr_u32(v1, 0);
        for (size_t i = 0; i < 32; ++i) {
            l3::wr_u32(v1, i == 2 ? 0x1200001du : 0u);
            v1.push_back(i == 2 ? 12 : 0); v1.push_back(i == 2 ? 1 : 0); l3::wr_u16(v1, 0);
        }
        const auto t = l3::load_l3negative_materials(v1, false);
        assert(t[0][2].known && t[0][2].atlas_tile == 12 && !t[0][2].near_known);
    }
    rejects([] { l3::cook_l3negative_materials({}); });
    rejects([] { l3::cook_l3negative_materials(l3::NegativeMaterialTables(257)); });
    auto bad = source;
    bad[0][0].material_word = 1;
    rejects([&] { l3::cook_l3negative_materials(bad); });
    bad = source; bad[0][2].atlas_tile = 64;
    rejects([&] { l3::cook_l3negative_materials(bad); });
    bad = source; bad[0][2].material_word = 0x1200001cu;
    rejects([&] { l3::cook_l3negative_materials(bad); });
    auto bytes = cooked;
    bytes[16] ^= 1;
    rejects([&] { l3::load_l3negative_materials(bytes); });
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; bytes.push_back(0);
    rejects([&] { l3::load_l3negative_materials(bytes); });
    bytes = cooked; bytes[16 + 6] = 1;
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; bytes[16 + 5] = 4;
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; l3::patch_u32(bytes, 8, uint32_t(bytes.size()) - 1u);
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; bytes[6] = 0; bytes[7] = 0;
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; bytes.resize(15);
    rejects([&] { l3::load_l3negative_materials(bytes); });
    std::cout << "negative material tables: area/slot/UV/unknown/bounded malformed rejection PASS\n";
}

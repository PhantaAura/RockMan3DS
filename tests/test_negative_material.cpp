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
    source[0][2] = {0x1200001du, 12, true};
    source[1][2] = {0x1240001du, 63, true};
    const auto cooked = l3::cook_l3negative_materials(source);
    assert(cooked.size() == 16u + 2u * 32u * 8u);
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
    bytes = cooked; bytes[16 + 5] = 2;
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; l3::patch_u32(bytes, 8, uint32_t(bytes.size()) - 1u);
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; bytes[6] = 0; bytes[7] = 0;
    rejects([&] { l3::load_l3negative_materials(bytes, false); });
    bytes = cooked; bytes.resize(15);
    rejects([&] { l3::load_l3negative_materials(bytes); });
    std::cout << "negative material tables: area/slot/UV/unknown/bounded malformed rejection PASS\n";
}

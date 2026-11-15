// Type-10 palette patch (shared handler 0x8001BF44): a non-banked type 10
// uploads RECT(x, y, 64, entries * rows / 64) (0x8001C024), so 16-colour
// CLUT k lands at (x + 16 * (k % 4), y + k / 4). Authored resource bytes.
#include "legends3ds/material_resolver.h"
#include "legends3ds/ps1_texture.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace {
void put32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int k = 0; k < 4; ++k) b[o + size_t(k)] = uint8_t(v >> (8 * k)); }
std::vector<uint8_t> resource(uint32_t type, uint32_t cx, uint32_t cy) {
    std::vector<uint8_t> b(0x800, 0);
    put32(b, 0, type); put32(b, 4, 0x20 + 16 * 8 * 2); put32(b, 8, 0x12);
    put32(b, 12, cx); put32(b, 16, cy); put32(b, 20, 16); put32(b, 24, 8);
    std::memcpy(b.data() + 0x40, "..\\T.TIM", 9);
    for (int i = 0; i < 128; ++i) { b[0x100 + 2 * i] = uint8_t(i); b[0x101 + 2 * i] = 0x80; }
    return b;
}
} // namespace

int main() {
    const auto t10 = l3::parse_mml_type9_palette(resource(10, 832, 248), 0);
    assert(t10.physical_width == 64 && !t10.palette_row_banked);
    // CLUT 7 -> (880, 249) = GetClut(880, 249) 0x3E77; CLUT 0 -> (832, 248); CLUT 4 -> (832, 249).
    assert(l3::palette_patch_row(t10, uint16_t((249 << 6) | (880 >> 4))) == 7);
    assert(l3::palette_patch_row(t10, uint16_t((248 << 6) | (832 >> 4))) == 0);
    assert(l3::palette_patch_row(t10, uint16_t((249 << 6) | (832 >> 4))) == 4);
    assert(l3::palette_patch_row(t10, uint16_t((250 << 6) | (832 >> 4))) == -1);
    // Type 9 keeps the vertical RECT(x, y, 16, 8).
    const auto t9 = l3::parse_mml_type9_palette(resource(9, 832, 248), 0);
    assert(t9.physical_width == 0 && l3::palette_patch_row(t9, uint16_t((255 << 6) | (832 >> 4))) == 7);
    assert(l3::palette_patch_row(t9, uint16_t((249 << 6) | (880 >> 4))) == -1);
    // The scanner now keeps type 10.
    std::vector<uint8_t> container = resource(10, 832, 248);
    const auto found = l3::scan_mml_indexed_resources(container);
    assert(found.size() == 1 && found[0].kind == l3::IndexedResourceKind::PaletteType9 && found[0].palette9.physical_width == 64);
    std::cout << "palette_type10: ok\n";
    return 0;
}

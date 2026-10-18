#include "legends3ds/material_cache.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
constexpr uint16_t kPage = 0x001du, kClut = uint16_t(480u << 6);
constexpr uint32_t kMaterial = (uint32_t(kClut) << 16) | kPage;
l3::IndexedResource image(uint16_t x, uint16_t y, uint16_t width_words,
                          uint16_t height, uint16_t bits, uint8_t bpp = 4) {
    l3::IndexedResource resource;
    resource.kind = l3::IndexedResourceKind::TextureType1;
    auto& texture = resource.texture;
    texture.image_x_words = x; texture.image_y = y;
    texture.width_words = width_words; texture.height = height;
    texture.width_pixels = uint16_t(width_words * (bpp == 4 ? 4u : 2u));
    texture.bpp = bpp;
    texture.packed_indices.resize(size_t(width_words) * height * 2u);
    for (size_t i = 0; i < texture.packed_indices.size(); i += 2) {
        texture.packed_indices[i] = uint8_t(bits);
        texture.packed_indices[i + 1] = uint8_t(bits >> 8);
    }
    return resource;
}
l3::IndexedResource palette(uint16_t x, uint16_t y, uint16_t width, uint16_t first) {
    l3::IndexedResource resource;
    resource.kind = l3::IndexedResourceKind::ClutType4;
    auto& patch = resource.clut4;
    patch.clut_x = x; patch.clut_y = y;
    patch.palette_entries = width; patch.palette_rows = 1;
    patch.bpp = width == 16 ? 4 : 8;
    for (uint16_t i = 0; i < width; ++i) patch.palette_ps1.push_back(uint16_t(first + i));
    return resource;
}
l3::IndexedResource bank_palette(uint16_t x, uint16_t y, uint16_t stored_rows, uint16_t first) {
    auto resource = palette(x, y, 16, first);
    auto& patch = resource.clut4;
    patch.palette_row_banked = true;
    patch.palette_rows = stored_rows;
    for (uint16_t i = 16; i < uint16_t(16 * stored_rows); ++i)
        patch.palette_ps1.push_back(uint16_t(first + i));
    return resource;
}
uint32_t material_clut(uint16_t x, uint16_t y) {
    return (uint32_t(uint16_t((y << 6) | (x / 16))) << 16) | kPage;
}
template <typename Fn> void rejects(Fn&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    // Atlas stripes compose into one physical texture page; a later overlapping
    // stripe replaces only its rectangle, preserving earlier unaffected rows.
    std::vector<l3::IndexedResource> uploads{
        image(832, 256, 64, 32, 0x1111), image(832, 288, 64, 32, 0x2222),
        image(832, 320, 64, 64, 0x3333), image(832, 352, 64, 32, 0x4444),
        palette(0, 480, 16, 0x100)};
    std::vector<l3::MaterialVramDiagnostics> diagnostics;
    auto cache = l3::build_material_cache({kMaterial, kMaterial}, uploads, &diagnostics);
    assert(cache.entries.size() == 1 && cache.entries[0].resolved);
    const auto& entry = cache.entries[0];
    assert(entry.source_width == 256 && entry.source_height == 128);
    for (size_t row = 0; row < 256; ++row) {
        const uint8_t expected = row < 32 ? 1 : row < 64 ? 2 : row < 96 ? 3 : row < 128 ? 4 : 0;
        assert(entry.indices[row * 256] == expected && entry.indices[row * 256 + 255] == expected);
    }
    assert(diagnostics.size() == 1 && diagnostics[0].image_words_written == 64u * 128u);
    assert(diagnostics[0].image_words_total == 64u * 256u && diagnostics[0].palette_words_written == 16);
    // A partial patch at a page-internal X offset updates precisely those words.
    uploads.push_back(image(848, 272, 4, 2, 0x7654, 8));
    cache = l3::build_material_cache({kMaterial}, uploads);
    const auto& changed = cache.entries[0];
    assert(changed.indices[16u * 256u + 63] == 1 && changed.indices[16u * 256u + 80] == 1);
    assert(changed.indices[16u * 256u + 64] == 4 && changed.indices[16u * 256u + 65] == 5);
    assert(changed.indices[16u * 256u + 66] == 6 && changed.indices[16u * 256u + 67] == 7);
    assert(changed.indices[17u * 256u + 79] == 7 && changed.indices[18u * 256u + 64] == 1);
    // A wider CLUT upload can supply a smaller requested physical row; later
    // partial writes replace overlapping colors even with different source BPP.
    uploads.push_back(palette(0, 480, 256, 0x200));
    uploads.push_back(palette(8, 480, 16, 0x300));
    cache = l3::build_material_cache({kMaterial}, uploads);
    for (size_t i = 0; i < 8; ++i) assert(cache.entries[0].palette_ps1[i] == 0x200u + i);
    for (size_t i = 8; i < 16; ++i) assert(cache.entries[0].palette_ps1[i] == 0x300u + i - 8u);
    // Image and CLUT share physical VRAM: an image upload at the CLUT location
    // can replace those words, with no resource-name or unrelated-palette guess.
    uploads.push_back(image(0, 480, 16, 1, 0x4321));
    cache = l3::build_material_cache({kMaterial}, uploads);
    for (const auto color : cache.entries[0].palette_ps1) assert(color == 0x4321);
    const auto cooked = l3::cook_l3material_cache(cache);
    const auto loaded = l3::load_l3material_cache(cooked);
    assert(loaded.entries[0].indices == cache.entries[0].indices);
    assert(loaded.entries[0].palette_ps1 == cache.entries[0].palette_ps1);
    // An image need not start at the TPAGE origin; clip it to the requested page.
    cache = l3::build_material_cache({kMaterial},
        {image(828, 252, 8, 8, 0x5555), palette(0, 480, 16, 0x100)}, &diagnostics);
    assert(cache.entries[0].resolved && cache.entries[0].source_width == 16 && cache.entries[0].source_height == 4);
    assert(cache.entries[0].indices[0] == 5 && cache.entries[0].indices[3u * 256u + 15] == 5);
    assert(cache.entries[0].indices[16] == 0 && diagnostics[0].image_words_written == 16);
    // A neighboring palette alone must never resolve a missing requested CLUT.
    cache = l3::build_material_cache({kMaterial},
        {image(832, 256, 64, 32, 0x1111), palette(16, 480, 16, 0x100)}, &diagnostics);
    assert(!cache.entries[0].resolved && cache.entries[0].indices.empty());
    assert(diagnostics[0].palette_words_written == 0);
    // An incomplete overlapping CLUT stays unresolved rather than filling colors.
    cache = l3::build_material_cache({kMaterial},
        {image(832, 256, 64, 32, 0x1111), palette(8, 480, 16, 0x100)}, &diagnostics);
    assert(!cache.entries[0].resolved && diagnostics[0].palette_words_written == 8);
    auto malformed = image(1020, 500, 8, 8, 0x1111);
    rejects([&] { l3::build_material_cache({kMaterial}, {malformed}); });
    malformed = image(832, 256, 64, 32, 0x1111); malformed.texture.packed_indices.pop_back();
    rejects([&] { l3::build_material_cache({kMaterial}, {malformed}); });
    auto wrong_palette = palette(0, 480, 16, 0x100); wrong_palette.clut4.palette_ps1.pop_back();
    rejects([&] { l3::build_material_cache({kMaterial}, {wrong_palette}); });

    // Stored sixteen-by-eight CLUT banks at Y510/511 occupy one horizontal
    // physical row. All colors survive; no vertical clipping/wrapping occurs.
    const auto banked = bank_palette(128, 510, 8, 0x500);
    const auto bank_word = material_clut(144, 510);
    cache = l3::build_material_cache({bank_word,material_clut(144,511)},
        {image(832,256,64,32,0x1111),banked}, &diagnostics);
    const auto* bank_entry = l3::find_material_cache_entry(cache,bank_word);
    assert(bank_entry && bank_entry->resolved);
    for (size_t i = 0; i < 16; ++i) assert(bank_entry->palette_ps1[i] == 0x510u+i);
    assert(l3::palette_patch_row(banked.clut4,uint16_t((510u<<6)|9u)) == 1);
    assert(!l3::find_material_cache_entry(cache,material_clut(144,511))->resolved);

    // A CPU copy crossing bank15->16 is contiguous RAM. Only its starting
    // row is dirtied, so the next row stays GPU-unknown until another copy
    // dirties it. Its physical mapping jumps from Y255 to Y496.
    const auto crossing = bank_palette(240,255,2,0x600);
    std::vector<l3::IndexedResource> bank_uploads{image(832,256,64,32,0x1111),crossing};
    const uint32_t before_word = material_clut(240,255), after_word = material_clut(0,496);
    cache = l3::build_material_cache({before_word,after_word},bank_uploads);
    assert(l3::find_material_cache_entry(cache,before_word)->resolved);
    assert(!l3::find_material_cache_entry(cache,after_word)->resolved);
    bank_uploads.push_back(bank_palette(16,496,1,0x700));
    // This direct image write occurs later in resource order, but bank staging
    // follows the whole initial load batch and restores the retained CPU tail.
    bank_uploads.push_back(image(0,496,16,1,0x4321));
    cache = l3::build_material_cache({after_word},bank_uploads);
    assert(cache.entries[0].resolved);
    for (size_t i = 0; i < 16; ++i) assert(cache.entries[0].palette_ps1[i] == 0x610u+i);
    auto outside_bank = bank_palette(240,511,2,0x800);
    rejects([&] { l3::build_material_cache({bank_word},{outside_bank}); });
    auto direct = palette(320,511,16,0x800);
    direct.clut4.palette_rows=2;
    direct.clut4.palette_ps1.resize(32);
    rejects([&] { l3::build_material_cache({bank_word},{direct}); });
    std::cout << "material VRAM: stripes/last writes/CLUT banks/deferred staging/unknown tails/strict bounds PASS\n";
}

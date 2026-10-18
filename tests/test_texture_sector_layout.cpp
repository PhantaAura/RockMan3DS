#include "legends3ds/endian_io.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/ps1_texture.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
uint8_t authored_pixel(uint32_t x, uint32_t y, uint8_t bpp) {
    return bpp == 4 ? uint8_t(1u + (x * 3u + y * 7u) % 15u)
                    : uint8_t((x * 17u + y * 11u + 3u) & 255u);
}
uint16_t authored_word(uint32_t word_x, uint32_t y, uint8_t bpp) {
    const uint32_t pixels = 16u / bpp;
    uint16_t word = 0;
    for (uint32_t i = 0; i < pixels; ++i)
        word |= uint16_t(uint16_t(authored_pixel(word_x * pixels + i, y, bpp)) << (i * bpp));
    return word;
}
std::vector<uint8_t> source_texture(uint32_t width_words, uint32_t height, uint8_t bpp) {
    // Entirely authored fixture: sequential sectors, each one independent
    // 32-word x32-row source RECT. Colors and coordinates are not retail data.
    const uint32_t colors = bpp == 4 ? 16u : 256u;
    std::vector<uint8_t> bytes(2048u + size_t(width_words) * height * 2u);
    l3::patch_u32(bytes, 0, 1);
    l3::patch_u32(bytes, 4, 32u + colors * 2u + width_words * height * 2u);
    l3::patch_u32(bytes, 8, 0x12345678u);
    l3::patch_u32(bytes, 12, 0); l3::patch_u32(bytes, 16, 240);
    l3::patch_u32(bytes, 20, colors); l3::patch_u32(bytes, 24, 1);
    l3::patch_u32(bytes, 28, 320); l3::patch_u32(bytes, 32, 0);
    l3::patch_u32(bytes, 36, width_words); l3::patch_u32(bytes, 40, height);
    for (uint32_t i = 0; i < colors; ++i) {
        bytes[256u + i * 2u] = uint8_t(i);
        bytes[257u + i * 2u] = 0x80;
    }
    for (uint32_t sector = 0; sector < (width_words / 32u) * (height / 32u); ++sector) {
        const uint32_t tile_x = sector % (width_words / 32u);
        const uint32_t tile_y = sector / (width_words / 32u);
        for (uint32_t word = 0; word < 1024u; ++word) {
            const uint16_t value = authored_word(tile_x * 32u + word % 32u,
                                                 tile_y * 32u + word / 32u, bpp);
            const size_t offset = 2048u + size_t(sector) * 2048u + word * 2u;
            bytes[offset] = uint8_t(value); bytes[offset + 1u] = uint8_t(value >> 8u);
        }
    }
    return bytes;
}
void reject(const std::vector<uint8_t>& bytes) {
    bool rejected = false;
    try { (void)l3::parse_mml_type1_texture(bytes, 0); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    size_t cases = 0;
    for (uint8_t bpp : {uint8_t(4), uint8_t(8)}) {
        for (const auto dimensions : {std::pair<uint32_t,uint32_t>{32,32}, {64,32},
                                     {32,64}, {64,64}, {64,256}, {128,64}}) {
            const auto bytes = source_texture(dimensions.first, dimensions.second, bpp);
            const auto texture = l3::parse_mml_type1_texture(bytes, 0);
            const auto pixels = l3::unpack_texture_indices(texture);
            const uint32_t width_pixels = dimensions.first * (16u / bpp);
            assert(texture.width_pixels == width_pixels && texture.height == dimensions.second);
            assert(texture.source_arg2 == 0x12345678u);
            for (uint32_t y = 0; y < dimensions.second; ++y) {
                for (uint32_t x = 0; x < dimensions.first; ++x)
                    assert(l3::rd_u16(texture.packed_indices.data() +
                           (size_t(y) * dimensions.first + x) * 2u) == authored_word(x,y,bpp));
                for (uint32_t x = 0; x < width_pixels; ++x)
                    assert(pixels[size_t(y) * width_pixels + x] == authored_pixel(x,y,bpp));
            }
            // A single source sector already has the canonical layout.
            if (dimensions.first == 32 && dimensions.second == 32)
                assert(std::equal(texture.packed_indices.begin(),texture.packed_indices.end(),bytes.begin()+2048));
            const auto neutral = l3::cook_l3texture(texture);
            const auto header = l3::inspect_l3texture(neutral,true);
            assert(header.width_words == dimensions.first && header.height == dimensions.second);
            assert(std::equal(texture.packed_indices.begin(),texture.packed_indices.end(),
                              neutral.begin()+header.off_indices));
            // Verify the same canonical words reach stateful VRAM composition,
            // including its CLUT and 4/8bpp nibble/byte interpretation.
            l3::IndexedResource resource;
            resource.kind=l3::IndexedResourceKind::TextureType1; resource.texture=texture;
            const uint32_t material=(uint32_t(240u<<6u)<<16u)|5u|(bpp==8 ? 0x80u : 0u);
            const auto cache=l3::build_material_cache({material},{resource});
            assert(cache.entries[0].resolved);
            for (uint32_t y=0;y<std::min(dimensions.second,256u);++y)
                for (uint32_t x=0;x<std::min(width_pixels,256u);++x)
                    assert(cache.entries[0].indices[size_t(y)*256u+x]==authored_pixel(x,y,bpp));
            ++cases;
        }
    }
    const auto valid=source_texture(64,64,4);
    for (const auto malformed : {std::pair<size_t,uint32_t>{36,0}, {40,0}, {36,31},
                                {36,33}, {40,31}, {40,63}, {28,1000}, {32,500},
                                {36,65536}, {40,65536}, {28,0xffffffffu}, {32,0xffffffffu}}) {
        auto bytes=valid; l3::patch_u32(bytes,malformed.first,malformed.second); reject(bytes);
    }
    auto short_payload=valid; short_payload.pop_back(); reject(short_payload);
    auto wrong_size=valid; l3::patch_u32(wrong_size,4,4096); reject(wrong_size);
    auto prefixed=valid; prefixed.insert(prefixed.begin(),2048u,0);
    const auto offset_texture=l3::parse_mml_type1_texture(prefixed,2048u);
    assert(offset_texture.packed_indices==l3::parse_mml_type1_texture(valid,0).packed_indices);
    std::cout << "source sector layout: " << cases
              << " authored4/8bpp single/multi-tile cases; every word/pixel/cooked/material path,"
                 " offsets and14 malformed cases PASS\n";
}

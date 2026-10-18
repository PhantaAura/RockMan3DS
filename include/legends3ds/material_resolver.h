#pragma once
#include "ps1_texture.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace l3 {

enum class IndexedResourceKind : uint8_t { TextureType1 = 1, ClutType4 = 4, PaletteType9 = 9 };

struct IndexedResource {
    IndexedResourceKind kind = IndexedResourceKind::TextureType1;
    size_t header_offset = 0;
    std::string path;
    Type1Texture texture;
    Type4ClutPatch clut4;
    Type9PalettePatch palette9;
};

struct MaterialResolution {
    int image_resource = -1;
    int palette_resource = -1;
    int palette_row = -1;
    uint8_t bpp = 0;
    bool complete() const { return image_resource >= 0 && palette_resource >= 0 && palette_row >= 0; }
};

// Scans only sector-aligned compiled indexed-image/palette resources and rejects malformed lookalikes.
std::vector<IndexedResource> scan_mml_indexed_resources(const std::vector<uint8_t>& container);
MaterialResolution resolve_material_word(uint32_t material_word, const std::vector<IndexedResource>& resources);

} // namespace l3

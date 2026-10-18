#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3TextureVersion = 1;
constexpr uint16_t kL3PaletteVersion = 1;

struct Ps1TexturePage {
    uint16_t raw = 0;
    uint16_t vram_x_words = 0;
    uint16_t vram_y = 0;
    uint8_t bpp = 0;
    uint8_t abr = 0;
};

struct Ps1ClutRef {
    uint16_t raw = 0;
    uint16_t vram_x = 0;
    uint16_t vram_y = 0;
};

struct Type1Texture {
    uint32_t source_size = 0;
    uint32_t source_arg2 = 0;
    uint16_t clut_x = 0;
    uint16_t clut_y = 0;
    uint16_t palette_entries = 0;
    uint16_t palette_rows = 0;
    uint16_t image_x_words = 0;
    uint16_t image_y = 0;
    uint16_t width_words = 0;
    uint16_t width_pixels = 0;
    uint16_t height = 0;
    uint8_t bpp = 0;
    // Low-X MML resources store consecutive palette banks in RAM. The stored
    // palette_rows count is not the height of their later physical VRAM write.
    bool palette_row_banked = false;
    std::vector<uint16_t> palette_ps1;
    // Canonical row-major VRAM words, two little-endian bytes per word.
    // parse_mml_type1_texture converts the owner's sector-tiled image payload
    // into this layout; neutral/authored textures already use this layout.
    std::vector<uint8_t> packed_indices;
};

// MML DAT type 9 retains a CLUT payload/placement but does not carry image geometry in the compiled header.
// This is intentionally modeled separately from Type1Texture so image-page and palette ownership can be resolved independently.
struct Type4ClutPatch {
    uint32_t source_size = 0;
    uint32_t source_arg2 = 0;
    uint16_t clut_x = 0;
    uint16_t clut_y = 0;
    uint16_t palette_entries = 0;
    uint16_t palette_rows = 0;
    uint8_t bpp = 0;
    bool palette_row_banked = false;
    std::vector<uint16_t> palette_ps1;
};

struct Type9PalettePatch {
    uint32_t source_size = 0;
    uint32_t source_arg2 = 0;
    uint16_t clut_x = 0;
    uint16_t clut_y = 0;
    uint16_t palette_entries = 0;
    uint16_t palette_rows = 0;
    uint8_t bpp = 0;
    bool palette_row_banked = false;
    std::vector<uint16_t> palette_ps1;
};

struct L3TextureHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint16_t width_pixels = 0;
    uint16_t height = 0;
    uint16_t width_words = 0;
    uint8_t bpp = 0;
    uint8_t reserved0 = 0;
    uint16_t palette_entries = 0;
    uint16_t palette_rows = 0;
    uint16_t image_x_words = 0;
    uint16_t image_y = 0;
    uint16_t clut_x = 0;
    uint16_t clut_y = 0;
    uint32_t off_palette = 0;
    uint32_t palette_bytes = 0;
    uint32_t off_indices = 0;
    uint32_t index_bytes = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

struct L3PaletteHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t source_size = 0;
    uint32_t source_arg2 = 0;
    uint16_t clut_x = 0;
    uint16_t clut_y = 0;
    uint16_t palette_entries = 0;
    uint16_t palette_rows = 0;
    uint8_t bpp = 0;
    uint8_t reserved0 = 0;
    uint16_t reserved1 = 0;
    uint32_t off_palette = 0;
    uint32_t palette_bytes = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

Ps1TexturePage decode_ps1_tpage(uint16_t raw);
Ps1ClutRef decode_ps1_clut(uint16_t raw);

// Parses one sector-aligned MML DAT resource whose 0x800-byte resource header begins at resource_header_offset.
Type1Texture parse_mml_type1_texture(const std::vector<uint8_t>& container, size_t resource_header_offset);
Type4ClutPatch parse_mml_type4_clut(const std::vector<uint8_t>& container, size_t resource_header_offset);
Type9PalettePatch parse_mml_type9_palette(const std::vector<uint8_t>& container, size_t resource_header_offset);

// Image and CLUT ownership are deliberately separate: stage materials frequently use a type-1 image page
// together with a palette row supplied by another type-1 resource or a type-9 palette patch.
bool texture_owns_image_page(const Type1Texture& texture, uint16_t tpage);
bool texture_resolves_material(const Type1Texture& texture, uint16_t tpage, uint16_t clut);
int texture_palette_row(const Type1Texture& texture, uint16_t clut);
int palette_patch_row(const Type4ClutPatch& patch, uint16_t clut);
int palette_patch_row(const Type9PalettePatch& patch, uint16_t clut);
bool palette_patch_resolves_clut(const Type4ClutPatch& patch, uint16_t clut, uint8_t material_bpp);
bool palette_patch_resolves_clut(const Type9PalettePatch& patch, uint16_t clut, uint8_t material_bpp);

std::vector<uint8_t> cook_l3texture(const Type1Texture& texture);
L3TextureHeader inspect_l3texture(const std::vector<uint8_t>& bytes, bool verify_crc = true);
std::vector<uint8_t> cook_l3palette(const Type4ClutPatch& patch);
std::vector<uint8_t> cook_l3palette(const Type9PalettePatch& patch);
L3PaletteHeader inspect_l3palette(const std::vector<uint8_t>& bytes, bool verify_crc = true);

// CPU helper for tests/debug tools. Returns palette indices, one byte per pixel.
std::vector<uint8_t> unpack_texture_indices(const Type1Texture& texture);

} // namespace l3

#include "legends3ds/material_resolver.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <utility>
#include <stdexcept>

namespace l3 {
namespace {
std::string resource_path(const std::vector<uint8_t>& b, size_t off) {
    std::string out;
    const size_t begin = off + 0x40u;
    const size_t end = std::min(b.size(), off + 0x100u);
    for (size_t i = begin; i < end && b[i] != 0; ++i) {
        const uint8_t c = b[i];
        if (c < 0x20u || c >= 0x7Fu) return {};
        out.push_back(static_cast<char>(c));
    }
    return out;
}
}

std::vector<IndexedResource> scan_mml_indexed_resources(const std::vector<uint8_t>& b) {
    std::vector<IndexedResource> out;
    for (size_t off = 0; off + 0x800u <= b.size(); off += 0x800u) {
        const uint32_t type = rd_u32(b.data() + off);
        if (!(type == 1u || type == 4u || type == 9u)) continue;
        try {
            IndexedResource r;
            r.header_offset = off;
            r.path = resource_path(b, off);
            if (type == 1u) {
                r.kind = IndexedResourceKind::TextureType1;
                r.texture = parse_mml_type1_texture(b, off);
            } else if (type == 4u) {
                r.kind = IndexedResourceKind::ClutType4;
                r.clut4 = parse_mml_type4_clut(b, off);
            } else {
                r.kind = IndexedResourceKind::PaletteType9;
                r.palette9 = parse_mml_type9_palette(b, off);
            }
            out.push_back(std::move(r));
        } catch (const std::exception&) {
            // Some data sectors can begin with small integers that resemble resource types. A malformed
            // sector is not an indexed resource; callers get only parser-validated entries.
        }
    }
    return out;
}

MaterialResolution resolve_material_word(uint32_t word, const std::vector<IndexedResource>& resources) {
    const uint16_t tpage = static_cast<uint16_t>(word & 0xFFFFu);
    const uint16_t clut = static_cast<uint16_t>(word >> 16);
    const auto page = decode_ps1_tpage(tpage);
    MaterialResolution out;
    out.bpp = page.bpp;
    for (size_t i = 0; i < resources.size(); ++i) {
        const auto& r = resources[i];
        if (r.kind == IndexedResourceKind::TextureType1 && texture_owns_image_page(r.texture, tpage)) {
            out.image_resource = static_cast<int>(i);
            break;
        }
    }
    for (size_t i = 0; i < resources.size(); ++i) {
        const auto& r = resources[i];
        int row = -1;
        if (r.kind == IndexedResourceKind::TextureType1 && r.texture.bpp == page.bpp) row = texture_palette_row(r.texture, clut);
        else if (r.kind == IndexedResourceKind::ClutType4) {
            if (r.clut4.bpp == page.bpp) row = palette_patch_row(r.clut4, clut);
        } else if (r.kind == IndexedResourceKind::PaletteType9) {
            if (r.palette9.bpp == page.bpp) row = palette_patch_row(r.palette9, clut);
        }
        if (row >= 0) {
            out.palette_resource = static_cast<int>(i);
            out.palette_row = row;
            break;
        }
    }
    return out;
}

} // namespace l3

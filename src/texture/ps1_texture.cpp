#include "legends3ds/ps1_texture.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace l3 {
namespace {
constexpr size_t kSector = 0x800;
constexpr size_t kPaletteOffset = 0x100;

uint16_t narrow_u16(uint32_t v, const char* what) {
    if (v > std::numeric_limits<uint16_t>::max()) throw std::runtime_error(std::string(what) + " exceeds u16");
    return static_cast<uint16_t>(v);
}

template <typename Palette> int stored_palette_row(const Palette& palette, uint16_t clut) {
    const auto c = decode_ps1_clut(clut);
    if (!palette.palette_row_banked) {
        if (c.vram_x != palette.clut_x || c.vram_y < palette.clut_y ||
            c.vram_y >= uint32_t(palette.clut_y) + palette.palette_rows) return -1;
        return int(c.vram_y - palette.clut_y);
    }
    if (uint32_t(c.vram_x) + palette.palette_entries > 256u || !palette.palette_entries) return -1;
    const uint32_t y = palette.clut_y;
    const int64_t bank_row = int64_t(y) - (y & 0x100u) + ((y & 0x100u) >> 4) - 240;
    if (bank_row < 0 || bank_row >= 32) return -1;
    const uint32_t first = uint32_t(bank_row) * 256u + palette.clut_x;
    const uint32_t words = uint32_t(palette.palette_entries) * palette.palette_rows;
    if (first > 8192u || words > 8192u - first) return -1;
    uint32_t requested_row;
    if (c.vram_y >= 240 && c.vram_y < 256) requested_row = c.vram_y - 240;
    else if (c.vram_y >= 496 && c.vram_y < 512) requested_row = c.vram_y - 480;
    else return -1;
    if (requested_row != uint32_t(bank_row)) return -1; // Only the starting row is marked dirty.
    const uint32_t requested = requested_row * 256u + c.vram_x;
    if (requested < first || requested - first >= words ||
        (requested - first) % palette.palette_entries) return -1;
    return int((requested - first) / palette.palette_entries);
}
}

Ps1TexturePage decode_ps1_tpage(uint16_t raw) {
    Ps1TexturePage out;
    out.raw = raw;
    out.vram_x_words = static_cast<uint16_t>((raw & 0x0Fu) * 64u);
    out.vram_y = static_cast<uint16_t>((raw & 0x10u) ? 256u : 0u);
    out.abr = static_cast<uint8_t>((raw >> 5) & 0x03u);
    const uint8_t depth = static_cast<uint8_t>((raw >> 7) & 0x03u);
    if (depth == 0) out.bpp = 4;
    else if (depth == 1) out.bpp = 8;
    else if (depth == 2) out.bpp = 16;
    else out.bpp = 0;
    return out;
}

Ps1ClutRef decode_ps1_clut(uint16_t raw) {
    Ps1ClutRef out;
    out.raw = raw;
    out.vram_x = static_cast<uint16_t>((raw & 0x3Fu) * 16u);
    out.vram_y = static_cast<uint16_t>((raw >> 6) & 0x01FFu);
    return out;
}

Type1Texture parse_mml_type1_texture(const std::vector<uint8_t>& c, size_t h) {
    if (h > c.size() || c.size() - h < kSector) throw std::runtime_error("texture resource header out of range");
    const uint8_t* p = c.data() + h;
    const uint32_t type = rd_u32(p + 0x00);
    if (type != 1) throw std::runtime_error("resource is not a type-1 compiled texture");

    const uint32_t source_size = rd_u32(p + 0x04);
    const uint32_t arg2 = rd_u32(p + 0x08);
    const uint32_t cx = rd_u32(p + 0x0C);
    const uint32_t cy = rd_u32(p + 0x10);
    const uint32_t pe = rd_u32(p + 0x14);
    const uint32_t pr = rd_u32(p + 0x18);
    const uint32_t ix = rd_u32(p + 0x1C);
    const uint32_t iy = rd_u32(p + 0x20);
    const uint32_t ww = rd_u32(p + 0x24);
    const uint32_t hh = rd_u32(p + 0x28);

    if (!(pe == 16 || pe == 256)) throw std::runtime_error("unsupported indexed texture palette width");
    if (pr == 0 || pr > 64) throw std::runtime_error("invalid texture palette row count");
    if (ww == 0 || hh == 0) throw std::runtime_error("zero-sized texture");
    // The original type-1 loader uploads each 2048-byte sector as a 32-word
    // by 32-row RECT (8001c350), advancing X tiles before Y tiles. A partial
    // tile has no supported source interpretation; do not guess its padding.
    if ((ww % 32u) != 0u || (hh % 32u) != 0u)
        throw std::runtime_error("type-1 image must contain whole 32-word by 32-row sector tiles");
    if (ix > 1024u || ww > 1024u - ix || iy > 512u || hh > 512u - iy)
        throw std::runtime_error("type-1 image exceeds physical VRAM bounds");
    const uint8_t bpp = pe == 16 ? 4 : 8;
    const uint64_t width_pixels64 = uint64_t(ww) * (bpp == 4 ? 4u : 2u);
    const uint64_t palette_bytes64 = uint64_t(pe) * pr * 2u;
    const uint64_t index_bytes64 = uint64_t(ww) * 2u * hh;
    if (width_pixels64 > 65535u || palette_bytes64 > 0x700u) throw std::runtime_error("texture dimensions/palette exceed supported resource layout");
    if (index_bytes64 > std::numeric_limits<size_t>::max()) throw std::runtime_error("texture index data too large");
    const size_t palette_bytes = static_cast<size_t>(palette_bytes64);
    const size_t index_bytes = static_cast<size_t>(index_bytes64);
    if (h + kPaletteOffset + palette_bytes > h + kSector) throw std::runtime_error("palette overlaps resource payload sector");
    if (h + kSector > c.size() || index_bytes > c.size() - (h + kSector)) throw std::runtime_error("texture pixel payload out of range");

    // Community tools and our disc observations agree source_size is the logical 0x20-byte texture header
    // + CLUT bytes + packed pixel bytes, while the on-disc compiled resource keeps CLUT at +0x100 and pixels at +0x800.
    const uint64_t expected_source_size = 0x20u + palette_bytes64 + index_bytes64;
    if (source_size != expected_source_size) throw std::runtime_error("type-1 texture logical size check failed");

    Type1Texture out;
    out.source_size = source_size;
    out.source_arg2 = arg2;
    out.clut_x = narrow_u16(cx, "clut_x");
    out.clut_y = narrow_u16(cy, "clut_y");
    out.palette_entries = narrow_u16(pe, "palette_entries");
    out.palette_rows = narrow_u16(pr, "palette_rows");
    out.image_x_words = narrow_u16(ix, "image_x_words");
    out.image_y = narrow_u16(iy, "image_y");
    out.width_words = narrow_u16(ww, "width_words");
    out.width_pixels = static_cast<uint16_t>(width_pixels64);
    out.height = narrow_u16(hh, "height");
    out.bpp = bpp;
    // USA loader0x8001be18 routes low-X CLUTs into the contiguous CPU bank.
    out.palette_row_banked = cx < 320u;
    out.palette_ps1.reserve(size_t(pe) * pr);
    for (size_t i = 0; i < size_t(pe) * pr; ++i) out.palette_ps1.push_back(rd_u16(c.data() + h + kPaletteOffset + i * 2));
    // Canonical row-major VRAM words are shared by the neutral texture cooker
    // and stateful material composer. The raw file is NOT one row-major image:
    // consecutive sectors contain complete 32x32 word tiles. No Y flip or
    // palette/color reinterpretation belongs to this source-layout conversion.
    out.packed_indices.resize(index_bytes);
    const size_t row_bytes = size_t(ww) * 2u;
    const size_t tiles_x = size_t(ww) / 32u;
    for (size_t tile_y = 0; tile_y < size_t(hh) / 32u; ++tile_y) {
        for (size_t tile_x = 0; tile_x < tiles_x; ++tile_x) {
            const size_t sector = h + kSector + (tile_y * tiles_x + tile_x) * kSector;
            for (size_t row = 0; row < 32u; ++row) {
                const size_t destination = (tile_y * 32u + row) * row_bytes + tile_x * 64u;
                std::copy_n(c.data() + sector + row * 64u, 64u,
                            out.packed_indices.data() + destination);
            }
        }
    }
    return out;
}

Type4ClutPatch parse_mml_type4_clut(const std::vector<uint8_t>& c, size_t h) {
    if (h > c.size() || c.size() - h < kSector) throw std::runtime_error("CLT resource header out of range");
    const uint8_t* p = c.data() + h;
    if (rd_u32(p + 0x00) != 4) throw std::runtime_error("resource is not a type-4 compiled CLT patch");
    const uint32_t source_size = rd_u32(p + 0x04);
    const uint32_t arg2 = rd_u32(p + 0x08);
    const uint32_t cx = rd_u32(p + 0x0C), cy = rd_u32(p + 0x10);
    const uint32_t pe = rd_u32(p + 0x14), pr = rd_u32(p + 0x18);
    if (!(pe == 16 || pe == 256)) throw std::runtime_error("unsupported type-4 CLT palette width");
    if (pr == 0 || pr > 64) throw std::runtime_error("invalid type-4 CLT palette row count");
    const uint64_t palette_bytes64 = uint64_t(pe) * pr * 2u;
    if (palette_bytes64 > 0x700u || h + kPaletteOffset + palette_bytes64 > h + kSector) throw std::runtime_error("type-4 CLT palette exceeds header sector");
    Type4ClutPatch out;
    out.source_size = source_size; out.source_arg2 = arg2;
    out.clut_x = narrow_u16(cx, "clut_x"); out.clut_y = narrow_u16(cy, "clut_y");
    out.palette_entries = narrow_u16(pe, "palette_entries"); out.palette_rows = narrow_u16(pr, "palette_rows");
    out.bpp = pe == 16 ? 4 : 8;
    // Shared type4/type9 branch0x8001bf4c uses the narrower low-X cutoff.
    out.palette_row_banked = cx < 256u;
    out.palette_ps1.reserve(size_t(pe) * pr);
    for (size_t i = 0; i < size_t(pe) * pr; ++i) out.palette_ps1.push_back(rd_u16(c.data() + h + kPaletteOffset + i * 2));
    return out;
}

Type9PalettePatch parse_mml_type9_palette(const std::vector<uint8_t>& c, size_t h) {
    if (h > c.size() || c.size() - h < kSector) throw std::runtime_error("palette resource header out of range");
    const uint8_t* p = c.data() + h;
    if (rd_u32(p + 0x00) != 9) throw std::runtime_error("resource is not a type-9 compiled palette patch");
    const uint32_t source_size = rd_u32(p + 0x04);
    const uint32_t arg2 = rd_u32(p + 0x08);
    const uint32_t cx = rd_u32(p + 0x0C), cy = rd_u32(p + 0x10);
    const uint32_t pe = rd_u32(p + 0x14), pr = rd_u32(p + 0x18);
    if (!(pe == 16 || pe == 256)) throw std::runtime_error("unsupported type-9 palette width");
    if (pr == 0 || pr > 64) throw std::runtime_error("invalid type-9 palette row count");
    const uint64_t palette_bytes64 = uint64_t(pe) * pr * 2u;
    if (palette_bytes64 > 0x700u || h + kPaletteOffset + palette_bytes64 > h + kSector) throw std::runtime_error("type-9 palette exceeds header sector");
    Type9PalettePatch out;
    out.source_size = source_size; out.source_arg2 = arg2;
    out.clut_x = narrow_u16(cx, "clut_x"); out.clut_y = narrow_u16(cy, "clut_y");
    out.palette_entries = narrow_u16(pe, "palette_entries"); out.palette_rows = narrow_u16(pr, "palette_rows");
    out.bpp = pe == 16 ? 4 : 8;
    out.palette_row_banked = cx < 256u;
    out.palette_ps1.reserve(size_t(pe) * pr);
    for (size_t i = 0; i < size_t(pe) * pr; ++i) out.palette_ps1.push_back(rd_u16(c.data() + h + kPaletteOffset + i * 2));
    return out;
}

int texture_palette_row(const Type1Texture& t, uint16_t clut) {
    return stored_palette_row(t, clut);
}

bool texture_owns_image_page(const Type1Texture& t, uint16_t tpage) {
    const auto tp = decode_ps1_tpage(tpage);
    return tp.bpp == t.bpp && tp.vram_x_words == t.image_x_words && tp.vram_y == t.image_y;
}

bool texture_resolves_material(const Type1Texture& t, uint16_t tpage, uint16_t clut) {
    return texture_owns_image_page(t, tpage) && texture_palette_row(t, clut) >= 0;
}

int palette_patch_row(const Type4ClutPatch& p, uint16_t clut) {
    return stored_palette_row(p, clut);
}

bool palette_patch_resolves_clut(const Type4ClutPatch& p, uint16_t clut, uint8_t material_bpp) {
    return p.bpp == material_bpp && palette_patch_row(p, clut) >= 0;
}

int palette_patch_row(const Type9PalettePatch& p, uint16_t clut) {
    return stored_palette_row(p, clut);
}

bool palette_patch_resolves_clut(const Type9PalettePatch& p, uint16_t clut, uint8_t material_bpp) {
    return p.bpp == material_bpp && palette_patch_row(p, clut) >= 0;
}

std::vector<uint8_t> unpack_texture_indices(const Type1Texture& t) {
    const size_t pixels = size_t(t.width_pixels) * t.height;
    std::vector<uint8_t> out;
    out.reserve(pixels);
    if (t.bpp == 8) {
        if (t.packed_indices.size() != pixels) throw std::runtime_error("8bpp packed byte count mismatch");
        return t.packed_indices;
    }
    if (t.bpp != 4 || t.packed_indices.size() * 2 != pixels) throw std::runtime_error("4bpp packed byte count mismatch");
    for (uint8_t b : t.packed_indices) { out.push_back(static_cast<uint8_t>(b & 0x0F)); out.push_back(static_cast<uint8_t>(b >> 4)); }
    return out;
}

std::vector<uint8_t> cook_l3texture(const Type1Texture& t) {
    if (!(t.bpp == 4 || t.bpp == 8)) throw std::runtime_error("L3TX only supports 4/8bpp indexed source textures");
    const uint32_t pal_bytes = uint32_t(t.palette_ps1.size() * 2u);
    const uint32_t idx_bytes = uint32_t(t.packed_indices.size());
    std::vector<uint8_t> o;
    o.reserve(64u + pal_bytes + idx_bytes);
    o.insert(o.end(), {'L','3','T','X'});
    wr_u16(o, kL3TextureVersion); wr_u16(o, 64);
    wr_u32(o, 1u); // bit0: raw PS1 A1B5G5R5 CLUT + packed palette indices retained
    wr_u16(o, t.width_pixels); wr_u16(o, t.height); wr_u16(o, t.width_words);
    o.push_back(t.bpp); o.push_back(0);
    wr_u16(o, t.palette_entries); wr_u16(o, t.palette_rows);
    wr_u16(o, t.image_x_words); wr_u16(o, t.image_y); wr_u16(o, t.clut_x); wr_u16(o, t.clut_y);
    const size_t p_off_pal = o.size(); wr_u32(o, 0); wr_u32(o, pal_bytes);
    const size_t p_off_idx = o.size(); wr_u32(o, 0); wr_u32(o, idx_bytes);
    const size_t p_file = o.size(); wr_u32(o, 0);
    const size_t p_crc = o.size(); wr_u32(o, 0);
    wr_u32(o, 0); wr_u32(o, 0);
    if (o.size() != 64) throw std::runtime_error("L3TX header size bug");
    const uint32_t off_pal = uint32_t(o.size());
    for (uint16_t v : t.palette_ps1) wr_u16(o, v);
    const uint32_t off_idx = uint32_t(o.size());
    o.insert(o.end(), t.packed_indices.begin(), t.packed_indices.end());
    patch_u32(o, p_off_pal, off_pal); patch_u32(o, p_off_idx, off_idx); patch_u32(o, p_file, uint32_t(o.size()));
    patch_u32(o, p_crc, crc32(o.data() + 64, o.size() - 64));
    return o;
}

L3TextureHeader inspect_l3texture(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < 64 || b[0] != 'L' || b[1] != '3' || b[2] != 'T' || b[3] != 'X') throw std::runtime_error("not L3TX");
    L3TextureHeader h;
    h.version = rd_u16(&b[4]); h.header_bytes = rd_u16(&b[6]); h.flags = rd_u32(&b[8]);
    h.width_pixels = rd_u16(&b[12]); h.height = rd_u16(&b[14]); h.width_words = rd_u16(&b[16]); h.bpp = b[18]; h.reserved0 = b[19];
    h.palette_entries = rd_u16(&b[20]); h.palette_rows = rd_u16(&b[22]); h.image_x_words = rd_u16(&b[24]); h.image_y = rd_u16(&b[26]); h.clut_x = rd_u16(&b[28]); h.clut_y = rd_u16(&b[30]);
    h.off_palette = rd_u32(&b[32]); h.palette_bytes = rd_u32(&b[36]); h.off_indices = rd_u32(&b[40]); h.index_bytes = rd_u32(&b[44]); h.file_bytes = rd_u32(&b[48]); h.payload_crc32 = rd_u32(&b[52]);
    if (h.version != kL3TextureVersion || h.header_bytes != 64) throw std::runtime_error("unsupported L3TX");
    if (h.file_bytes != b.size()) throw std::runtime_error("L3TX file size mismatch");
    if (!(h.off_palette >= 64 && uint64_t(h.off_palette) + h.palette_bytes <= b.size())) throw std::runtime_error("L3TX palette range invalid");
    if (!(h.off_indices >= h.off_palette + h.palette_bytes && uint64_t(h.off_indices) + h.index_bytes <= b.size())) throw std::runtime_error("L3TX index range invalid");
    if (verify_crc && crc32(b.data() + 64, b.size() - 64) != h.payload_crc32) throw std::runtime_error("L3TX CRC mismatch");
    return h;
}

std::vector<uint8_t> cook_l3palette(const Type4ClutPatch& p) {
    if (!(p.bpp == 4 || p.bpp == 8)) throw std::runtime_error("L3PL only supports 4/8bpp indexed palettes");
    const uint32_t pal_bytes = uint32_t(p.palette_ps1.size() * 2u);
    std::vector<uint8_t> o; o.reserve(48u + pal_bytes);
    o.insert(o.end(), {'L','3','P','L'}); wr_u16(o, kL3PaletteVersion); wr_u16(o, 48); wr_u32(o, 3u); // bit1: source was type-4 CLT
    wr_u32(o, p.source_size); wr_u32(o, p.source_arg2);
    wr_u16(o, p.clut_x); wr_u16(o, p.clut_y); wr_u16(o, p.palette_entries); wr_u16(o, p.palette_rows);
    o.push_back(p.bpp); o.push_back(0); wr_u16(o, 0);
    const size_t po = o.size(); wr_u32(o, 0); wr_u32(o, pal_bytes);
    const size_t pf = o.size(); wr_u32(o, 0); const size_t pc = o.size(); wr_u32(o, 0);
    if (o.size() != 48) throw std::runtime_error("L3PL header size bug");
    const uint32_t off = uint32_t(o.size()); for (uint16_t v : p.palette_ps1) wr_u16(o, v);
    patch_u32(o, po, off); patch_u32(o, pf, uint32_t(o.size())); patch_u32(o, pc, crc32(o.data() + 48, o.size() - 48));
    return o;
}

std::vector<uint8_t> cook_l3palette(const Type9PalettePatch& p) {
    if (!(p.bpp == 4 || p.bpp == 8)) throw std::runtime_error("L3PL only supports 4/8bpp indexed palettes");
    const uint32_t pal_bytes = uint32_t(p.palette_ps1.size() * 2u);
    std::vector<uint8_t> o; o.reserve(48u + pal_bytes);
    o.insert(o.end(), {'L','3','P','L'}); wr_u16(o, kL3PaletteVersion); wr_u16(o, 48); wr_u32(o, 1u);
    wr_u32(o, p.source_size); wr_u32(o, p.source_arg2);
    wr_u16(o, p.clut_x); wr_u16(o, p.clut_y); wr_u16(o, p.palette_entries); wr_u16(o, p.palette_rows);
    o.push_back(p.bpp); o.push_back(0); wr_u16(o, 0);
    const size_t po = o.size(); wr_u32(o, 0); wr_u32(o, pal_bytes);
    const size_t pf = o.size(); wr_u32(o, 0); const size_t pc = o.size(); wr_u32(o, 0);
    if (o.size() != 48) throw std::runtime_error("L3PL header size bug");
    const uint32_t off = uint32_t(o.size()); for (uint16_t v : p.palette_ps1) wr_u16(o, v);
    patch_u32(o, po, off); patch_u32(o, pf, uint32_t(o.size())); patch_u32(o, pc, crc32(o.data() + 48, o.size() - 48));
    return o;
}

L3PaletteHeader inspect_l3palette(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < 48 || b[0] != 'L' || b[1] != '3' || b[2] != 'P' || b[3] != 'L') throw std::runtime_error("not L3PL");
    L3PaletteHeader h; h.version=rd_u16(&b[4]); h.header_bytes=rd_u16(&b[6]); h.flags=rd_u32(&b[8]);
    h.source_size=rd_u32(&b[12]); h.source_arg2=rd_u32(&b[16]); h.clut_x=rd_u16(&b[20]); h.clut_y=rd_u16(&b[22]);
    h.palette_entries=rd_u16(&b[24]); h.palette_rows=rd_u16(&b[26]); h.bpp=b[28]; h.reserved0=b[29]; h.reserved1=rd_u16(&b[30]);
    h.off_palette=rd_u32(&b[32]); h.palette_bytes=rd_u32(&b[36]); h.file_bytes=rd_u32(&b[40]); h.payload_crc32=rd_u32(&b[44]);
    if (h.version != kL3PaletteVersion || h.header_bytes != 48) throw std::runtime_error("unsupported L3PL");
    if (h.file_bytes != b.size()) throw std::runtime_error("L3PL file size mismatch");
    if (h.off_palette < 48 || uint64_t(h.off_palette) + h.palette_bytes > b.size()) throw std::runtime_error("L3PL palette range invalid");
    if (verify_crc && crc32(b.data()+48,b.size()-48) != h.payload_crc32) throw std::runtime_error("L3PL CRC mismatch");
    return h;
}

} // namespace l3

#include "legends3ds/material_cache.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace l3 {
namespace {
constexpr uint32_t kHeaderBytes = 48;
constexpr uint32_t kFlagResolved = 1u;
constexpr uint32_t kCacheFlagRawPs1Palette = 1u;
constexpr uint32_t kCacheFlagPadded256Page = 2u;

uint8_t resource_bpp(const IndexedResource& r) {
    if (r.kind == IndexedResourceKind::TextureType1) return r.texture.bpp;
    if (r.kind == IndexedResourceKind::ClutType4) return r.clut4.bpp;
    return r.palette9.bpp;
}

int resource_palette_row(const IndexedResource& r, uint16_t clut) {
    if (r.kind == IndexedResourceKind::TextureType1) return texture_palette_row(r.texture, clut);
    if (r.kind == IndexedResourceKind::ClutType4) return palette_patch_row(r.clut4, clut);
    return palette_patch_row(r.palette9, clut);
}

uint8_t expand5(uint16_t v) {
    const uint8_t x = static_cast<uint8_t>(v & 31u);
    return static_cast<uint8_t>((x << 3) | (x >> 2));
}

// Retain only the requested page and CLUT, rather than an entire 1 MiB PS1
// framebuffer. Upload words carry no intrinsic indexed depth: the destination
// TPAGE chooses how the final VRAM bits are decoded into 4/8-bit pixel indices.
struct VramRegion {
    uint16_t x = 0, y = 0, width = 0, height = 0;
    std::vector<uint16_t> words;
    std::vector<uint8_t> written;
    uint16_t extent_x = 0, extent_y = 0;
    VramRegion(uint16_t px, uint16_t py, uint16_t w, uint16_t h)
        : x(px), y(py), width(w), height(h), words(size_t(w) * h), written(size_t(w) * h) {
        if (uint32_t(x) + width > 1024u || uint32_t(y) + height > 512u)
            throw std::runtime_error("requested material VRAM region out of bounds");
    }
    template <typename WordAt> void upload(uint16_t ux, uint16_t uy, uint16_t uw,
                                          uint16_t uh, WordAt word_at) {
        if (uint32_t(ux) + uw > 1024u || uint32_t(uy) + uh > 512u)
            throw std::runtime_error("material upload exceeds physical VRAM bounds");
        const uint32_t left = std::max<uint32_t>(x, ux), top = std::max<uint32_t>(y, uy);
        const uint32_t right = std::min<uint32_t>(uint32_t(x) + width, uint32_t(ux) + uw);
        const uint32_t bottom = std::min<uint32_t>(uint32_t(y) + height, uint32_t(uy) + uh);
        if (left >= right || top >= bottom) return;
        for (uint32_t row = top; row < bottom; ++row) for (uint32_t column = left; column < right; ++column) {
            const size_t destination = size_t(row - y) * width + column - x;
            words[destination] = word_at(size_t(row - uy) * uw + column - ux);
            written[destination] = 1;
        }
        extent_x = static_cast<uint16_t>(std::max<uint32_t>(extent_x, right - x));
        extent_y = static_cast<uint16_t>(std::max<uint32_t>(extent_y, bottom - y));
    }
    void unknown_row(uint16_t uy) {
        if (uy < y || uint32_t(uy) >= uint32_t(y) + height) return;
        const uint32_t right = std::min<uint32_t>(uint32_t(x) + width, 256u);
        for (uint32_t column = x; column < right; ++column) {
            const size_t destination = size_t(uy - y) * width + column - x;
            words[destination] = 0;
            written[destination] = 0;
        }
    }
    size_t coverage() const { return static_cast<size_t>(std::count(written.begin(), written.end(), uint8_t(1))); }
};

struct CpuPaletteBank {
    std::vector<uint16_t> words = std::vector<uint16_t>(8192);
    std::vector<uint8_t> written = std::vector<uint8_t>(8192);
    uint32_t dirty = 0;
};

void flush_palette_bank(const CpuPaletteBank& bank, VramRegion& image, VramRegion& clut) {
    for (uint16_t row = 0; row < 32u; ++row) {
        if (!(bank.dirty & (uint32_t(1) << row))) continue;
        const uint16_t physical_y = static_cast<uint16_t>(row < 16u ? 240u + row : 480u + row);
        image.unknown_row(physical_y);
        clut.unknown_row(physical_y);
        const size_t row_start = size_t(row) * 256u;
        for (uint16_t x = 0; x < 256u;) {
            if (!bank.written[row_start + x]) { ++x; continue; }
            const uint16_t begin = x;
            while (x < 256u && bank.written[row_start + x]) ++x;
            const uint16_t width = static_cast<uint16_t>(x - begin);
            const auto word_at = [&](size_t index) { return bank.words[row_start + begin + index]; };
            image.upload(begin, physical_y, width, 1, word_at);
            clut.upload(begin, physical_y, width, 1, word_at);
        }
    }
}

template <typename Palette> void upload_palette(const Palette& palette, VramRegion& image,
                                                VramRegion& clut, CpuPaletteBank& bank) {
    if (palette.palette_ps1.empty() && !palette.palette_entries && !palette.palette_rows) return;
    if (!(palette.palette_entries == 16 || palette.palette_entries == 256) ||
        !palette.palette_rows || palette.palette_ps1.size() != size_t(palette.palette_entries) * palette.palette_rows)
        throw std::runtime_error("material palette upload shape mismatch");
    if (palette.palette_row_banked) {
        // Original low-X loader copies stored colors contiguously into 32 RAM
        // rows of 256 words (0x8001be10/0x8001bf44). Dirty rows are staged at
        // 0x8001786c, then uploaded by0x800178fc as (0,240,256,16) and
        // (0,496,256,16). Split across bank rows before mapping physical Y;
        // the discontinuity between row15 and16 must not become VRAM wrapping.
        const uint32_t y = palette.clut_y;
        const int64_t bank_row = int64_t(y) - (y & 0x100u) + ((y & 0x100u) >> 4) - 240;
        if (bank_row < 0 || bank_row >= 32)
            throw std::runtime_error("material palette CPU bank row out of bounds");
        const uint32_t first = uint32_t(bank_row) * 256u + palette.clut_x;
        const size_t total = palette.palette_ps1.size();
        if (first > 8192u || total > 8192u - first)
            throw std::runtime_error("material palette CPU bank span out of bounds");
        for (size_t offset = 0; offset < total; ++offset) {
            bank.words[first + offset] = palette.palette_ps1[offset];
            bank.written[first + offset] = 1;
        }
        // The loader dirties only the header's starting bank row. A span may
        // cross into another RAM row, but that tail is not GPU-visible until a
        // later resource dirties its row. Retain CPU bytes and dirty flags until
        // the initial ordered load batch has finished, then stage bank rows.
        bank.dirty |= uint32_t(1) << static_cast<uint32_t>(bank_row);
        return;
    }
    const auto word_at = [&](size_t index) { return palette.palette_ps1[index]; };
    image.upload(palette.clut_x, palette.clut_y, palette.palette_entries, palette.palette_rows, word_at);
    clut.upload(palette.clut_x, palette.clut_y, palette.palette_entries, palette.palette_rows, word_at);
}

void compose_uploads(const std::vector<IndexedResource>& resources, VramRegion& image, VramRegion& clut) {
    CpuPaletteBank bank;
    for (const auto& resource : resources) {
        if (resource.kind == IndexedResourceKind::ClutType4) {
            upload_palette(resource.clut4, image, clut, bank);
        } else if (resource.kind == IndexedResourceKind::PaletteType9) {
            upload_palette(resource.palette9, image, clut, bank);
        } else if (resource.kind == IndexedResourceKind::TextureType1) {
            const auto& texture = resource.texture;
            upload_palette(texture, image, clut, bank);
            if (!texture.width_words || !texture.height ||
                texture.packed_indices.size() != size_t(texture.width_words) * texture.height * 2u)
                throw std::runtime_error("material image upload shape mismatch");
            const auto word_at = [&](size_t index) { return rd_u16(texture.packed_indices.data() + index * 2u); };
            image.upload(texture.image_x_words, texture.image_y, texture.width_words, texture.height, word_at);
            clut.upload(texture.image_x_words, texture.image_y, texture.width_words, texture.height, word_at);
        } else {
            throw std::runtime_error("unknown material upload resource kind");
        }
    }
    // This offline cache represents the initial load batch followed by palette
    // staging, not frame-dependent fades, animation or subsequent dirty timing.
    flush_palette_bank(bank, image, clut);
}

void validate_entry(const MaterialCacheEntry& e) {
    const auto page = decode_ps1_tpage(static_cast<uint16_t>(e.material_word & 0xFFFFu));
    if (!(page.bpp == 4 || page.bpp == 8)) throw std::runtime_error("material cache has unsupported TPAGE depth");
    if (e.bpp != page.bpp) throw std::runtime_error("material cache BPP does not match TPAGE");
    if (!e.resolved) {
        if (!e.indices.empty() || !e.palette_ps1.empty()) throw std::runtime_error("unresolved material cache entry has payload");
        return;
    }
    const uint16_t expected_palette = e.bpp == 4 ? 16u : 256u;
    if (e.palette_entries != expected_palette || e.palette_ps1.size() != expected_palette)
        throw std::runtime_error("material cache palette size mismatch");
    if (e.indices.size() != size_t(kL3MaterialTextureWidth) * kL3MaterialTextureHeight)
        throw std::runtime_error("material cache page index size mismatch");
    if (e.source_width == 0 || e.source_height == 0 || e.source_width > kL3MaterialTextureWidth || e.source_height > kL3MaterialTextureHeight)
        throw std::runtime_error("material cache source dimensions invalid");
}
}

MaterialResolution resolve_material_word_last_writer(uint32_t word, const std::vector<IndexedResource>& resources) {
    const uint16_t tpage = static_cast<uint16_t>(word & 0xFFFFu);
    const uint16_t clut = static_cast<uint16_t>(word >> 16);
    const auto page = decode_ps1_tpage(tpage);
    MaterialResolution out;
    out.bpp = page.bpp;
    for (size_t i = resources.size(); i-- > 0;) {
        const auto& r = resources[i];
        if (r.kind == IndexedResourceKind::TextureType1 && texture_owns_image_page(r.texture, tpage)) {
            out.image_resource = static_cast<int>(i);
            break;
        }
    }
    for (size_t i = resources.size(); i-- > 0;) {
        const auto& r = resources[i];
        if (resource_bpp(r) != page.bpp) continue;
        const int row = resource_palette_row(r, clut);
        if (row >= 0) {
            out.palette_resource = static_cast<int>(i);
            out.palette_row = row;
            break;
        }
    }
    return out;
}

MaterialCache build_material_cache(const std::vector<uint32_t>& words, const std::vector<IndexedResource>& resources,
                                  std::vector<MaterialVramDiagnostics>* diagnostics) {
    std::vector<uint32_t> unique = words;
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    MaterialCache out;
    out.entries.reserve(unique.size());
    if (diagnostics) { diagnostics->clear(); diagnostics->reserve(unique.size()); }
    for (const uint32_t word : unique) {
        const auto page = decode_ps1_tpage(static_cast<uint16_t>(word & 0xFFFFu));
        if (!(page.bpp == 4 || page.bpp == 8)) throw std::runtime_error("material word uses unsupported TPAGE depth");
        MaterialCacheEntry e;
        e.material_word = word;
        e.bpp = page.bpp;
        e.abr = page.abr;
        const uint16_t pixels_per_word = page.bpp == 4 ? 4u : 2u;
        const uint16_t pe = page.bpp == 4 ? 16u : 256u;
        const auto clut_ref = decode_ps1_clut(static_cast<uint16_t>(word >> 16));
        VramRegion image(page.vram_x_words, page.vram_y,
                         static_cast<uint16_t>(kL3MaterialTextureWidth / pixels_per_word), kL3MaterialTextureHeight);
        VramRegion clut(clut_ref.vram_x, clut_ref.vram_y, pe, 1);
        compose_uploads(resources, image, clut);
        const size_t image_coverage = image.coverage(), clut_coverage = clut.coverage();
        if (diagnostics) diagnostics->push_back({word, static_cast<uint32_t>(image_coverage),
            static_cast<uint32_t>(image.words.size()), static_cast<uint16_t>(clut_coverage), pe});
        e.source_width = static_cast<uint16_t>(image.extent_x * pixels_per_word);
        e.source_height = image.extent_y;
        if (image_coverage && clut_coverage == pe) {
            e.indices.resize(size_t(kL3MaterialTextureWidth) * kL3MaterialTextureHeight);
            const uint16_t mask = page.bpp == 4 ? 15u : 255u;
            for (size_t index = 0; index < image.words.size(); ++index) {
                for (uint16_t pixel = 0; pixel < pixels_per_word; ++pixel)
                    e.indices[index * pixels_per_word + pixel] =
                        static_cast<uint8_t>((image.words[index] >> (pixel * page.bpp)) & mask);
            }
            e.palette_ps1 = std::move(clut.words);
            e.palette_entries = pe;
            e.resolved = true;
        }
        validate_entry(e);
        out.entries.push_back(std::move(e));
    }
    return out;
}

const MaterialCacheEntry* find_material_cache_entry(const MaterialCache& cache, uint32_t material_word) {
    const auto it = std::lower_bound(cache.entries.begin(), cache.entries.end(), material_word,
        [](const MaterialCacheEntry& e, uint32_t word) { return e.material_word < word; });
    return it != cache.entries.end() && it->material_word == material_word ? &*it : nullptr;
}

std::vector<uint8_t> cook_l3material_cache(const MaterialCache& cache) {
    if (cache.entries.size() > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("too many material cache entries");
    uint32_t resolved = 0;
    for (size_t i = 0; i < cache.entries.size(); ++i) {
        validate_entry(cache.entries[i]);
        if (i && cache.entries[i - 1].material_word >= cache.entries[i].material_word)
            throw std::runtime_error("material cache entries must be strictly sorted");
        if (cache.entries[i].resolved) ++resolved;
    }
    std::vector<uint8_t> o;
    const size_t records_bytes = cache.entries.size() * kL3MaterialRecordBytes;
    o.reserve(kHeaderBytes + records_bytes + size_t(resolved) * (size_t(kL3MaterialTextureWidth) * kL3MaterialTextureHeight + 512u));
    o.insert(o.end(), {'L','3','M','C'});
    wr_u16(o, kL3MaterialCacheVersion); wr_u16(o, kHeaderBytes);
    wr_u32(o, kCacheFlagRawPs1Palette | kCacheFlagPadded256Page);
    wr_u32(o, static_cast<uint32_t>(cache.entries.size()));
    wr_u32(o, resolved); wr_u32(o, static_cast<uint32_t>(cache.entries.size()) - resolved);
    wr_u16(o, kL3MaterialTextureWidth); wr_u16(o, kL3MaterialTextureHeight);
    wr_u16(o, kL3MaterialRecordBytes); wr_u16(o, 0);
    const uint32_t off_records = kHeaderBytes;
    const uint32_t off_payload = static_cast<uint32_t>(kHeaderBytes + records_bytes);
    wr_u32(o, off_records); wr_u32(o, off_payload);
    const size_t p_file = o.size(); wr_u32(o, 0);
    const size_t p_crc = o.size(); wr_u32(o, 0);
    if (o.size() != kHeaderBytes) throw std::runtime_error("L3MC header size bug");

    const size_t records_start = o.size();
    o.resize(o.size() + records_bytes, 0);
    for (size_t i = 0; i < cache.entries.size(); ++i) {
        const auto& e = cache.entries[i];
        const size_t r = records_start + i * kL3MaterialRecordBytes;
        auto p32 = [&](size_t off, uint32_t v) { patch_u32(o, r + off, v); };
        auto p16 = [&](size_t off, uint16_t v) { o[r + off] = static_cast<uint8_t>(v); o[r + off + 1] = static_cast<uint8_t>(v >> 8); };
        p32(0, e.material_word); p32(4, e.resolved ? kFlagResolved : 0u);
        o[r + 8] = e.bpp; o[r + 9] = e.abr; p16(10, e.palette_entries);
        p16(12, e.source_width); p16(14, e.source_height);
        if (e.resolved) {
            const uint32_t index_off = static_cast<uint32_t>(o.size());
            o.insert(o.end(), e.indices.begin(), e.indices.end());
            const uint32_t palette_off = static_cast<uint32_t>(o.size());
            for (uint16_t c : e.palette_ps1) wr_u16(o, c);
            p32(16, index_off); p32(20, static_cast<uint32_t>(e.indices.size()));
            p32(24, palette_off); p32(28, static_cast<uint32_t>(e.palette_ps1.size() * 2u));
        }
    }
    patch_u32(o, p_file, static_cast<uint32_t>(o.size()));
    patch_u32(o, p_crc, crc32(o.data() + kHeaderBytes, o.size() - kHeaderBytes));
    return o;
}

L3MaterialCacheHeader inspect_l3material_cache(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < kHeaderBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'M' || b[3] != 'C') throw std::runtime_error("not L3MC");
    L3MaterialCacheHeader h;
    h.version = rd_u16(&b[4]); h.header_bytes = rd_u16(&b[6]); h.flags = rd_u32(&b[8]);
    h.material_count = rd_u32(&b[12]); h.resolved_count = rd_u32(&b[16]); h.unresolved_count = rd_u32(&b[20]);
    h.texture_width = rd_u16(&b[24]); h.texture_height = rd_u16(&b[26]); h.record_bytes = rd_u16(&b[28]); h.reserved0 = rd_u16(&b[30]);
    h.off_records = rd_u32(&b[32]); h.off_payload = rd_u32(&b[36]); h.file_bytes = rd_u32(&b[40]); h.payload_crc32 = rd_u32(&b[44]);
    if (h.version != kL3MaterialCacheVersion || h.header_bytes != kHeaderBytes || h.record_bytes != kL3MaterialRecordBytes)
        throw std::runtime_error("unsupported L3MC");
    if (h.texture_width != kL3MaterialTextureWidth || h.texture_height != kL3MaterialTextureHeight) throw std::runtime_error("L3MC page dimensions mismatch");
    if (h.reserved0 != 0 || h.material_count != h.resolved_count + h.unresolved_count) throw std::runtime_error("L3MC header counts invalid");
    if (h.file_bytes != b.size()) throw std::runtime_error("L3MC file size mismatch");
    if (h.off_records != kHeaderBytes || uint64_t(h.off_records) + uint64_t(h.material_count) * h.record_bytes != h.off_payload || h.off_payload > b.size())
        throw std::runtime_error("L3MC section offsets invalid");
    if (verify_crc && crc32(b.data() + kHeaderBytes, b.size() - kHeaderBytes) != h.payload_crc32) throw std::runtime_error("L3MC CRC mismatch");
    return h;
}

MaterialCache load_l3material_cache(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3material_cache(b, verify_crc);
    MaterialCache out;
    out.entries.reserve(h.material_count);
    uint32_t resolved = 0;
    for (uint32_t i = 0; i < h.material_count; ++i) {
        const size_t r = size_t(h.off_records) + size_t(i) * h.record_bytes;
        MaterialCacheEntry e;
        e.material_word = rd_u32(&b[r]);
        const uint32_t flags = rd_u32(&b[r + 4]);
        if (flags & ~kFlagResolved) throw std::runtime_error("L3MC record has unknown flags");
        e.resolved = (flags & kFlagResolved) != 0;
        e.bpp = b[r + 8]; e.abr = b[r + 9]; e.palette_entries = rd_u16(&b[r + 10]);
        e.source_width = rd_u16(&b[r + 12]); e.source_height = rd_u16(&b[r + 14]);
        const uint32_t index_off = rd_u32(&b[r + 16]), index_bytes = rd_u32(&b[r + 20]);
        const uint32_t palette_off = rd_u32(&b[r + 24]), palette_bytes = rd_u32(&b[r + 28]);
        if (rd_u32(&b[r + 32]) != 0 || rd_u32(&b[r + 36]) != 0) throw std::runtime_error("L3MC record reserved field nonzero");
        if (e.resolved) {
            if (index_bytes != size_t(kL3MaterialTextureWidth) * kL3MaterialTextureHeight) throw std::runtime_error("L3MC resolved index size invalid");
            if (palette_bytes != uint32_t(e.palette_entries) * 2u) throw std::runtime_error("L3MC resolved palette size invalid");
            if (index_off < h.off_payload || uint64_t(index_off) + index_bytes > b.size() || palette_off < h.off_payload || uint64_t(palette_off) + palette_bytes > b.size())
                throw std::runtime_error("L3MC resolved payload range invalid");
            e.indices.assign(b.begin() + index_off, b.begin() + index_off + index_bytes);
            e.palette_ps1.reserve(e.palette_entries);
            for (uint16_t p = 0; p < e.palette_entries; ++p) e.palette_ps1.push_back(rd_u16(&b[palette_off + size_t(p) * 2u]));
            ++resolved;
        } else if (index_off != 0 || index_bytes != 0 || palette_off != 0 || palette_bytes != 0 || e.palette_entries != 0) {
            throw std::runtime_error("L3MC unresolved record has payload metadata");
        }
        validate_entry(e);
        if (!out.entries.empty() && out.entries.back().material_word >= e.material_word) throw std::runtime_error("L3MC records are not strictly sorted");
        out.entries.push_back(std::move(e));
    }
    if (resolved != h.resolved_count) throw std::runtime_error("L3MC resolved count mismatch");
    return out;
}

std::vector<uint8_t> material_page_rgba8(const MaterialCacheEntry& e) {
    validate_entry(e);
    if (!e.resolved) return {};
    std::vector<uint8_t> out(e.indices.size() * 4u, 0);
    for (size_t i = 0; i < e.indices.size(); ++i) {
        const uint8_t pi = e.indices[i];
        if (pi >= e.palette_ps1.size()) throw std::runtime_error("material page palette index out of range");
        const uint16_t c = e.palette_ps1[pi];
        out[i * 4u + 0] = expand5(c >> 0);
        out[i * 4u + 1] = expand5(c >> 5);
        out[i * 4u + 2] = expand5(c >> 10);
        // Black with STP (0x8000) remains visible; only the zero word is discarded.
        if (c == 0) out[i * 4u + 3] = 0;
        else out[i * 4u + 3] = (c & 0x8000u) ? 0x80u : 0xFFu;
    }
    return out;
}

} // namespace l3

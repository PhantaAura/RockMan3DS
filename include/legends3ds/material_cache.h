#pragma once
#include "material_resolver.h"
#include "st00_render.h"
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3MaterialCacheVersion = 1;
constexpr uint16_t kL3MaterialTextureWidth = 256;
constexpr uint16_t kL3MaterialTextureHeight = 256;
constexpr uint16_t kL3MaterialRecordBytes = 40;

struct MaterialCacheEntry {
    uint32_t material_word = 0;
    uint8_t bpp = 0;
    uint8_t abr = 0;
    uint16_t palette_entries = 0;
    uint16_t source_width = 0;
    uint16_t source_height = 0;
    bool resolved = false;
    // Resolved entries use a 256x256 page so original byte UVs can be submitted
    // directly by a backend. Overlapping image uploads are composed in VRAM
    // word coordinates. Unwritten portions of a page remain explicitly zero.
    std::vector<uint8_t> indices;
    std::vector<uint16_t> palette_ps1;
};

struct MaterialCache {
    std::vector<MaterialCacheEntry> entries;
};

struct MaterialVramDiagnostics {
    uint32_t material_word = 0;
    uint32_t image_words_written = 0;
    uint32_t image_words_total = 0;
    uint16_t palette_words_written = 0;
    uint16_t palette_words_total = 0;
};

struct L3MaterialCacheHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t material_count = 0;
    uint32_t resolved_count = 0;
    uint32_t unresolved_count = 0;
    uint16_t texture_width = 0;
    uint16_t texture_height = 0;
    uint16_t record_bytes = 0;
    uint16_t reserved0 = 0;
    uint32_t off_records = 0;
    uint32_t off_payload = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

// PS1 VRAM is stateful. This resolver treats resources as upload-ordered and
// selects the last matching image page / CLUT write instead of the first match.
// Retained as a legacy whole-resource diagnostic. Material cache construction
// below composes all overlapping physical VRAM writes, including partial pages.
MaterialResolution resolve_material_word_last_writer(
    uint32_t material_word,
    const std::vector<IndexedResource>& upload_order);

// Upload order is supplied by the caller. Each validated type1 image/CLUT and
// type4/type9 CLUT updates clipped physical word rectangles; later words win.
// A full requested CLUT is required. Unwritten image words remain zero UNKNOWN
// and coverage is available to host probes without storing owner data in source.
MaterialCache build_material_cache(
    const std::vector<uint32_t>& material_words,
    const std::vector<IndexedResource>& upload_order,
    std::vector<MaterialVramDiagnostics>* diagnostics = nullptr);

const MaterialCacheEntry* find_material_cache_entry(const MaterialCache& cache, uint32_t material_word);

std::vector<uint8_t> cook_l3material_cache(const MaterialCache& cache);
L3MaterialCacheHeader inspect_l3material_cache(const std::vector<uint8_t>& bytes, bool verify_crc = true);
MaterialCache load_l3material_cache(const std::vector<uint8_t>& bytes, bool verify_crc = true);

// Backend helper. Converts a resolved material page to linear RGBA8 while
// preserving the source PS1 palette data in the cache itself. PS1 color zero is
// transparent; STP is retained as 0x80 alpha for now so a backend can identify
// semitransparent palette entries without losing the bit.
std::vector<uint8_t> material_page_rgba8(const MaterialCacheEntry& entry);

} // namespace l3

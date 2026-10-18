#pragma once
#include "l3map.h"
#include "st00_render.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
constexpr uint16_t kMapInitializationVersion = 1;
constexpr uint32_t kMapInitializationHeaderBytes = 48;
constexpr uint32_t kMapInitializationEntryBytes = 16;
constexpr uint32_t kMapInitializationMaxIdentities = 2048;
constexpr uint32_t kMapInitializationMaxFileBytes = 48 + 2048 * 16;

// The source-backed stage0 startup guard preserves materialized seeds. Later
// stage5/6 variants and saved mutable state are not represented by this profile.
enum class MapInitializationProfile : uint32_t { Stage0Startup = 1 };

struct MapInitializationEntry {
    uint16_t idx_index = 0;
    uint16_t descriptor_index = 0;
    uint16_t render_record_index = 0;
    uint16_t expected_raw6 = 0;
    uint16_t seed = 0;
};

struct MapInitialization {
    MapInitializationProfile profile = MapInitializationProfile::Stage0Startup;
    uint32_t idx_count = 0;
    uint32_t descriptor_count = 0;
    uint32_t render_record_count = 0;
    uint32_t map_crc32 = 0;       // Full original cooked L3MP file.
    uint32_t render_crc32 = 0;    // Full original cooked L3RG file.
    uint32_t decoded_bytes = 0;
    std::vector<MapInitializationEntry> entries;
};

struct MapInitializationHeader {
    MapInitializationProfile profile = MapInitializationProfile::Stage0Startup;
    uint32_t idx_count = 0;
    uint32_t descriptor_count = 0;
    uint32_t render_record_count = 0;
    uint32_t entry_count = 0;
    uint32_t map_crc32 = 0;
    uint32_t render_crc32 = 0;
    uint32_t decoded_bytes = 0;
};

struct MapInitializationDiagnostics {
    uint32_t write_count = 0;
    uint32_t decoded_bytes = 0;
};

// Performs complete finite format/entry checks before any allocation.
MapInitializationHeader inspect_map_initialization(const std::vector<uint8_t>& bytes,
                                                   bool verify_crc = true);
MapInitialization load_map_initialization(const std::vector<uint8_t>& bytes,
                                         bool verify_crc = true);
std::vector<uint8_t> cook_map_initialization(const MapInitialization& initialization);

// Host-only derivation retains exact first HED match, zero terminator, masked
// identity and512-entry search cap. It never infers admission from the union of
// descriptors, or replaces unmatched IDX fields with a blanket selector.
MapInitialization derive_map_initialization(const St00Source& source,
                                            const std::vector<uint8_t>& hed,
                                            const std::vector<uint8_t>& stg,
                                            const RuntimeMap& map,
                                            const St00RenderSource& render,
                                            uint32_t map_crc32,
                                            uint32_t render_crc32,
                                            MapInitializationProfile profile);

// Validates all entries and bindings before the first write. Applying again to
// a changed map fails the expected-field checks. These APIs allocate nothing.
MapInitializationDiagnostics validate_map_initialization(const RuntimeMap& map,
                                                         const St00RenderSource& render,
                                                         const MapInitialization& initialization,
                                                         uint32_t map_crc32,
                                                         uint32_t render_crc32);
MapInitializationDiagnostics apply_map_initialization(RuntimeMap& map,
                                                      const St00RenderSource& render,
                                                      const MapInitialization& initialization,
                                                      uint32_t map_crc32,
                                                      uint32_t render_crc32);
}

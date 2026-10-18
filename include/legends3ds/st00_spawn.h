#pragma once
#include "legends3ds/stage_spawn.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
constexpr size_t kSt00SpawnAreaCount = 5;
constexpr uint32_t kSt00SpawnMaxRegions = 64;
constexpr uint32_t kSt00SpawnMaxRecords = 256;
// Source E0/00 state2 tests this story flag and skips records whose flag is set.
// The additional u32 belongs to the conditional 24-byte list, not a 20-byte
// static stage-object list. Unknown object families retain their original fields.
struct St00ConditionalSpawn {
    StageObjectRecord object;
    uint32_t predicate = 0;
};
struct St00SpawnRegion {
    int16_t id = 0;
    int16_t qualifier = 0;
    int8_t min_x = 0, min_z = 0;
    uint8_t width = 0, depth = 0;
    std::vector<St00ConditionalSpawn> records;
};
struct St00SpawnArea { std::vector<St00SpawnRegion> regions; };
struct St00SpawnCatalog { std::array<St00SpawnArea, kSt00SpawnAreaCount> areas{}; };
struct L3St00SpawnHeader {
    uint32_t area_count = 0, region_count = 0, record_count = 0;
    uint32_t off_areas = 0, off_regions = 0, off_records = 0;
    uint32_t file_bytes = 0, payload_crc32 = 0;
};
// USA ST00 E0/00 descriptor table reached through 0x8011C898. Bounds are
// converted to neutral fields; PS1 pointers and code are never retained.
St00SpawnCatalog parse_st00_spawn_us(const std::vector<uint8_t>& overlay,
                                     uint32_t load_base = 0x80100000u);
std::vector<uint8_t> cook_l3st00_spawn(const St00SpawnCatalog& catalog);
// Validates canonical coverage, finite caps, record/predicate domains and CRC
// before allocating any decoded region or object vector.
L3St00SpawnHeader inspect_l3st00_spawn(const std::vector<uint8_t>& bytes,
                                     bool verify_crc = true);
St00SpawnCatalog load_l3st00_spawn(const std::vector<uint8_t>& bytes,
                                  bool verify_crc = true);
// Exact 0x80111880..0x801118CC gate: signed16 X/Z, floor divide by512,
// unsigned interval comparisons. Maximum boundary is exclusive.
bool st00_spawn_region_contains(const St00SpawnRegion&, int16_t player_x, int16_t player_z);
// Source scans the entire descriptor list. The last passing descriptor wins.
// A same-qualifier descriptor does not select a new spawn list. Caller must
// preserve state1 -> state2 next-tick ordering and commit qualifier/id after
// processing selected records; this helper does not allocate or mutate work.
std::optional<size_t> st00_spawn_selection(const St00SpawnArea&, int8_t current_qualifier,
                                          int16_t player_x, int16_t player_z);
} // namespace l3

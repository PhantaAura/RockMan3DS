#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr size_t kStageObjectRecordBytes = 20;

// Raw 20-byte runtime object record consumed by the game's common stage-object parser.
// Class/id semantics are intentionally not named until linked to behavior. The three signed
// coordinates at +0x0C/+0x0E/+0x10 are exposed because their use is independently observed.
struct StageObjectRecord {
    std::array<uint8_t, kStageObjectRecordBytes> raw{};
    uint8_t id = 0;
    uint8_t object_class = 0;
    std::array<uint16_t, 5> words_02_to_0a{};
    int16_t x = 0, y = 0, z = 0;
    uint16_t tail_12 = 0;
};

struct StageObjectList {
    std::vector<StageObjectRecord> records;
    size_t bytes_consumed = 0; // includes the 0xFF terminator byte, not arbitrary trailing data
};

StageObjectList parse_stage_object_list(const uint8_t* data, size_t bytes);
StageObjectList parse_stage_object_list(const std::vector<uint8_t>& data, size_t offset = 0);

} // namespace l3

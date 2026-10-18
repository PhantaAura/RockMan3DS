#pragma once
#include "legends3ds/qol_settings.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace l3 {

constexpr uint16_t kL3SaveVersion = 2;
constexpr size_t kL3StoryFlagBytes = 256; // retail flag space 0x000..0x7FF

struct RuntimeSaveState {
    uint32_t checkpoint_id = 0;
    uint16_t area_id = 0;
    uint16_t player_yaw = 0;
    int32_t player_x = 0;
    int32_t player_y = 0;
    int32_t player_z = 0;
    uint16_t hp = 0;
    uint16_t hp_max = 0;
    uint32_t play_frames = 0;
    std::array<uint8_t, kL3StoryFlagBytes> story_flags{};
    L3QolSettings qol{};
};

struct L3SaveHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t payload_bytes = 0;
    uint32_t payload_crc32 = 0;
};

std::vector<uint8_t> cook_l3save(const RuntimeSaveState& state);
L3SaveHeader inspect_l3save(const std::vector<uint8_t>& bytes, bool verify_crc = true);
RuntimeSaveState load_l3save(const std::vector<uint8_t>& bytes, bool verify_crc = true);

void runtime_story_flag_set(RuntimeSaveState& state, uint16_t bit, bool value);
bool runtime_story_flag_get(const RuntimeSaveState& state, uint16_t bit);

// Host/3DS-neutral safe-write policy: write a temporary file, optionally preserve
// the previous valid file as .bak, then replace the destination.  The 3DS backend
// can use the same byte format with its own filesystem wrapper if desired.
bool write_l3save_atomic(const std::filesystem::path& path,
                         const RuntimeSaveState& state,
                         bool preserve_backup = true);

} // namespace l3

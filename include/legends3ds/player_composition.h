#pragma once
#include "pbd_model.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3PlayerCompositionVersion = 1;
constexpr size_t kPlayerComponentSlots = 5;
constexpr size_t kBaselinePlayerMeshEntries = 18;

// Renderer order proven from ROCK_NEO.EXE's player draw loop.  A component slot is a separately
// loaded PBD mesh bank; each mesh entry is attached to one of the 15 transformed player matrices.
enum class PlayerComponentSlot : uint8_t {
    Body = 0,
    Head = 1,
    RightArm = 2,
    LeftArm = 3,
    Feet = 4,
};

struct PlayerMeshAttachment {
    uint8_t component_slot = 0;
    uint8_t mesh_index = 0;
    uint8_t part_id = 0;
    uint8_t transform_id = 0;
};

struct PlayerComposition {
    std::vector<PlayerMeshAttachment> attachments;
};

struct L3PlayerCompositionHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t component_slots = 0;
    uint32_t attachment_count = 0;
    uint32_t off_attachments = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

// Builds the baseline/tutorial composition from five already-parsed PBDs in renderer slot order:
// body, head, right arm, left arm, feet.  The transform lookup table is direct executable evidence.
PlayerComposition build_baseline_player_composition(const std::array<PbdModel, kPlayerComponentSlots>& components);

std::vector<uint8_t> cook_l3player_composition(const PlayerComposition& composition);
L3PlayerCompositionHeader inspect_l3player_composition(const std::vector<uint8_t>& bytes,
                                                        bool verify_crc = true);
// Runtime reader for pointer-free cooked L3PC attachment data.
PlayerComposition load_l3player_composition(const std::vector<uint8_t>& bytes,
                                             bool verify_crc = true);

} // namespace l3

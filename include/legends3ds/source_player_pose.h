#pragma once

#include "legends3ds/child_runtime.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

enum class SourcePlayerPoseStatus : std::uint8_t {
    SourceBackedPartial,
    NeedsSkeleton,
    NeedsPose,
    NeedsReference,
    NeedsScratchShift,
    UnsupportedScratchShift,
    NeedsLimbPoseSelector,
    NeedsAlternateLimbPose,
    NeedsPlayerPosition,
    InvalidHierarchy
};

struct SourcePlayerPoseInputs {
    const PbdMainSkeleton* skeleton = nullptr;
    const PbdMainPoseBank* primary_pose = nullptr;
    const ReferenceTables* reference = nullptr;
    std::optional<std::uint8_t> scratch_shift_1f800074;
    std::optional<std::uint8_t> limb_pose_selector_106;
    std::optional<std::array<std::int16_t,3>> player_position_14;
    std::uint16_t heading_56 = 0;
    std::uint16_t heading_offset_116 = 0;
};

struct SourcePlayerIntegerPart {
    std::array<std::int16_t,9> rotation_q12{};
    std::array<std::int32_t,3> accumulated_translation{};
    std::array<std::int16_t,3> position_3a8{};
};

struct SourcePlayerIntegerPose {
    SourcePlayerPoseStatus status = SourcePlayerPoseStatus::NeedsPose;
    std::uint8_t part_count = 0;
    std::uint8_t scratch_shift = 0;
    std::array<SourcePlayerIntegerPart,kPbdMainPosePartCount> parts{};
};

// Fixed-point ordinary-pose subset of USA ROCK_NEO.EXE
// 0x800325D4 -> 0x800331A0. The original routine has a second eight-part
// pose bank selected by work+0x106 for arm overlays. The current L3AN format
// retains the primary 15-part pose only, so selectors 1..3 hold explicitly.
// No renderer float matrix, guessed joint offset, or gameplay constant is used.
SourcePlayerIntegerPose produce_source_player_integer_pose_800325d4(
    const SourcePlayerPoseInputs&) noexcept;

struct SourcePlayerMuzzleReceipt {
    SourcePlayerPoseStatus status = SourcePlayerPoseStatus::NeedsPose;
    std::array<std::uint16_t,3> muzzle_3c8_part4{};
    std::array<std::uint16_t,3> muzzle_3e0_part7{};
};

// Exact field identity consumed by 0x80035724: stack argument nonzero selects
// part4/work+3C8, while zero selects part7/work+3E0. This only copies an
// admitted integer pose and does not establish firing cadence or projectile AI.
SourcePlayerMuzzleReceipt source_player_muzzles_800331a0(
    const SourcePlayerIntegerPose&) noexcept;

// Populate only the two pose-owned lanes. Selection, subtype, counters,
// player class and heading remain with their independent source owners.
bool bind_source_player_muzzles_80035724(const SourcePlayerMuzzleReceipt&,
                                         SourceBusterPlayer&) noexcept;

} // namespace l3

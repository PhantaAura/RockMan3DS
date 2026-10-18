#pragma once

#include "legends3ds/child_runtime.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

// Neutral cooked form of the five signed halfwords read by source80034EB8.
// The retail table is supplied by the owner's cooker and is never compiled in.
struct SourcePlayerTargetProfile {
    std::int16_t maximum_depth = 0;
    std::int16_t minimum_x = 0;
    std::int16_t maximum_x = 0;
    std::int16_t minimum_y = 0;
    std::int16_t maximum_y = 0;
};

bool decode_source_player_target_profile(
    const std::vector<std::uint8_t>& bytes, SourcePlayerTargetProfile& out) noexcept;

enum class SourcePlayerBusterStatus : std::uint8_t {
    SourceBackedPartial,
    Inactive,
    NeedsTargetProfile,
    NeedsPlayerPoint,
    NeedsTargetMode,
    NeedsFireFlags,
    NeedsProxyBank,
    LeaseHeld,
    ProjectionHeld,
    BusterHeld
};

struct SourcePlayerBusterInputs {
    bool active = false;                         // player work+00 bit0
    std::optional<std::uint8_t> target_mode_49e; // GP+049E for player0
    std::optional<std::uint8_t> fire_flags_174;  // work+174 bits20/10
    std::optional<std::array<std::int16_t,3>> player_point_14;
    std::optional<SourcePlayerTargetProfile> target_profile_8008994c;
    std::optional<std::array<std::uint32_t,8>> camera_1f800008;
};

struct SourcePlayerBusterResult {
    SourcePlayerBusterStatus status = SourcePlayerBusterStatus::NeedsTargetProfile;
    SourcePlayerProxyStatus proxy_status = SourcePlayerProxyStatus::SourceBackedPartial;
    std::array<std::int32_t,2> selected_proxy{{-1,-1}}; // [broad20, window10]
    std::array<bool,2> spawn_attempted{{false,false}};
    std::array<SourceBusterSpawn,2> spawns{};
    std::uint16_t player_depth_plus_50 = 0;
    std::size_t projected_proxies = 0;
};

// Bounded source80034EB8 player0 path. It consumes the established80035508
// lease/GTE owner, projects the player and shared source proxy bank, preserves
// composite-row advancement, then calls the exact80035724 scalar producer for
// bits20 and10. It does not implement projectile callbacks, damage, or cadence.
SourcePlayerBusterResult source_player_select_and_spawn_80034eb8(
    ChildRuntime& runtime, SourceGteRtpsState& gte,
    SourceBusterPlayer& player, std::optional<std::uint32_t>& proxy_mask_80098798,
    const SourcePlayerBusterInputs& input) noexcept;

} // namespace l3

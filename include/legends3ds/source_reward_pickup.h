#pragma once
#include "legends3ds/player_contact_runtime.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_audio_queue.h"
#include "legends3ds/source_effects.h"
#include "legends3ds/source_rng_service.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

// Neutral cooked L3RW/1 tables (tools/cook_source_reward_profile.py). The
// retail values are cooked from the owner's executable and never compiled in.
struct SourceRewardProfile {
    bool loaded = false;
    std::array<std::array<uint8_t,16>,16> size_rolls_8008a5a8{};
    std::array<uint16_t,17> spread_80097efa{};
    std::array<uint16_t,5> scale_80097ee0{};
    std::array<int16_t,5> speed_80097f1c{}, lift_80097f24{};
    std::array<std::array<uint16_t,4>,2> radius_80097eec{};
    std::array<std::array<RetailPlayerBounds,4>,2> bounds_8008a618{};
    std::array<uint16_t,24> awards_8008a678{};   // [selector*8 + money0..3, energy0..3]
};
bool load_source_reward_profile(const std::vector<uint8_t>& bytes, SourceRewardProfile& out) noexcept;

// Subtype 0x87 (135) pickup effect. Work layout follows the original 72-byte
// effect: +0 flags, +2 subtype, +3 kind (0 shard / 1 energy), +8 state,
// +9 sparkle phase, +10 size, +11 timer, +C/+E spin angles, +2C/2E/30
// velocity, +32/34/36 acceleration, +38/3A/3C position, +3E scale,
// +40 colour word, +44 original model address.
constexpr uint8_t kSourceRewardPickupSubtype = 0x87;

enum class SourceRewardStatus : uint8_t {
    SourceBackedPartial, Inactive, NeedsProfile, NeedsRng, NeedsTrig,
    UnsupportedTable, UnsupportedSelector, NeedsContact, RngFailed
};
struct SourceRewardExpansion {
    SourceRewardStatus status = SourceRewardStatus::NeedsProfile;
    uint8_t spawned = 0, discarded = 0, pool_full = 0;
};
// Exact 0x80046B34 into the shared effect pool. All-or-nothing: on any hold
// the pool and RNG stream are unchanged and the caller keeps the request.
SourceRewardExpansion expand_source_reward_80046b34(SourceEffectPool& pool,
    const SourceRewardProfile& profile, const ReferenceTables& refs,
    SourceRngService& rng, uint64_t source_generation,
    const std::array<int16_t,4>& point_180, uint32_t descriptor) noexcept;

struct SourceRewardWorld {
    const RuntimeMap* map = nullptr;
    size_t area_index = 0;
    std::array<int16_t,3> player{};       // 0x800B51C4/C6/C8
    int8_t stats_selector_800c1b6b = 1;   // fresh game 0x8001326C
    SourceAudioQueue* audio = nullptr;
};
struct SourceRewardTick {
    SourceRewardStatus status = SourceRewardStatus::Inactive;
    uint8_t updated = 0, held = 0, collected = 0, released = 0;
    int32_t money_awarded = 0, energy_awarded = 0;
};
// 0x800173B4 dispatch of every live subtype-0x87 effect through 0x8004602C
// and its eight state owners (0x8008A6A8). Collection (0x80046A0C) applies
// 0x80043294 to `money_800c1b2c` and 0x80043340 to `hp` (player +0x6E,
// clamped to +0xB0). A pickup whose contact or RNG cannot be resolved holds
// unchanged for that frame. Rendering-only work (0x8002F9C4 resource lookup,
// rotation/scale matrices, sparkle sprites) is not reproduced.
SourceRewardTick tick_source_reward_pickups(SourceEffectPool& pool,
    const SourceRewardProfile& profile, const ReferenceTables& refs,
    SourceRngService& rng, uint64_t source_generation, const SourceRewardWorld& world,
    int32_t& money_800c1b2c, int16_t& hp, int16_t hp_max) noexcept;

} // namespace l3

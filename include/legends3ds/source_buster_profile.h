#pragma once

#include "legends3ds/child_runtime.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr std::uint16_t kSourceBusterProfileVersion=2;
constexpr std::size_t kSourceBusterSelectionCount=16;
constexpr std::size_t kSourceBusterLevelCount=8;
struct SourceBusterSelectionProfile {
    std::uint8_t subtype_80089a8b=0;
    std::int16_t initial_counter_4fa=0;
};
// One PTR_DAT_80089EAC row: +0 damage (0x800375D4), +4 range (0x80037634).
struct SourceBusterLevelRow { std::array<std::int16_t,4> words{}; };
struct SourceBusterProfile {
    std::array<SourceBusterSelectionProfile,kSourceBusterSelectionCount> selection{};
    std::array<std::array<SourceBusterLevelRow,kSourceBusterLevelCount>,kSourceBusterSelectionCount> levels{};
    std::array<std::uint8_t,kSourceBusterLevelCount> shot_radius_80097dcc{};
    std::array<std::uint8_t,kSourceBusterLevelCount> shot_speed_80097dd4{};
    std::array<std::uint8_t,8> impact82_timers_8008a240{};
};

// Pointer-free L3BP/2 data cooked from the owner executable's 16 twelve-byte
// selection rows and the row-zero counter reached through PTR_DAT_80089EAC.
// Throws on magic, version, size, reserved bytes or CRC failure.
SourceBusterProfile load_source_buster_profile(const std::vector<std::uint8_t>&,
                                                bool verify_crc=true);

// Exact fresh branch subset of 0x8003BE6C -> 0x80037534 relevant to the
// current two-lane 0x80035724 producer: selectors +170/+171 are zero and both
// resolve subtype/counter selection zero. Class, heading and muzzle coordinates
// remain independent owners and are intentionally untouched.
void initialize_source_buster_fresh_lanes_80037534(
    const SourceBusterProfile&,SourceBusterPlayer&) noexcept;

// Restore branch of 0x8003BE6C: +170/+171 come from the game block
// 0x800B5F88/89. MEASURED (PCSX-Redux OpenBIOS, new game, first tutorial
// control): +170 = 0, +171 = 1, so Square fires lane +171 = subtype 0x81.
// The block's new-game writer is not traced; these two bytes are inputs.
void initialize_source_buster_lanes_8003be6c(const SourceBusterProfile&,
    SourceBusterPlayer&,std::uint8_t selection_170,std::uint8_t selection_171) noexcept;

// Subtype-0x81 shot stats for player level bytes +0x4FC (damage, radius) and
// +0x4FE (range, speed). 0x80043E78 and 0x80043A8C read weapon row 1.
std::optional<SourceBusterShot81Stats> source_buster_shot81_stats(const SourceBusterProfile&,
    std::int8_t level_4fc,std::int8_t level_4fe) noexcept;

} // namespace l3

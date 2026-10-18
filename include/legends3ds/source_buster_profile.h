#pragma once

#include "legends3ds/child_runtime.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr std::uint16_t kSourceBusterProfileVersion=1;
constexpr std::size_t kSourceBusterSelectionCount=16;
struct SourceBusterSelectionProfile {
    std::uint8_t subtype_80089a8b=0;
    std::int16_t initial_counter_4fa=0;
};
struct SourceBusterProfile {
    std::array<SourceBusterSelectionProfile,kSourceBusterSelectionCount> selection{};
};

// Pointer-free L3BP/1 data cooked from the owner executable's 16 twelve-byte
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

} // namespace l3

#pragma once
#include "legends3ds/source_event_phase2.h"
#include <array>
#include <vector>

namespace l3 {
// Owner-local decoded scalars and initial LOAD data, not executable code.
// These cold-boot snapshots may only initialize a fresh diagnostic owner.
// Scene resets and area transitions must not promote retained bytes to known.
constexpr size_t kSourceEventPhase2PackageBytes=368;
struct SourceEventPhase2Package {
    SourceEventPhase2Profile profile;
    std::array<uint8_t,32> boot_descriptors{};
    std::array<uint8_t,256> boot_command_records{};
};
SourceEventPhase2Package load_source_event_phase2_package(const std::vector<uint8_t>& bytes);
} // namespace l3

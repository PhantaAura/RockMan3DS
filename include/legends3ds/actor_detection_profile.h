#pragma once
#include "legends3ds/actor_detection.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
// Keyed neutral owner-local profile metadata. Missing IDs are unknown, never
// replaced with another actor's row. The bounded table has no heap ownership.
using ActorDetectionProfiles = std::array<std::optional<ActorDetectionProfile>, 256>;
constexpr size_t kActorDetectionProfileHeaderBytes = 32;
ActorDetectionProfiles load_l3actor_detection_profiles(const uint8_t* bytes, size_t size);
std::vector<uint8_t> encode_l3actor_detection_profiles(const ActorDetectionProfiles& profiles);
} // namespace l3

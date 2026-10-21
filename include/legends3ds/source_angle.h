#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {

// Values must come from an explicitly verified owner-data profile. The
// original12-word table is not embedded in distributed source.
using SourceAngleSteps = std::array<int32_t, 12>;

constexpr size_t kActorAnglePackageBytes = 80;

// Optional L3AHv1 sidecar: fixed32-byte header and48-byte little-endian
// table payload. Strict length/header/reserved/CRC validation; no heap storage.
// Missing file handling belongs to the caller, not a manufactured default.
SourceAngleSteps load_l3actor_angle(const uint8_t* bytes, size_t size);
std::array<uint8_t, kActorAnglePackageBytes> encode_l3actor_angle(
    const SourceAngleSteps& steps) noexcept;

// Exact integer arithmetic of owner8007A414, including32-bit wrapping and
// signed arithmetic shifts. This is not a floating-point atan approximation.
int32_t source_catan_8007a414(int32_t ratio, const SourceAngleSteps& steps) noexcept;

// Owner80031198 consumes signed16 raw coordinates at position+0 and+4.
// The full original result-register bits are returned. Missing table data
// rejects non-axis queries; the original axis branch needs no table.
std::optional<uint32_t> source_heading_80031198(
    int16_t origin_x, int16_t origin_z, int16_t target_x, int16_t target_z,
    const SourceAngleSteps* steps) noexcept;

// Owner800310CC: the same heading over 16.16 fixed positions (+0x1C/+0x24):
// dz = (target.z - origin.z) >> 12, dx = target.x - origin.x (no shift).
std::optional<uint32_t> source_heading_800310cc(
    int32_t origin_x, int32_t origin_z, int32_t target_x, int32_t target_z,
    const SourceAngleSteps* steps) noexcept;

} // namespace l3

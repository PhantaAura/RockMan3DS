#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {
// Original80092D38:192 signed16 lookup records; owner-data sidecar only.
using SourceNormalizationLookup = std::array<int16_t, 192>;
constexpr size_t kSourceNormalizationPackageBytes = 416;
SourceNormalizationLookup load_l3normalization_lookup(const uint8_t* bytes, size_t size);
std::array<uint8_t, kSourceNormalizationPackageBytes> encode_l3normalization_lookup(
    const SourceNormalizationLookup& lookup) noexcept;

// Exact supported integer/GTE field semantics used by8007A694. Signed16 IR
// narrowing is preserved. Zero returns zero independently of unknowntabledata;
// overflowing source signed ADD domains reject. No floatingpoint square root.
std::optional<std::array<int16_t, 3>> source_normalize_8007a694(
    const std::array<int32_t, 3>& vector, const SourceNormalizationLookup* lookup) noexcept;

// Original80033F04 with actual detector arguments turn357 / threshold0FF069DA.
// Retains wrapped dot branch, OP crossproducts, CPU halfword narrowing, MVMVA
// candidate order/comparison, and the original lookup normalizer.
std::optional<std::array<int16_t, 3>> source_detection_interpolate_80033f04(
    const std::array<int16_t, 3>& normalized, const std::array<int16_t, 3>& previous,
    const SourceNormalizationLookup* lookup) noexcept;
} // namespace l3

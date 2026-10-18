#include "legends3ds/actor_detection_math.h"
#include "legends3ds/l3map.h"
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
uint16_t get16(const uint8_t* p) noexcept { return static_cast<uint16_t>(p[0] | uint16_t(p[1]) << 8); }
uint32_t get32(const uint8_t* p) noexcept { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
void put32(uint8_t* p, uint32_t value) noexcept { for (unsigned i = 0; i != 4; ++i) p[i] = static_cast<uint8_t>(value >> (8U * i)); }
int16_t narrow(uint32_t bits) noexcept { bits &= UINT32_C(65535); return bits < 32768 ? static_cast<int16_t>(bits) : static_cast<int16_t>(static_cast<int32_t>(bits) - 65536); }
int32_t word(uint32_t bits) noexcept { return bits < UINT32_C(0x80000000) ? static_cast<int32_t>(bits) : static_cast<int32_t>(static_cast<int64_t>(bits) - INT64_C(4294967296)); }
int32_t shift(int64_t value, unsigned amount) noexcept {
    if (value >= 0) return static_cast<int32_t>(value / (INT64_C(1) << amount));
    return -static_cast<int32_t>((-value + (INT64_C(1) << amount) - 1) / (INT64_C(1) << amount));
}
std::array<int32_t, 3> cross(const std::array<int32_t, 3>& a, const std::array<int32_t, 3>& b) noexcept {
    std::array<int16_t, 3> aa{}, bb{};
    for (size_t i = 0; i != 3; ++i) { aa[i] = narrow(static_cast<uint32_t>(a[i])); bb[i] = narrow(static_cast<uint32_t>(b[i])); }
    return {{shift(static_cast<int64_t>(aa[1]) * bb[2] - static_cast<int64_t>(aa[2]) * bb[1], 12),
             shift(static_cast<int64_t>(aa[2]) * bb[0] - static_cast<int64_t>(aa[0]) * bb[2], 12),
             shift(static_cast<int64_t>(aa[0]) * bb[1] - static_cast<int64_t>(aa[1]) * bb[0], 12)}};
}
int32_t dot(const std::array<int16_t, 3>& a, const std::array<int32_t, 3>& b) noexcept {
    uint32_t total = 0;
    for (size_t i = 0; i != 3; ++i) total += static_cast<uint32_t>(a[i]) * static_cast<uint32_t>(b[i]);
    return word(total);
}
} // namespace
SourceNormalizationLookup load_l3normalization_lookup(const uint8_t* bytes, size_t size) {
    if (bytes == nullptr || size != kSourceNormalizationPackageBytes) throw std::runtime_error("L3NR: exact416-byte package required");
    if (bytes[0] != 'L' || bytes[1] != '3' || bytes[2] != 'N' || bytes[3] != 'R' || get16(bytes + 4) != 1 ||
        get16(bytes + 6) != 32 || get32(bytes + 8) != size || get32(bytes + 12) != 192 ||
        get32(bytes + 20) != 0 || get32(bytes + 24) != 0 || get32(bytes + 28) != 0)
        throw std::runtime_error("L3NR: unsupported or noncanonical header");
    if (crc32(bytes + 32, 384) != get32(bytes + 16)) throw std::runtime_error("L3NR: payload CRC mismatch");
    SourceNormalizationLookup lookup{};
    for (size_t i = 0; i != lookup.size(); ++i) lookup[i] = narrow(get16(bytes + 32 + i * 2));
    return lookup;
}
std::array<uint8_t, kSourceNormalizationPackageBytes> encode_l3normalization_lookup(
    const SourceNormalizationLookup& lookup) noexcept {
    std::array<uint8_t, kSourceNormalizationPackageBytes> bytes{};
    bytes[0] = 'L'; bytes[1] = '3'; bytes[2] = 'N'; bytes[3] = 'R'; bytes[4] = 1; bytes[6] = 32;
    put32(bytes.data() + 8, static_cast<uint32_t>(bytes.size())); put32(bytes.data() + 12, 192);
    for (size_t i = 0; i != lookup.size(); ++i) {
        const uint16_t value = static_cast<uint16_t>(lookup[i]);
        bytes[32 + i * 2] = static_cast<uint8_t>(value); bytes[33 + i * 2] = static_cast<uint8_t>(value >> 8);
    }
    put32(bytes.data() + 16, crc32(bytes.data() + 32, 384)); return bytes;
}
std::optional<std::array<int16_t, 3>> source_normalize_8007a694(
    const std::array<int32_t, 3>& vector, const SourceNormalizationLookup* lookup) noexcept {
    std::array<int16_t, 3> ir{}; uint64_t sum = 0;
    for (size_t i = 0; i != 3; ++i) {
        ir[i] = narrow(static_cast<uint32_t>(vector[i]));
        sum += static_cast<uint64_t>(static_cast<int64_t>(ir[i]) * ir[i]);
    }
    if (sum == 0) return std::array<int16_t, 3>{};
    if (sum > static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) || lookup == nullptr) return std::nullopt;
    uint32_t leading = 0, bits = static_cast<uint32_t>(sum);
    while ((bits & UINT32_C(0x80000000)) == 0) { ++leading; bits <<= 1; }
    leading &= ~1U;
    const uint32_t exponent = (31U - leading) >> 1;
    const uint32_t scaled = leading >= 24 ? static_cast<uint32_t>(sum) << (leading - 24) : static_cast<uint32_t>(sum) >> (24 - leading);
    if (scaled < 64 || scaled > 255) return std::nullopt;
    const int16_t factor = (*lookup)[scaled - 64];
    std::array<int16_t, 3> out{};
    for (size_t i = 0; i != 3; ++i) out[i] = narrow(static_cast<uint32_t>(shift(static_cast<int64_t>(factor) * ir[i], exponent)));
    return out;
}
std::optional<std::array<int16_t, 3>> source_detection_interpolate_80033f04(
    const std::array<int16_t, 3>& normalized, const std::array<int16_t, 3>& previous,
    const SourceNormalizationLookup* lookup) noexcept {
    const std::array<int32_t, 3> n{{normalized[0], normalized[1], normalized[2]}};
    const std::array<int32_t, 3> v{{previous[0], previous[1], previous[2]}};
    constexpr uint32_t threshold = UINT32_C(0x0ff069da);
    if (word(static_cast<uint32_t>(dot(normalized, v)) << 4) >= word(threshold)) return normalized;
    const auto perpendicular = cross(n, v);
    const auto secondary = cross(v, perpendicular);
    const auto candidate = [&](int16_t turn) {
        std::array<int32_t, 3> out{};
        for (size_t i = 0; i != 3; ++i) {
            const int16_t column0 = narrow(static_cast<uint32_t>(secondary[i]));
            const int16_t column2 = previous[i];
            out[i] = shift(static_cast<int64_t>(column0) * turn + static_cast<int64_t>(column2) * static_cast<int16_t>(threshold >> 16), 12);
        }
        return out;
    };
    const auto positive = candidate(357), negative = candidate(-357);
    // Original candidates are (+/-357,0,threshold>>16), so matrix
    // column0 is the secondary cross vector and column1 contributes zero.
    const auto& selected = dot(normalized, negative) < dot(normalized, positive) ? positive : negative;
    return source_normalize_8007a694(selected, lookup);
}
} // namespace l3

#include "legends3ds/source_angle.h"
#include "legends3ds/l3map.h"
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
int32_t signed_word(uint32_t bits) noexcept {
    if (bits <= static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
        return static_cast<int32_t>(bits);
    return static_cast<int32_t>(static_cast<int64_t>(bits) - INT64_C(4294967296));
}
uint32_t arithmetic_shift(uint32_t bits, unsigned shift) noexcept {
    if (shift == 0) return bits;
    const uint32_t sign = (bits & UINT32_C(0x80000000)) != 0
        ? UINT32_MAX << (32U - shift) : 0;
    return (bits >> shift) | sign;
}
uint32_t get32(const uint8_t* bytes) noexcept {
    return static_cast<uint32_t>(bytes[0]) | static_cast<uint32_t>(bytes[1]) << 8 |
        static_cast<uint32_t>(bytes[2]) << 16 | static_cast<uint32_t>(bytes[3]) << 24;
}
uint16_t get16(const uint8_t* bytes) noexcept {
    return static_cast<uint16_t>(static_cast<uint16_t>(bytes[0]) |
        static_cast<uint16_t>(static_cast<uint16_t>(bytes[1]) << 8));
}
void put32(uint8_t* bytes, uint32_t value) noexcept {
    for (unsigned i = 0; i != 4; ++i) bytes[i] = static_cast<uint8_t>(value >> (8U * i));
}
} // namespace

SourceAngleSteps load_l3actor_angle(const uint8_t* bytes, size_t size) {
    if (bytes == nullptr || size != kActorAnglePackageBytes)
        throw std::runtime_error("L3AH: exact80-byte package required");
    if (bytes[0] != 'L' || bytes[1] != '3' || bytes[2] != 'A' || bytes[3] != 'H' ||
        get16(bytes + 4) != 1 || get16(bytes + 6) != 32 ||
        get32(bytes + 8) != kActorAnglePackageBytes || get32(bytes + 12) != 12 ||
        get32(bytes + 20) != 0 || get32(bytes + 24) != 0 || get32(bytes + 28) != 0)
        throw std::runtime_error("L3AH: unsupported or noncanonical header");
    if (crc32(bytes + 32, 48) != get32(bytes + 16))
        throw std::runtime_error("L3AH: payload CRC mismatch");
    SourceAngleSteps steps{};
    for (size_t i = 0; i != steps.size(); ++i) steps[i] = signed_word(get32(bytes + 32 + i * 4));
    return steps;
}

std::array<uint8_t, kActorAnglePackageBytes> encode_l3actor_angle(
    const SourceAngleSteps& steps) noexcept {
    std::array<uint8_t, kActorAnglePackageBytes> bytes{};
    bytes[0] = 'L'; bytes[1] = '3'; bytes[2] = 'A'; bytes[3] = 'H';
    bytes[4] = 1; bytes[6] = 32;
    put32(bytes.data() + 8, static_cast<uint32_t>(kActorAnglePackageBytes));
    put32(bytes.data() + 12, static_cast<uint32_t>(steps.size()));
    for (size_t i = 0; i != steps.size(); ++i)
        put32(bytes.data() + 32 + i * 4, static_cast<uint32_t>(steps[i]));
    put32(bytes.data() + 16, crc32(bytes.data() + 32, 48));
    return bytes;
}

int32_t source_catan_8007a414(int32_t ratio, const SourceAngleSteps& steps) noexcept {
    uint32_t x = 0x1000;
    uint32_t y = static_cast<uint32_t>(ratio);
    uint32_t angle = 0;
    for (size_t i = 0; i != steps.size(); ++i) {
        const uint32_t dx = arithmetic_shift(x, static_cast<unsigned>(i));
        const uint32_t dy = arithmetic_shift(y, static_cast<unsigned>(i));
        const uint32_t step = static_cast<uint32_t>(steps[i]);
        if ((y & UINT32_C(0x80000000)) != 0) {
            x -= dy;
            y += dx;
            angle -= step;
        } else {
            x += dy;
            y -= dx;
            angle += step;
        }
    }
    return signed_word(angle);
}

std::optional<uint32_t> source_heading_80031198(
    int16_t origin_x, int16_t origin_z, int16_t target_x, int16_t target_z,
    const SourceAngleSteps* steps) noexcept {
    const int32_t dx = static_cast<int32_t>(target_x) - origin_x;
    const int32_t dz = static_cast<int32_t>(target_z) - origin_z;
    // SLL runs before division. Large signed16 differences deliberately wrap;
    // using dx/dz as floating point would change original headings.
    const uint32_t numerator_bits = static_cast<uint32_t>(dx) << 16;
    if (dz == 0) return (numerator_bits & UINT32_C(0x80000000)) != 0
        ? UINT32_C(0x400) : UINT32_C(0xc00);
    if (steps == nullptr) return std::nullopt;
    const int32_t denominator = dz * 16;
    const uint32_t dividend_bits = dz > 0 ? numerator_bits : 0U - numerator_bits;
    // A nonzero denominator is a multiple of16, so INT_MIN/-1 cannot occur.
    const int32_t ratio = signed_word(dividend_bits) / denominator;
    const uint32_t angle = static_cast<uint32_t>(source_catan_8007a414(ratio, *steps));
    if (dz > 0) return angle + UINT32_C(0x800);
    return (UINT32_C(0x1000) - angle) & UINT32_C(0xfff);
}

} // namespace l3

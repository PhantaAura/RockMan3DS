#pragma once

#include <array>
#include <cstdint>

namespace l3 {

// Original leaf setters 8007B0F8 (FarColor/control21..23) and 8007B0D8
// (BackColor/control13..15) shift the complete argument left four bits.
// They have different register owners despite sharing scalar arithmetic.
struct SourceFarColorControls { std::array<uint32_t, 3> control21_23{}; };
struct SourceBackColorControls { std::array<uint32_t, 3> control13_15{}; };

inline constexpr SourceFarColorControls source_far_color_8007b0f8(
    const std::array<uint32_t, 3>& rgb) noexcept {
    return {{{rgb[0] << 4u, rgb[1] << 4u, rgb[2] << 4u}}};
}
inline constexpr SourceBackColorControls source_back_color_8007b0d8(
    const std::array<uint32_t, 3>& rgb) noexcept {
    return {{{rgb[0] << 4u, rgb[1] << 4u, rgb[2] << 4u}}};
}

// The proved flat textured constructors preserve RGB24 and source bit25,
// then install 24/2C. Source metadata bits are not a GP0 raw/Gouraud opcode.
inline constexpr uint32_t source_flat_textured_color_02ffffff(
    uint32_t modulation, bool quad) noexcept {
    return (modulation & 0x02ffffffu) | (quad ? 0x2c000000u : 0x24000000u);
}

struct SourceDpcsColor {
    uint32_t rgb_code = 0;
    std::array<int32_t, 3> mac{};
    std::array<int16_t, 3> ir{};
};

// Restricted DPCS0780010: sf=1, lm=0, FarColor owned by unsigned area-config
// bytes and an explicitly supplied IR0. This is scalar color math, not fog
// enablement, RTPS/H/depth ownership, general GTE emulation or hardware parity.
// Invalid premises leave output untouched. No guessed depth/camera/default FC.
inline bool source_dpcs_0780010(uint32_t rgb_code,
    const SourceFarColorControls& far, uint16_t ir0, SourceDpcsColor& output) noexcept {
    if (ir0 > 4096u) return false;
    for (uint32_t channel : far.control21_23)
        if (channel > 4080u || (channel & 15u)) return false;
    SourceDpcsColor next;
    next.rgb_code = rgb_code & 0xff000000u;
    for (unsigned channel = 0; channel < 3; ++channel) {
        const int32_t color = int32_t((rgb_code >> (channel * 8u)) & 255u);
        const int32_t difference = int32_t(far.control21_23[channel]) - color * 16;
        // The admitted byte/IR0 domain bounds both intermediates and this sum
        // inside int32 and makes the sum nonnegative. No MAC/IR saturation is
        // reachable; division therefore equals the GTE arithmetic shift.
        const int32_t numerator = color * 65536 + difference * int32_t(ir0);
        next.mac[channel] = numerator / 4096;
        next.ir[channel] = static_cast<int16_t>(next.mac[channel]);
        next.rgb_code |= uint32_t(next.mac[channel] / 16) << (channel * 8u);
    }
    output = next;
    return true;
}

} // namespace l3

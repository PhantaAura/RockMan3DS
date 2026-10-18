#pragma once

#include <cstdint>

namespace l3 {

// Citro3D RGB MODULATE followed by TEV scale 2 produces the PS1 /128 gain.
// Raw-texture primitives ignore their color; half intensity becomes unity gain.
inline constexpr float ps1_texture_color_input(uint8_t primitive_code, uint8_t color) {
    return (primitive_code & 1u) ? 0.5f : float(color) / 256.0f;
}

// Material RGBA retains STP as alpha128. An opaque textured primitive must
// display every nonzero texel opaquely; STP applies only with opcode bit1.
// Alpha TEV scale2 maps the retained0/128/255 mask to0/255/255 without
// modifying a shared texture page or losing its STP state.
inline constexpr bool ps1_textured_semitransparency_enabled(uint8_t primitive_code) {
    return (primitive_code & 2u) != 0;
}
inline constexpr float ps1_texture_alpha_scale(uint8_t primitive_code) {
    return ps1_textured_semitransparency_enabled(primitive_code) ? 1.0f : 2.0f;
}

} // namespace l3

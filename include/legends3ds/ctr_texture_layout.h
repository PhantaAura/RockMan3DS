#pragma once
#include <cstdint>
#include <vector>

namespace l3 {
// Convert linear RGBA8888 pixels into the PICA200 GPU_RGBA8 memory layout:
// 8x8 Morton-tiled texels, bytes A B G R per texel (matching tex3ds/citro3d).
// Width/height must be multiples of 8.
std::vector<uint8_t> ctr_rgba8_tiled_abgr(const std::vector<uint8_t>& linear_rgba,
                                          uint16_t width,
                                          uint16_t height);
}

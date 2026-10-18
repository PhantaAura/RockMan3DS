#include "legends3ds/ctr_texture_layout.h"
#include <stdexcept>

namespace l3 {
namespace {
uint32_t morton3(uint32_t x, uint32_t y) {
    uint32_t out = 0;
    for (uint32_t bit = 0; bit < 3; ++bit) {
        out |= ((x >> bit) & 1u) << (bit * 2u);
        out |= ((y >> bit) & 1u) << (bit * 2u + 1u);
    }
    return out;
}
}

std::vector<uint8_t> ctr_rgba8_tiled_abgr(const std::vector<uint8_t>& linear_rgba,
                                          uint16_t width,
                                          uint16_t height) {
    if (width == 0 || height == 0 || (width & 7u) || (height & 7u))
        throw std::runtime_error("CTR texture dimensions must be non-zero multiples of 8");
    const size_t expected = size_t(width) * size_t(height) * 4u;
    if (linear_rgba.size() != expected)
        throw std::runtime_error("CTR texture source RGBA byte count mismatch");

    std::vector<uint8_t> out(expected, 0);
    const uint32_t tiles_x = uint32_t(width) / 8u;
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const uint32_t tile_x = x >> 3u;
            const uint32_t tile_y = y >> 3u;
            const uint32_t in_tile = morton3(x & 7u, y & 7u);
            const size_t dst_px = (size_t(tile_y) * tiles_x + tile_x) * 64u + in_tile;
            const size_t src = (size_t(y) * width + x) * 4u;
            const size_t dst = dst_px * 4u;
            // tex3ds GPU_RGBA8 encoder emits A, B, G, R bytes.
            out[dst + 0] = linear_rgba[src + 3];
            out[dst + 1] = linear_rgba[src + 2];
            out[dst + 2] = linear_rgba[src + 1];
            out[dst + 3] = linear_rgba[src + 0];
        }
    }
    return out;
}
}

#pragma once
#include "legends3ds/frontend_title.h"
#include "legends3ds/material_cache.h"
#include <array>
#include <cstdint>
#include <vector>

// CPU rasterizer for the GP0 subset MessageWindows emits (glyph POLY_FT4 on
// the font page, semi-transparent POLY_F4, POLY_G4 bevels, E1 draw modes).
// Used for the in-game Options page on the 3DS bottom screen (port
// presentation: the original draws the sub-screen on the TV at the same
// 320x240 pixel positions). Draw order follows the PS1 OT: buckets from the
// highest down, each bucket in reverse AddPrim order.
namespace l3 {

struct MessageFont {
    struct Page {
        uint16_t tpage = 0;
        std::vector<uint8_t> indices;                  // 256x256, one index per pixel
        std::vector<std::pair<uint16_t, std::array<uint16_t, 16>>> palettes;   // CLUT -> PS1 colours
    };
    std::vector<Page> pages;
    bool ready() const { return !pages.empty(); }
    // Adds every resolved 4-bit entry of a cooked material cache (one index
    // page kept per TPAGE, one palette per CLUT).
    void add(const MaterialCache& cache);
};
// Font from a cooked materials/font.l3mc (entries on the font page).
MessageFont message_font_from_cache(const MaterialCache& cache);

// Draws into rgb (320x240, row-major, 3 bytes per pixel, R G B), which the
// caller pre-fills with the backdrop. Returns the number of primitives drawn.
size_t rasterize_message_windows(const std::vector<FrontendPacket>& packets, const MessageFont& font, uint8_t* rgb);

} // namespace l3

// Message font material cache from the owner's FONT.BIN (USA).
// FONT.BIN: one 0x800 header (type 3, "..\bg\char\font.dat", image at VRAM
// x 960, y 256), then 0x8000 bytes = 16 tiles of 32x32 halfwords, two tiles
// across (row-major), covering VRAM x 960..1023, y 256..511. The 16 message
// CLUTs (0x80097F30: 0x7F3C + ..., x 960 + 16 * (c & 3), y 508 + (c >> 2))
// are the page's own last four rows; the oracle's VRAM at (960,508) matched
// this layout word for word (2026-11-14). Material word per CLUT:
// GetTPage(0,0,960,256) | clut << 16, the page the message SPRTs use
// (0x80054240). Only colours 0..3 (row 508) are cooked: the OPTION menu and
// its help use 0 and 3, and each entry is a 256x256 page. Output holds owner-derived data: keep it out of the repo.
#include "legends3ds/frontend_title.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) { std::cerr << "usage: l3cook_font_materials FONT.BIN OUT.l3mc\n"; return 2; }
        const auto file = l3::read_file(argv[1]);
        if (file.size() < 0x8800 || file[0] != 3 || file[12] != 0xC0 || file[13] != 0x03 || file[16] != 0x00 || file[17] != 0x01)
            throw std::runtime_error("FONT.BIN header is not the type-3 page at (960,256)");
        // Detile into VRAM words vram[y][x], x 0..63, y 0..255.
        std::array<std::array<uint16_t, 64>, 256> vram{};
        for (unsigned tile = 0; tile < 16; ++tile)
            for (unsigned row = 0; row < 32; ++row)
                for (unsigned col = 0; col < 32; ++col) {
                    const size_t off = 0x800 + tile * 0x800u + (row * 32u + col) * 2u;
                    vram[(tile / 2) * 32 + row][(tile % 2) * 32 + col] = uint16_t(file[off] | (file[off + 1] << 8));
                }
        l3::MaterialCache cache;
        const uint16_t tpage = l3::psx_get_tpage(0, 0, 960, 256);
        for (unsigned c = 0; c < 4; ++c) {
            const unsigned cx = 960 + 16 * (c & 3), cy = 508 + (c >> 2);
            l3::MaterialCacheEntry e;
            e.material_word = l3::psx_material_word(l3::psx_get_clut(cx, cy), tpage);
            e.bpp = 4; e.abr = 0; e.palette_entries = 16; e.source_width = 256; e.source_height = 256; e.resolved = true;
            for (unsigned i = 0; i < 16; ++i) e.palette_ps1.push_back(vram[cy - 256][(cx - 960) + i]);
            e.indices.resize(256u * 256u);
            for (unsigned v = 0; v < 256; ++v)
                for (unsigned u = 0; u < 256; ++u)
                    e.indices[v * 256u + u] = uint8_t((vram[v][u / 4] >> ((u % 4) * 4)) & 15u);
            cache.entries.push_back(std::move(e));
        }
        const auto cooked = l3::cook_l3material_cache(cache);
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), std::streamsize(cooked.size()));
        if (!out) throw std::runtime_error("failed writing font material cache");
        std::cout << "font materials: CLUTs 0..3, page (960,256)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3cook_font_materials: " << error.what() << "\n";
        return 1;
    }
}

// Bottom-screen HUD art: the owner's GAUGE01u.TIM (..\BG\CHAR\GAUGE01u.TIM in
// DEMO_DAT.BIN), unpacked to one byte per pixel plus its palette rows.
// ROCK_NEO's life gauge sprite table (0x8008D37C.., drawn by FUN_8005C9BC)
// samples this page as tpage 0x000D (x=832) with clut 0x3E34 = (832,248),
// the TIM's palette row 0. The life cells use clut 0x3E75 = (848,249), which
// lies outside the TIM's 16x8 CLUT block (CANDIDATE: row 0 is used).
// Output (.l3ui, little endian): "L3UI", u16 version=1, u16 width, u16 height,
// u16 palette_entries, u16 palette_rows, u16 reserved,
// u16 palette[entries*rows] (PS1 BGR555, bit 15 = STP), width*height indices.
#include "legends3ds/material_resolver.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void put16(std::vector<uint8_t>& o, uint16_t v) { o.push_back(uint8_t(v)); o.push_back(uint8_t(v >> 8)); }
}
int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: l3cook_bottom_gauge DEMO_DAT.BIN OUT.l3ui\n";
            return 2;
        }
        const l3::Type1Texture* gauge = nullptr;
        const auto resources = l3::scan_mml_indexed_resources(l3::read_file(argv[1]));
        for (const auto& r : resources)
            if (r.kind == l3::IndexedResourceKind::TextureType1 && r.path.find("GAUGE01u.TIM") != std::string::npos &&
                r.texture.image_x_words == 832 && r.texture.image_y == 0) { gauge = &r.texture; break; }
        if (!gauge) throw std::runtime_error("GAUGE01u.TIM at VRAM (832,0) not found");
        if (gauge->bpp != 4 || gauge->width_pixels != 256 || gauge->height != 256)
            throw std::runtime_error("unexpected GAUGE01u.TIM shape");
        std::vector<uint8_t> out{'L', '3', 'U', 'I'};
        put16(out, 1);
        put16(out, gauge->width_pixels); put16(out, gauge->height);
        put16(out, gauge->palette_entries); put16(out, gauge->palette_rows); put16(out, 0);
        for (uint16_t c : gauge->palette_ps1) put16(out, c);
        // 4bpp VRAM words: the low nibble is the left pixel.
        for (uint8_t b : gauge->packed_indices) { out.push_back(b & 15u); out.push_back(b >> 4); }
        std::ofstream file(argv[2], std::ios::binary);
        file.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
        if (!file) throw std::runtime_error("failed writing gauge art");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3cook_bottom_gauge: " << error.what() << "\n";
        return 1;
    }
}

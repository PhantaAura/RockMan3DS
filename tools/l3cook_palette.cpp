#include "legends3ds/ps1_texture.h"
#include "legends3ds/st00_source.h"
#include "legends3ds/endian_io.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static std::string header_path(const std::vector<uint8_t>& b, size_t off) {
    if (off + 0x100 > b.size()) return {};
    size_t p = off + 0x40, e = p;
    while (e < off + 0x100 && b[e]) ++e;
    if (e == off + 0x100) return {};
    return std::string(reinterpret_cast<const char*>(b.data() + p), e - p);
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) {
            std::cerr << "usage: l3cook_palette CONTAINER.BIN INTERNAL_PATH_OR_SUFFIX OUT.l3pal\n";
            return 2;
        }
        const auto b = l3::read_file(argv[1]);
        const std::string wanted = argv[2];
        size_t found = size_t(-1); std::string path; uint32_t type = 0;
        for (size_t off = 0; off + 0x800 <= b.size(); off += 0x800) {
            const auto p = header_path(b, off);
            if (p.empty()) continue;
            const bool exact = p == wanted;
            const bool suffix = p.size() >= wanted.size() && p.compare(p.size() - wanted.size(), wanted.size(), wanted) == 0;
            if (!(exact || suffix)) continue;
            const uint32_t t = l3::rd_u32(b.data() + off);
            if (!(t == 4u || t == 9u)) throw std::runtime_error("matched resource is not a type-4/type-9 palette: " + p);
            if (found != size_t(-1)) throw std::runtime_error("resource selector is ambiguous");
            found = off; path = p; type = t;
        }
        if (found == size_t(-1)) throw std::runtime_error("compiled palette resource not found");
        std::vector<uint8_t> out; uint8_t bpp = 0; uint16_t rows = 0, x = 0, y = 0;
        if (type == 4u) {
            const auto patch = l3::parse_mml_type4_clut(b, found);
            out = l3::cook_l3palette(patch); bpp = patch.bpp; rows = patch.palette_rows; x = patch.clut_x; y = patch.clut_y;
        } else {
            const auto patch = l3::parse_mml_type9_palette(b, found);
            out = l3::cook_l3palette(patch); bpp = patch.bpp; rows = patch.palette_rows; x = patch.clut_x; y = patch.clut_y;
        }
        std::ofstream f(argv[3], std::ios::binary);
        f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
        if (!f) throw std::runtime_error("failed writing output");
        std::cout << path << ": type=" << type << " " << unsigned(bpp) << "bpp palettes=" << rows
                  << " clut=(" << x << "," << y << ") -> " << out.size() << " bytes\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_palette: " << e.what() << "\n";
        return 1;
    }
}

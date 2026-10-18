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
            std::cerr << "usage: l3cook_texture CONTAINER.BIN INTERNAL_PATH_OR_SUFFIX OUT.l3tex\n";
            return 2;
        }
        const auto b = l3::read_file(argv[1]);
        const std::string wanted = argv[2];
        size_t found = size_t(-1);
        std::string found_path;
        for (size_t off = 0; off + 0x800 <= b.size(); off += 0x800) {
            const auto p = header_path(b, off);
            if (p.empty()) continue;
            const bool exact = p == wanted;
            const bool suffix = p.size() >= wanted.size() && p.compare(p.size() - wanted.size(), wanted.size(), wanted) == 0;
            if (!(exact || suffix)) continue;
            if (l3::rd_u32(b.data() + off) != 1) throw std::runtime_error("matched resource is not type 1: " + p);
            if (found != size_t(-1)) throw std::runtime_error("resource selector is ambiguous");
            found = off; found_path = p;
        }
        if (found == size_t(-1)) throw std::runtime_error("type-1 texture resource not found");
        const auto t = l3::parse_mml_type1_texture(b, found);
        const auto cooked = l3::cook_l3texture(t);
        std::ofstream f(argv[3], std::ios::binary);
        f.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!f) throw std::runtime_error("failed writing output");
        std::cout << found_path << ": " << t.width_pixels << "x" << t.height << " " << unsigned(t.bpp)
                  << "bpp palettes=" << t.palette_rows << " vram=(" << t.image_x_words << "," << t.image_y
                  << ") clut=(" << t.clut_x << "," << t.clut_y << ") -> " << cooked.size() << " bytes\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_texture: " << e.what() << "\n";
        return 1;
    }
}

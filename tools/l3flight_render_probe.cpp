#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
static std::vector<uint8_t> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + p.string());
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3flight_render_probe <legends3ds/tutorial runtime dir>\n";
            return 2;
        }
        const fs::path root = argv[1];
        const auto map = l3::load_l3map(read_file(root / "st00.l3map"), true);
        const auto render = l3::load_l3render(read_file(root / "st00.l3render"), true);
        uint64_t total_prims = 0;
        for (size_t ai = 0; ai < map.areas.size(); ++ai) {
            const auto& area = map.areas[ai];
            uint32_t positive = 0, prims = 0;
            for (uint32_t local = 0; local < area.cell_count; ++local) {
                const uint32_t ci = area.cell_first + local;
                if (ci >= map.cells.size()) throw std::runtime_error("area cell range escaped map");
                const int16_t packed = map.cells[ci];
                if (packed <= 0) continue;
                ++positive;
                const size_t ii = static_cast<uint16_t>(packed);
                if (ii >= map.idx.size()) throw std::runtime_error("positive cell IDX escaped map");
                const auto& idx = map.idx[ii];
                if (idx.descriptor_index == 0xFFFFu || idx.descriptor_index >= render.descriptors.size())
                    throw std::runtime_error("runtime descriptor index cannot be rendered");
                const uint32_t lx = local % area.width, lz = local / area.width;
                const int32_t gx = int32_t(area.start_x) + int32_t(lx);
                const int32_t gz = int32_t(area.start_z) + int32_t(lz);
                const l3::St00CellOrigin origin{(gx << 9) - 0x7F00, idx.y, (gz << 9) - 0x7F00};
                const auto expanded = l3::expand_st00_descriptor_lod(render, idx.descriptor_index,
                    l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin,
                    static_cast<uint16_t>(packed), 0);
                prims += static_cast<uint32_t>(expanded.size());
            }
            total_prims += prims;
            std::cout << "area=" << ai << " positive=" << positive << " mid_primitives=" << prims << "\n";
        }
        if (map.areas.size() != 5 || total_prims == 0) throw std::runtime_error("tutorial render bridge produced no usable geometry");
        std::cout << "flight_render_probe=PASS total_mid_primitives=" << total_prims
                  << " triangle_vertices=" << (total_prims * 6u) << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "flight_render_probe=FAIL " << e.what() << "\n";
        return 1;
    }
}

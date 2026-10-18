#include "legends3ds/map_initialization.h"
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if ((argc != 9 && argc != 10) || std::string(argv[8]) != "--stage0-startup") {
            std::cerr << "usage: l3cook_map_init MDT IDX STG HED map.l3map render.l3render OUT.l3init --stage0-startup [HD2]\n";
            return 2;
        }
        if (std::filesystem::exists(argv[7])) throw std::runtime_error("refusing to replace existing initialization");
        const auto mdt = l3::read_file(argv[1]), idx = l3::read_file(argv[2]);
        const auto stg = l3::read_file(argv[3]), hed = l3::read_file(argv[4]);
        const auto map_bytes = l3::read_file(argv[5]), render_bytes = l3::read_file(argv[6]);
        const auto source = l3::parse_st00(mdt, idx, stg, hed,
            argc == 10 ? l3::read_file(argv[9]) : std::vector<uint8_t>{});
        const auto map = l3::load_l3map(map_bytes);
        const auto render = l3::load_l3render(render_bytes);
        const auto init = l3::derive_map_initialization(source, hed, stg, map, render,
            l3::crc32(map_bytes.data(), map_bytes.size()), l3::crc32(render_bytes.data(), render_bytes.size()),
            l3::MapInitializationProfile::Stage0Startup);
        const auto bytes = l3::cook_map_initialization(init);
        // Exclusive creation also protects a file appearing after the first check.
        FILE* out = std::fopen(argv[7], "wbx");
        if (!out) throw std::runtime_error("exclusive initialization output creation failed");
        const bool written = std::fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size();
        const bool closed = std::fclose(out) == 0;
        if (!written || !closed) throw std::runtime_error("initialization output write failed");
        std::cout << "SOURCE_BACKED_PARTIAL stage0_startup identities=" << init.idx_count
                  << " writes=" << init.entries.size() << " bytes=" << bytes.size()
                  << " decoded_bytes=" << init.decoded_bytes << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_map_init: " << e.what() << "\n";
        return 1;
    }
}

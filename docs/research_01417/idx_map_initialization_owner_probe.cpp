#include "legends3ds/map_initialization.h"
#include "legends3ds/endian_io.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: owner probe MAP RENDER INIT (expected scalar flags on stdin)");
        const auto map_bytes = l3::read_file(argv[1]), render_bytes = l3::read_file(argv[2]);
        const auto init_bytes = l3::read_file(argv[3]);
        auto map = l3::load_l3map(map_bytes);
        const auto before = map;
        const auto render = l3::load_l3render(render_bytes);
        const auto init = l3::load_map_initialization(init_bytes);
        const auto result = l3::apply_map_initialization(map, render, init,
            l3::crc32(map_bytes.data(), map_bytes.size()), l3::crc32(render_bytes.data(), render_bytes.size()));
        std::vector<uint8_t> flags;
        for (size_t i = 0; i < map.idx.size(); ++i) {
            unsigned expected = 0;
            if (!(std::cin >> expected) || expected > 65535u ||
                static_cast<uint16_t>(map.idx[i].raw6) != expected)
                throw std::runtime_error("native/original scalar mismatch");
            const auto& a = before.idx[i]; const auto& b = map.idx[i];
            if (a.x != b.x || a.y != b.y || a.z != b.z || a.descriptor_index != b.descriptor_index)
                throw std::runtime_error("initializer changed other IDX fields");
            l3::wr_u16(flags, static_cast<uint16_t>(b.raw6));
        }
        unsigned extra = 0;
        if (std::cin >> extra) throw std::runtime_error("extra expected scalar field");
        std::cout << "{\"all_original_idx_flags_matched\":true,\"xyz_descriptor_preserved\":true,"
                  << "\"idx_records_compared\":" << map.idx.size() << ",\"write_count\":" << result.write_count
                  << ",\"decoded_bytes\":" << result.decoded_bytes << ",\"flags_crc32\":"
                  << l3::crc32(flags.data(), flags.size()) << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

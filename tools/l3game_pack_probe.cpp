#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/stage_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open: " + path.string());
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3game_pack_probe WORLD_DIR\n";
            return 2;
        }
        const fs::path root = fs::path(argv[1]);
        const auto catalog_bytes = read_file(root / "stage_catalog.l3sc");
        const auto catalog = l3::load_l3stage_catalog(catalog_bytes, true);

        uint64_t total_areas = 0;
        uint64_t total_cells = 0;
        uint64_t total_descriptors = 0;
        uint64_t total_contacts = 0;
        uint64_t supported_contact_families = 0;
        uint64_t unsupported_contact_families = 0;
        uint64_t total_render_records = 0;
        uint64_t total_render_streams = 0;
        uint64_t total_render_primitives = 0;
        size_t maps = 0;
        size_t renders = 0;

        for (const auto& entry : catalog.entries) {
            const fs::path package = root / "stages" / entry.package_string();
            const auto map_bytes = read_file(package / "map.l3map");
            if (map_bytes.size() != entry.map_bytes ||
                l3::crc32(map_bytes.data(), map_bytes.size()) != entry.map_crc32) {
                throw std::runtime_error("stage catalog map metadata mismatch: " + entry.package_string());
            }
            const auto map = l3::load_l3map(map_bytes, true);
            ++maps;
            total_areas += map.areas.size();
            total_cells += map.cells.size();
            total_descriptors += map.descriptors.size();
            total_contacts += map.contact_primitives.size();
            for (const auto& primitive : map.contact_primitives) {
                if (l3::runtime_contact_dispatch_family(primitive.code)) ++supported_contact_families;
                else ++unsupported_contact_families;
            }

            if ((entry.flags & l3::kL3StageCatalogFlagRender) != 0u) {
                const auto render_bytes = read_file(package / "render.l3render");
                if (render_bytes.size() != entry.render_bytes ||
                    l3::crc32(render_bytes.data(), render_bytes.size()) != entry.render_crc32) {
                    throw std::runtime_error("stage catalog render metadata mismatch: " + entry.package_string());
                }
                const auto render = l3::load_l3render(render_bytes, true);
                ++renders;
                total_render_records += render.records.size();
                total_render_streams += render.streams.size();
                for (const auto& stream : render.streams) total_render_primitives += stream.primitives.size();
            }
        }

        std::cout << "packages=" << catalog.entries.size()
                  << " maps=" << maps
                  << " renders=" << renders
                  << " areas=" << total_areas
                  << " cells=" << total_cells
                  << " descriptors=" << total_descriptors
                  << " contact_primitives=" << total_contacts
                  << " contact_dispatch_supported=" << supported_contact_families
                  << " contact_dispatch_unsupported=" << unsupported_contact_families
                  << " render_records=" << total_render_records
                  << " render_streams=" << total_render_streams
                  << " render_primitives=" << total_render_primitives << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}

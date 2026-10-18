#include "legends3ds/l3map.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/pbd_model.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/st00_source.h"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3flight_material_probe TUTORIAL_ROOT\n";
            return 2;
        }
        const fs::path root = argv[1];
        const auto map = l3::load_l3map(l3::read_file((root / "st00.l3map").string()), true);
        const auto render = l3::load_l3render(l3::read_file((root / "st00.l3render").string()), true);
        uint64_t total_prims = 0, resolved_prims = 0, unresolved_prims = 0;
        for (size_t area_index = 0; area_index < map.areas.size(); ++area_index) {
            char name[64];
            std::snprintf(name, sizeof(name), "area_%02zu.l3mc", area_index);
            const auto cache = l3::load_l3material_cache(l3::read_file((root / "materials" / name).string()), true);
            const auto& area = map.areas[area_index];
            uint64_t ap = 0, ar = 0, au = 0;
            for (uint32_t local = 0; local < area.cell_count; ++local) {
                const uint32_t ci = area.cell_first + local;
                if (ci >= map.cells.size()) throw std::runtime_error("cell escaped map");
                const int16_t packed = map.cells[ci];
                if (packed <= 0) continue;
                const size_t ii = static_cast<uint16_t>(packed);
                if (ii >= map.idx.size()) throw std::runtime_error("IDX escaped map");
                const auto& idx = map.idx[ii];
                if (idx.descriptor_index == 0xFFFFu || idx.descriptor_index >= render.descriptors.size()) continue;
                const uint32_t lx = local % area.width, lz = local / area.width;
                const int32_t gx = int32_t(area.start_x) + int32_t(lx);
                const int32_t gz = int32_t(area.start_z) + int32_t(lz);
                const l3::St00CellOrigin origin{(gx << 9) - 0x7F00, idx.y, (gz << 9) - 0x7F00};
                const auto prims = l3::expand_st00_descriptor_lod(render, idx.descriptor_index,
                    l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, static_cast<uint16_t>(packed), 0);
                for (const auto& p : prims) {
                    ++ap;
                    const auto* e = l3::find_material_cache_entry(cache, p.material_word);
                    if (!e) throw std::runtime_error("expanded terrain material missing from per-area cache");
                    if (e->resolved) ++ar; else ++au;
                }
            }
            total_prims += ap; resolved_prims += ar; unresolved_prims += au;
            std::cout << "area=" << area_index << " primitives=" << ap << " resolved=" << ar << " unresolved=" << au << "\n";
        }

        std::set<uint32_t> player_words;
        const char* models[] = {"body.l3model","head.l3model","right.l3model","left.l3model","feet.l3model"};
        for (const char* n : models) {
            const auto model = l3::load_l3model(l3::read_file((root / "player" / n).string()), true);
            for (const auto& part : model.parts) {
                for (const auto& f : part.triangles) player_words.insert((uint32_t(f.clut)<<16u)|f.tpage);
                for (const auto& f : part.quads) player_words.insert((uint32_t(f.clut)<<16u)|f.tpage);
            }
        }
        for (size_t area_index = 0; area_index < map.areas.size(); ++area_index) {
            char name[64]; std::snprintf(name,sizeof(name),"player_area_%02zu.l3mc",area_index);
            const auto cache=l3::load_l3material_cache(l3::read_file((root/"materials"/name).string()),true);
            for (uint32_t word : player_words) {
                const auto* e=l3::find_material_cache_entry(cache,word);
                if(!e||!e->resolved) throw std::runtime_error("player material unresolved in area cache");
            }
        }
        std::cout << "terrain_total=" << total_prims << " resolved=" << resolved_prims << " unresolved=" << unresolved_prims << "\n";
        std::cout << "player_materials=" << player_words.size() << " all_areas_resolved=YES\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3flight_material_probe: " << e.what() << "\n";
        return 1;
    }
}

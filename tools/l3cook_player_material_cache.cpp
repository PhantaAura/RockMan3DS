#include "legends3ds/material_cache.h"
#include "legends3ds/pbd_model.h"
#include "legends3ds/ps1_texture.h"
#include "legends3ds/st00_source.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void collect_model_words(const l3::PbdModel& model, std::set<uint32_t>& words) {
    for (const auto& part : model.parts) {
        for (const auto& f : part.triangles)
            words.insert((uint32_t(f.clut) << 16u) | uint32_t(f.tpage));
        for (const auto& f : part.quads)
            words.insert((uint32_t(f.clut) << 16u) | uint32_t(f.tpage));
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 9) {
            std::cerr << "usage: l3cook_player_material_cache BODY.l3model HEAD.l3model RIGHT.l3model LEFT.l3model FEET.l3model OUT.l3mc CONTAINER.BIN...\n";
            return 2;
        }
        std::set<uint32_t> words;
        for (int i = 1; i <= 5; ++i)
            collect_model_words(l3::load_l3model(l3::read_file(argv[i]), true), words);
        if (words.empty()) throw std::runtime_error("player models contain no material words");

        std::vector<l3::IndexedResource> resources;
        for (int i = 7; i < argc; ++i) {
            auto found = l3::scan_mml_indexed_resources(l3::read_file(argv[i]));
            resources.insert(resources.end(), std::make_move_iterator(found.begin()), std::make_move_iterator(found.end()));
        }
        const std::vector<uint32_t> ordered(words.begin(), words.end());
        const auto cache = l3::build_material_cache(ordered, resources);
        const auto cooked = l3::cook_l3material_cache(cache);
        std::ofstream out(argv[6], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!out) throw std::runtime_error("failed writing player material cache");

        uint32_t resolved = 0;
        for (const auto& e : cache.entries) {
            if (e.resolved) ++resolved;
            else std::cout << "unresolved=0x" << std::hex << std::setw(8) << std::setfill('0') << e.material_word << std::dec << "\n";
        }
        std::cout << "player_materials=" << cache.entries.size()
                  << " resolved=" << resolved
                  << " unresolved=" << (cache.entries.size() - resolved)
                  << " resources=" << resources.size()
                  << " bytes=" << cooked.size() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_player_material_cache: " << e.what() << "\n";
        return 1;
    }
}

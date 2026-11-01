// Material cache for an explicit list of CLUT/tpage words (one hex word per
// line, e.g. from cook_intro_bg.py) plus, with --ebd, the words of every
// class-0x20 and class-0x40 mesh in a cooked EBD, composed from owner containers in the
// stage loader's upload order (later containers overwrite earlier VRAM).
// Every word must resolve.
#include "legends3ds/actor_resource.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc < 4) { std::cerr << "usage: l3cook_word_materials OUT.l3mc WORDS.txt [--ebd FILE.l3ebd] CONTAINER.BIN...\n"; return 2; }
        std::vector<uint32_t> words;
        int first_container = 3;
        std::set<uint32_t> mesh_words;
        if (argc > 5 && std::string(argv[3]) == "--ebd") {
            const auto ebd = l3::load_l3ebd_directory(l3::read_file(argv[4]), true);
            for (size_t row = 0; row < ebd.entries.size(); ++row) {
                const uint8_t cls = ebd.entries[row].object_class;
                if (cls != 0x20 && cls != 0x40) continue;
                const auto mesh = cls == 0x40 ? l3::decode_ebd_scene_actor40_mesh(ebd, row) : l3::decode_ebd_actor_mesh(ebd, row);
                for (const auto& part : mesh.model.parts) {
                    for (const auto& t : part.triangles) mesh_words.insert((uint32_t(t.clut) << 16) | t.tpage);
                    for (const auto& q : part.quads) mesh_words.insert((uint32_t(q.clut) << 16) | q.tpage);
                }
            }
            first_container = 5;
        }
        std::ifstream list(argv[2]);
        std::string line;
        while (std::getline(list, line)) if (!line.empty()) words.push_back(uint32_t(std::stoul(line, nullptr, 16)));
        if (!list.eof() || words.empty()) throw std::runtime_error("failed reading word list");
        for (const uint32_t w : mesh_words) if (std::find(words.begin(), words.end(), w) == words.end()) words.push_back(w);
        std::vector<l3::IndexedResource> resources;
        for (int i = first_container; i < argc; ++i) {
            auto more = l3::scan_mml_indexed_resources(l3::read_file(argv[i]));
            resources.insert(resources.end(), more.begin(), more.end());
        }
        std::vector<l3::MaterialVramDiagnostics> diagnostics;
        const auto cache = l3::build_material_cache(words, resources, &diagnostics);
        for (const auto& d : diagnostics)
            std::cout << std::hex << "material 0x" << d.material_word << std::dec << ": image "
                      << d.image_words_written << "/" << d.image_words_total << " palette "
                      << d.palette_words_written << "/" << d.palette_words_total << "\n";
        for (const auto& e : cache.entries)
            if (!e.resolved) throw std::runtime_error("unresolved listed material");
        const auto cooked = l3::cook_l3material_cache(cache);
        std::ofstream out(argv[1], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!out) throw std::runtime_error("failed writing material cache");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3cook_word_materials: " << error.what() << "\n";
        return 1;
    }
}

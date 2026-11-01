// Pre-title narration material cache from the owner's ST01.BIN: the four
// text-crawl pages OPENG0..3 (tpage 21..24, CLUT 0x7C00) the ST01 strips use,
// plus the background-layer words cook_intro_bg.py lists (one hex word per
// line) and the CLUT/tpage words of every mesh in the cooked ST01_00 EBD
// (the airship) (docs/research_01516/INTRO_EVENT71.md). Resources are composed in container
// order, as the stage loader uploads them.
#include "legends3ds/actor_resource.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/source_intro_crawl.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc < 3 || argc > 5) { std::cerr << "usage: l3cook_intro_materials ST01.BIN OUT.l3mc [BG_WORDS.txt [ST01_00.l3ebd]]\n"; return 2; }
        const auto resources = l3::scan_mml_indexed_resources(l3::read_file(argv[1]));
        std::vector<uint32_t> words(l3::kSourceIntroCrawlMaterialWords.begin(), l3::kSourceIntroCrawlMaterialWords.end());
        if (argc >= 4) {
            std::ifstream list(argv[3]);
            std::string line;
            while (std::getline(list, line)) if (!line.empty()) words.push_back(uint32_t(std::stoul(line, nullptr, 16)));
            if (!list.eof()) throw std::runtime_error("failed reading background word list");
        }
        if (argc == 5) {
            const auto ebd = l3::load_l3ebd_directory(l3::read_file(argv[4]), true);
            std::set<uint32_t> mesh_words;
            for (size_t row = 0; row < ebd.entries.size(); ++row) {
                const auto mesh = l3::decode_ebd_actor_mesh(ebd, row);
                for (const auto& part : mesh.model.parts) {
                    for (const auto& t : part.triangles) mesh_words.insert((uint32_t(t.clut) << 16) | t.tpage);
                    for (const auto& q : part.quads) mesh_words.insert((uint32_t(q.clut) << 16) | q.tpage);
                }
            }
            words.insert(words.end(), mesh_words.begin(), mesh_words.end());
        }
        std::vector<l3::MaterialVramDiagnostics> diagnostics;
        const auto cache = l3::build_material_cache(words, resources, &diagnostics);
        for (const auto& d : diagnostics)
            std::cout << std::hex << "material 0x" << d.material_word << std::dec << ": image "
                      << d.image_words_written << "/" << d.image_words_total << " palette "
                      << d.palette_words_written << "/" << d.palette_words_total << "\n";
        for (const auto& e : cache.entries)
            if (!e.resolved) throw std::runtime_error("unresolved intro material");
        const auto cooked = l3::cook_l3material_cache(cache);
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!out) throw std::runtime_error("failed writing intro material cache");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3cook_intro_materials: " << error.what() << "\n";
        return 1;
    }
}

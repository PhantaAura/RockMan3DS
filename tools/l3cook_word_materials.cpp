// Material cache for an explicit list of CLUT/tpage words (one hex word per
// line, e.g. from cook_intro_bg.py), composed from owner containers in the
// stage loader's upload order (later containers overwrite earlier VRAM).
// Every listed word must resolve.
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc < 4) { std::cerr << "usage: l3cook_word_materials OUT.l3mc WORDS.txt CONTAINER.BIN...\n"; return 2; }
        std::vector<uint32_t> words;
        std::ifstream list(argv[2]);
        std::string line;
        while (std::getline(list, line)) if (!line.empty()) words.push_back(uint32_t(std::stoul(line, nullptr, 16)));
        if (!list.eof() || words.empty()) throw std::runtime_error("failed reading word list");
        std::vector<l3::IndexedResource> resources;
        for (int i = 3; i < argc; ++i) {
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

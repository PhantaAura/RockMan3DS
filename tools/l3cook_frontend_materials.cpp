// Title-screen material cache from the owner's DEMO_DAT.BIN (front end,
// docs/research_01516/FRONTEND_TITLE.md). The four material words are the
// ones the ROCK_NEO front-end packets use:
//   logo 0x80013FB8        GetTPage(1,0,320,256) | GetClut(0,0x1F0) << 16
//   PRESS START 0x80014394 GetTPage(0,0,320,256) | GetClut(0,0x1F1) << 16
//   menu rows 0x8001456C   GetTPage(0,0,320,256) | GetClut(16 or 32,0x1F1) << 16
// Resources are composed in container order (NTITLEU, then NTITLE5U over its
// rows 160+), as the stage loader uploads them.
#include "legends3ds/frontend_title.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: l3cook_frontend_materials DEMO_DAT.BIN OUT.l3mc\n";
            return 2;
        }
        const auto resources = l3::scan_mml_indexed_resources(l3::read_file(argv[1]));
        const std::vector<uint32_t> words(l3::kFrontendMaterialWords.begin(), l3::kFrontendMaterialWords.end());
        std::vector<l3::MaterialVramDiagnostics> diagnostics;
        const auto cache = l3::build_material_cache(words, resources, &diagnostics);
        for (const auto& d : diagnostics)
            std::cout << std::hex << "material 0x" << d.material_word << std::dec << ": image "
                      << d.image_words_written << "/" << d.image_words_total << " palette "
                      << d.palette_words_written << "/" << d.palette_words_total << "\n";
        for (const auto& e : cache.entries)
            if (!e.resolved) throw std::runtime_error("unresolved front-end material");
        const auto cooked = l3::cook_l3material_cache(cache);
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!out) throw std::runtime_error("failed writing front-end material cache");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3cook_frontend_materials: " << error.what() << "\n";
        return 1;
    }
}

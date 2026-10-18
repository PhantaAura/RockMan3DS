#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <iomanip>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: l3inspect_material_cache FILE.l3mc\n"; return 2; }
        const auto bytes = l3::read_file(argv[1]);
        const auto h = l3::inspect_l3material_cache(bytes, true);
        const auto c = l3::load_l3material_cache(bytes, true);
        std::cout << "L3MC v" << h.version << " materials=" << h.material_count
                  << " resolved=" << h.resolved_count << " unresolved=" << h.unresolved_count
                  << " page=" << h.texture_width << "x" << h.texture_height << " bytes=" << h.file_bytes << "\n";
        for (const auto& e : c.entries) if (!e.resolved)
            std::cout << "unresolved=0x" << std::hex << std::setw(8) << std::setfill('0') << e.material_word << std::dec << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3inspect_material_cache: " << e.what() << "\n";
        return 1;
    }
}

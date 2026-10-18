#include "legends3ds/material_resolver.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/st00_source.h"
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 6) {
        std::cerr << "usage: l3scan_materials ST00_MAP.MDT ST00_MAP.IDX ST00.STG ST00.HED CONTAINER.BIN...\n";
        return 2;
    }
    try {
        auto mdt = l3::read_file(argv[1]), idx = l3::read_file(argv[2]);
        auto stg = l3::read_file(argv[3]), hed = l3::read_file(argv[4]);
        const auto stage = l3::parse_st00(mdt, idx, stg, hed);
        const auto render = l3::parse_st00_render(stage, stg);
        std::vector<l3::IndexedResource> resources;
        uint32_t t1 = 0, t4 = 0, t9 = 0;
        for (int a = 5; a < argc; ++a) {
            const auto found = l3::scan_mml_indexed_resources(l3::read_file(argv[a]));
            for (auto r : found) {
                if (r.kind == l3::IndexedResourceKind::TextureType1) ++t1;
                else if (r.kind == l3::IndexedResourceKind::ClutType4) ++t4;
                else ++t9;
                resources.push_back(std::move(r));
            }
        }
        std::map<uint32_t, uint32_t> refs;
        for (const auto& rec : render.records) for (uint32_t w : rec.materials) ++refs[w];
        uint32_t complete_refs = 0, incomplete_refs = 0, complete_unique = 0, incomplete_unique = 0;
        for (const auto& kv : refs) {
            const auto rr = l3::resolve_material_word(kv.first, resources);
            if (rr.complete()) {
                complete_refs += kv.second; ++complete_unique;
            } else {
                incomplete_refs += kv.second; ++incomplete_unique;
                std::cout << "unresolved=0x" << std::hex << std::setw(8) << std::setfill('0') << kv.first << std::dec
                          << " refs=" << kv.second << " image=" << (rr.image_resource >= 0)
                          << " palette=" << (rr.palette_resource >= 0) << " bpp=" << unsigned(rr.bpp) << "\n";
            }
        }
        std::cout << "type1=" << t1 << " type4=" << t4 << " type9=" << t9
                  << " material_refs=" << (complete_refs + incomplete_refs)
                  << " complete_refs=" << complete_refs << " incomplete_refs=" << incomplete_refs
                  << " complete_unique=" << complete_unique << " incomplete_unique=" << incomplete_unique << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}

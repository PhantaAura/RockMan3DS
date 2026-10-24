#include "legends3ds/material_cache.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/st00_source.h"
#include "legends3ds/render/negative_materials.h"
#include "legends3ds/render/terrain_clut_ramp.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc < 8) {
            std::cerr << "usage: l3cook_material_cache ST00_MAP.MDT ST00_MAP.IDX ST00.STG ST00.HED AREA_INDEX OUT.l3mc CONTAINER.BIN... [--negative-materials bindings.l3nm]\n";
            return 2;
        }
        const auto mdt = l3::read_file(argv[1]);
        const auto idx = l3::read_file(argv[2]);
        const auto stg = l3::read_file(argv[3]);
        const auto hed = l3::read_file(argv[4]);
        const size_t area_index = static_cast<size_t>(std::stoul(argv[5], nullptr, 0));
        const auto stage = l3::parse_st00(mdt, idx, stg, hed);
        const auto render = l3::parse_st00_render(stage, stg);
        if (area_index >= stage.areas.size()) throw std::runtime_error("area index out of range");

        std::set<uint32_t> words;
        const auto& area = stage.areas[area_index];
        for (const int16_t packed : area.cells) {
            if (packed <= 0) continue;
            const size_t ii = static_cast<uint16_t>(packed);
            if (ii >= stage.idx.size()) throw std::runtime_error("area cell IDX index out of range");
            const uint32_t descriptor_ptr = stage.idx[ii].descriptor_ptr;
            size_t di = 0;
            for (; di < stage.descriptors.size(); ++di) if (stage.descriptors[di].source_ptr == descriptor_ptr) break;
            if (di >= stage.descriptors.size() || di >= render.descriptors.size()) throw std::runtime_error("area descriptor pointer not present in HED/render table");
            for (const uint16_t ri : render.descriptors[di].record_indices) {
                if (ri == 0xFFFFu) continue;
                if (ri >= render.records.size()) throw std::runtime_error("render record index out of range");
                words.insert(render.records[ri].materials.begin(), render.records[ri].materials.end());
            }
        }

        std::vector<l3::IndexedResource> resources;
        l3::NegativeMaterialTables negative_materials;
        bool negative_option_present = false;
        std::string ramp_out;
        for (int i = 7; i < argc; ++i) {
            if (std::string(argv[i]) == "--ramp-out") {
                if (!ramp_out.empty() || i + 1 >= argc) throw std::runtime_error("ramp-out option duplicated or missing path");
                ramp_out = argv[++i];
                continue;
            }
            if (std::string(argv[i]) == "--negative-materials") {
                if (negative_option_present || i + 1 >= argc)
                    throw std::runtime_error("negative-materials option duplicated or missing path");
                negative_option_present = true;
                negative_materials = l3::load_l3negative_materials(l3::read_file(argv[++i]), true);
                if (negative_materials.size() != stage.areas.size())
                    throw std::runtime_error("negative material table/map area count mismatch");
                continue;
            }
            auto found = l3::scan_mml_indexed_resources(l3::read_file(argv[i]));
            resources.insert(resources.end(), std::make_move_iterator(found.begin()), std::make_move_iterator(found.end()));
        }
        uint32_t known_negative_cells = 0, unknown_negative_cells = 0;
        for (const int16_t packed : area.cells) {
            if (packed >= 0) continue;
            const auto* binding = l3::find_negative_material_binding(negative_materials, area_index,
                                                                   static_cast<uint16_t>(packed));
            if (binding) { words.insert(binding->material_word); ++known_negative_cells; }
            else ++unknown_negative_cells;
        }
        const std::vector<uint32_t> ordered_words(words.begin(), words.end());
        std::vector<l3::MaterialVramDiagnostics> diagnostics;
        const auto cache = l3::build_material_cache(ordered_words, resources, &diagnostics);
        const auto cooked = l3::cook_l3material_cache(cache);
        std::ofstream out(argv[6], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!out) throw std::runtime_error("failed writing material cache");

        if (!ramp_out.empty()) {
            // 0x80025900 depth ramp: the palettes at CLUT + k (k = 1..7) for every
            // resolved word, fitted to per-channel scales (L3CR).
            std::vector<uint32_t> variant_words;
            for (const auto& e : cache.entries)
                if (e.resolved) for (uint32_t k = 1; k < 8; ++k) variant_words.push_back(e.material_word + (k << 16));
            const auto variants = l3::build_material_cache(variant_words, resources);
            std::vector<l3::TerrainClutRampEntry> ramp;
            for (const auto& e : cache.entries) {
                if (!e.resolved) continue;
                std::array<std::vector<uint16_t>,8> pals{};
                for (uint32_t k = 1; k < 8; ++k) {
                    const auto* v = l3::find_material_cache_entry(variants, e.material_word + (k << 16));
                    if (v && v->resolved) pals[k] = v->palette_ps1;
                }
                ramp.push_back(l3::fit_terrain_clut_ramp(e.material_word, e.palette_ps1, pals));
            }
            const auto ramp_bytes = l3::cook_l3clut_ramp(ramp);
            std::ofstream rout(ramp_out, std::ios::binary);
            rout.write(reinterpret_cast<const char*>(ramp_bytes.data()), static_cast<std::streamsize>(ramp_bytes.size()));
            if (!rout) throw std::runtime_error("failed writing ramp sidecar");
            std::cout << "ramp_entries=" << ramp.size() << " bytes=" << ramp_bytes.size() << "\n";
        }
        uint32_t resolved = 0;
        for (const auto& e : cache.entries) {
            if (e.resolved) ++resolved;
            else std::cout << "unresolved=0x" << std::hex << std::setw(8) << std::setfill('0') << e.material_word << std::dec << "\n";
        }
        std::cout << "area=" << area_index << " materials=" << cache.entries.size()
                  << " resolved=" << resolved << " unresolved=" << (cache.entries.size() - resolved)
                  << " resources=" << resources.size() << " bytes=" << cooked.size()
                  << " known_negative_cells=" << known_negative_cells
                  << " unknown_negative_cells=" << unknown_negative_cells << "\n";
        uint64_t image_written = 0, image_total = 0, palette_written = 0, palette_total = 0;
        for (const auto& d : diagnostics) {
            image_written += d.image_words_written; image_total += d.image_words_total;
            palette_written += d.palette_words_written; palette_total += d.palette_words_total;
        }
        std::cout << "VRAM_COMPOSITION image_words_written=" << image_written
                  << " image_words_total=" << image_total << " unwritten_image_words=" << image_total - image_written
                  << " palette_words_written=" << palette_written << " palette_words_total=" << palette_total
                  << " unwritten_pixels=ZERO_UNKNOWN\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_material_cache: " << e.what() << "\n";
        return 1;
    }
}

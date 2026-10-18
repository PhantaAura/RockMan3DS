#include "legends3ds/ebd_directory.h"
#include "legends3ds/st00_flow.h"
#include "legends3ds/stage_runtime.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint8_t> read_file(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + p);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: l3runtime_probe R3_ST00.BIN ST00_00.EBD ST00_01.EBD ST00_03.EBD ST00_04.EBD\n";
        return 2;
    }
    try {
        const auto overlay = read_file(argv[1]);
        const auto flow = l3::parse_st00_flow_us(overlay);
        std::array<l3::EbdDirectory, 4> dirs{
            l3::parse_ebd_directory(read_file(argv[2]), 0x8016C000u),
            l3::parse_ebd_directory(read_file(argv[3]), 0x8016C000u),
            l3::parse_ebd_directory(read_file(argv[4]), 0x8016C000u),
            l3::parse_ebd_directory(read_file(argv[5]), 0x8016C000u),
        };
        const std::array<size_t, 5> area_dir{{0,1,1,2,3}};
        size_t total = 0, unique = 0, ambiguous = 0, none = 0;
        size_t raw_variant_unique = 0, raw_variant_none = 0, raw_variant_multi = 0;
        size_t ambiguous_narrowed_by_raw3 = 0, ambiguous_still_unresolved = 0;
        std::map<unsigned, size_t> family_hist;
        std::map<unsigned, size_t> peak_pool_use;
        size_t seeded_generic_work_objects = 0;
        size_t seeded_generic_work_bytes = 0;
        for (size_t area = 0; area < flow.areas.size(); ++area) {
            const auto objects = l3::instantiate_stage_runtime_objects(flow.areas[area].objects);
            const auto allocations = l3::assign_fresh_stage_pool_slots(objects);
            std::map<unsigned, size_t> area_pool_use;
            for (const auto& a : allocations) {
                if (!a.pool_slot) continue;
                const unsigned c = objects[a.source_index].source_class;
                area_pool_use[c] = std::max(area_pool_use[c], *a.pool_slot + 1);
                peak_pool_use[c] = std::max(peak_pool_use[c], *a.pool_slot + 1);
                const auto work = l3::seed_fresh_stage_runtime_work(objects[a.source_index], a);
                if (!work) throw std::runtime_error("generic allocation failed to seed work mirror");
                ++seeded_generic_work_objects;
                seeded_generic_work_bytes += work->bytes.size();
            }
            const auto& dir = dirs[area_dir[area]];
            size_t au = 0, aa = 0, an = 0;
            size_t avu = 0, avn = 0, avm = 0;
            for (const auto& o : objects) {
                ++total;
                ++family_hist[o.source_class];
                const auto candidates = l3::find_ebd_entries(dir, o.source_class, o.subtype);
                if (candidates.empty()) { ++none; ++an; }
                else if (candidates.size() == 1) { ++unique; ++au; }
                else { ++ambiguous; ++aa; }
                const auto raw_variant = l3::find_ebd_entries(dir, o.source_class, o.subtype, o.raw[3]);
                if (raw_variant.empty()) { ++raw_variant_none; ++avn; }
                else if (raw_variant.size() == 1) { ++raw_variant_unique; ++avu; }
                else { ++raw_variant_multi; ++avm; }
                if (candidates.size() > 1) {
                    if (raw_variant.size() == 1) ++ambiguous_narrowed_by_raw3;
                    else {
                        ++ambiguous_still_unresolved;
                        std::cout << "  unresolved area=" << area << " object=" << o.source_index
                                  << " class=0x" << std::hex << unsigned(o.source_class)
                                  << " subtype=0x" << unsigned(o.subtype)
                                  << " raw3=0x" << unsigned(o.raw[3]) << std::dec
                                  << " variants=";
                        for (const size_t ci : candidates)
                            std::cout << unsigned(dir.entries[ci].variant) << ",";
                        std::cout << "\n";
                    }
                }
            }
            std::cout << "area " << area << ": objects=" << objects.size()
                      << " EBD-unique=" << au << " EBD-ambiguous=" << aa
                      << " EBD-common/no-local=" << an
                      << " raw+3-unique=" << avu << " raw+3-none=" << avn << " raw+3-multi=" << avm
                      << " aux=" << flow.areas[area].aux.records.size();
            if (!area_pool_use.empty()) {
                std::cout << " pools=";
                for (const auto& kv : area_pool_use)
                    std::cout << "0x" << std::hex << kv.first << std::dec << ":" << kv.second << ",";
            }
            std::cout << "\n";
        }
        std::cout << "total objects=" << total << " unique=" << unique
                  << " ambiguous=" << ambiguous << " common/no-local=" << none << "\n";
        std::cout << "raw stage +3 variant candidates: unique=" << raw_variant_unique
                  << " none=" << raw_variant_none << " multi=" << raw_variant_multi << "\n";
        std::cout << "ambiguous class/subtype links narrowed to singleton by raw +3=" << ambiguous_narrowed_by_raw3
                  << " still unresolved=" << ambiguous_still_unresolved << "\n";
        std::cout << "EBD directory entries: area0=" << dirs[0].entries.size()
                  << " area1/2=" << dirs[1].entries.size()
                  << " area3=" << dirs[2].entries.size()
                  << " area4=" << dirs[3].entries.size() << "\n";
        std::cout << "class histogram:";
        for (const auto& kv : family_hist) std::cout << " " << std::hex << kv.first << std::dec << "=" << kv.second;
        std::cout << "\n";
        std::cout << "seeded generic work mirrors=" << seeded_generic_work_objects
                  << " aggregate source-stride bytes=" << seeded_generic_work_bytes << "\n";
        std::cout << "peak generic pool use:";
        for (const auto& kv : peak_pool_use) {
            const auto spec = l3::stage_object_pool_spec_for_class(static_cast<uint8_t>(kv.first));
            std::cout << " 0x" << std::hex << kv.first << std::dec << "=" << kv.second;
            if (spec) std::cout << "/" << spec->capacity;
        }
        std::cout << "\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

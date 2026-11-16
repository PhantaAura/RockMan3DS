#include "legends3ds/st00_flow.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint8_t> read_all(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static void write_all(const std::string& p, const std::vector<uint8_t>& b) {
    std::ofstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open output");
    f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
    if (!f) throw std::runtime_error("write failed");
}
int main(int argc, char** argv) {
    try {
        // usage: l3cook_st00_flow R3_STxx.BIN out.l3flow [--stage N]  (default stage 0)
        if (argc != 3 && !(argc == 5 && std::string(argv[3]) == "--stage")) {
            std::cerr << "usage: l3cook_st00_flow R3_STxx.BIN out.l3flow [--stage N]\n"; return 2;
        }
        const unsigned stage = argc == 5 ? static_cast<unsigned>(std::stoul(argv[4], nullptr, 0)) : 0u;
        const auto* table = stage <= 0xFF ? l3::stage_flow_addresses_us(static_cast<uint8_t>(stage)) : nullptr;
        if (!table) { std::cerr << "error: stage " << stage << " area setup tables are not traced\n"; return 1; }
        const auto raw = read_all(argv[1]);
        const auto flow = l3::parse_stage_flow_us(raw, *table);
        uint32_t objects = 0, aux = 0;
        for (size_t i = 0; i < flow.area_count; ++i) { const auto& a = flow.areas[i]; objects += static_cast<uint32_t>(a.objects.records.size()); aux += static_cast<uint32_t>(a.aux.records.size()); }
        const auto cooked = l3::cook_l3st00_flow(flow);
        write_all(argv[2], cooked);
        std::cout << "areas=" << flow.area_count << " objects=" << objects << " aux=" << aux << " bytes=" << cooked.size() << "\n";
        for (size_t i=0; i<flow.area_count; ++i) {
            const auto& a=flow.areas[i];
            std::cout << "area" << i << " objects=" << a.objects.records.size() << " aux=" << a.aux.records.size()
                      << " list_off=0x" << std::hex << a.source_object_offset
                      << " aux_off=0x" << a.source_aux_offset
                      << " resource_off=0x" << a.source_resource_offset
                      << std::dec << " setup_selector=" << a.setup_selector << "\n";
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
}

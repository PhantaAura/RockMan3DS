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
        if (argc != 3) { std::cerr << "usage: l3cook_st00_flow R3_ST00.BIN out.l3flow\n"; return 2; }
        const auto raw = read_all(argv[1]);
        const auto flow = l3::parse_st00_flow_us(raw);
        uint32_t objects = 0, aux = 0;
        for (const auto& a : flow.areas) { objects += static_cast<uint32_t>(a.objects.records.size()); aux += static_cast<uint32_t>(a.aux.records.size()); }
        const auto cooked = l3::cook_l3st00_flow(flow);
        write_all(argv[2], cooked);
        std::cout << "areas=" << flow.areas.size() << " objects=" << objects << " aux=" << aux << " bytes=" << cooked.size() << "\n";
        for (size_t i=0; i<flow.areas.size(); ++i) {
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

#include "legends3ds/st00_spawn.h"
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
        if (argc != 3 && argc != 6) {
            std::cerr << "usage: l3cook_st00_spawn R3_STxx.BIN out.l3spawn [slot_array stage area_count]\n";
            return 2;
        }
        const auto raw = read_all(argv[1]);
        const auto catalog = argc == 3 ? l3::parse_st00_spawn_us(raw)
            : l3::parse_stage_spawn_us(raw, static_cast<uint32_t>(std::stoul(argv[3], nullptr, 0)),
                  static_cast<uint8_t>(std::stoul(argv[4], nullptr, 0)), std::stoul(argv[5], nullptr, 0));
        const auto cooked = l3::cook_l3st00_spawn(catalog);
        write_all(argv[2], cooked);
        const auto h = l3::inspect_l3st00_spawn(cooked);
        std::cout << "areas=" << h.area_count << " regions=" << h.region_count
                  << " conditional_records=" << h.record_count << " bytes=" << cooked.size() << "\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
}

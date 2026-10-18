#include "legends3ds/ebd_directory.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open output");
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!f) throw std::runtime_error("write failed");
}
int main(int argc, char** argv) {
    try {
        if (argc != 4) {
            std::cerr << "usage: l3cook_ebd INPUT.EBD LOAD_BASE OUTPUT.l3ebd\n";
            return 2;
        }
        const auto source = read_file(argv[1]);
        const uint32_t base = static_cast<uint32_t>(std::stoul(argv[2], nullptr, 0));
        const auto dir = l3::parse_ebd_directory(source, base);
        const auto cooked = l3::cook_l3ebd_directory(dir);
        write_file(argv[3], cooked);
        std::cout << "EBD rows=" << dir.entries.size() << " -> " << cooked.size() << " bytes\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_ebd: " << e.what() << "\n";
        return 1;
    }
}

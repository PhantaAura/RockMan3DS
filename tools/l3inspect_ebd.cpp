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
int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: l3inspect_ebd FILE.l3ebd\n"; return 2; }
        const auto b = read_file(argv[1]);
        const auto h = l3::inspect_l3ebd_directory(b, true);
        std::cout << "L3EB v" << h.version << " rows=" << h.entry_count
                  << " source_base=0x" << std::hex << h.source_load_base << std::dec
                  << " bytes=" << h.file_bytes
                  << " retained_source_bytes=" << h.source_payload_bytes << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3inspect_ebd: " << e.what() << "\n";
        return 1;
    }
}

// Cooks the 32-row CPU palette bank (0x800AD168 rows 0..31) that the original
// loader fills for an ordered upload batch, with its dirtied rows, as L3PB.
//   l3cook_palette_bank OUT.l3pb UPLOAD_ORDER_CONTAINER.BIN...
// Output is owner-local data, never part of the source release.
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
void put(std::vector<uint8_t>& o, uint32_t v, int n) { for (int i = 0; i < n; ++i) o.push_back(uint8_t(v >> (8 * i))); }
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) { std::cerr << "usage: l3cook_palette_bank OUT.l3pb UPLOAD_ORDER_CONTAINER.BIN...\n"; return 2; }
        std::vector<l3::IndexedResource> uploads;
        for (int i = 2; i < argc; ++i) {
            auto rows = l3::scan_mml_indexed_resources(l3::read_file(argv[i]));
            uploads.insert(uploads.end(), std::make_move_iterator(rows.begin()), std::make_move_iterator(rows.end()));
        }
        const auto bank = l3::compose_palette_bank(uploads);
        if (bank.words.size() != 8192 || bank.written.size() != 8192) throw std::runtime_error("palette bank shape");
        std::vector<uint8_t> payload;
        put(payload, bank.dirty, 4);
        for (const uint16_t w : bank.words) put(payload, w, 2);
        for (const uint8_t f : bank.written) payload.push_back(f ? 1 : 0);
        std::vector<uint8_t> out{'L', '3', 'P', 'B'};
        put(out, 1, 2); put(out, 16, 2); put(out, uint32_t(16 + payload.size()), 4); put(out, crc32(payload.data(), payload.size()), 4);
        out.insert(out.end(), payload.begin(), payload.end());
        std::ofstream f(argv[1], std::ios::binary);
        if (!f) throw std::runtime_error("cannot open output");
        f.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
        size_t written = 0; for (const uint8_t x : bank.written) written += x ? 1 : 0;
        std::cout << "dirty=0x" << std::hex << bank.dirty << std::dec << " written_words=" << written << " bytes=" << out.size() << "\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
}

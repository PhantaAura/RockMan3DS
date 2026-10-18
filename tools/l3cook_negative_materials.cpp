#include "legends3ds/render/negative_materials.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: l3cook_negative_materials R3_ST00.BIN OUT.l3nm\n";
            return 2;
        }
        const auto overlay = l3::read_file(argv[1]);
        // Fixed address reconstruction requires the supplied USA overlay profile.
        // SHA-256: 9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c.
        // The outer owner profile verifies SHA-256; this local guard rejects a
        // changed size/CRC rather than applying fixed offsets to unknown data.
        if (overlay.size() != 141056u || l3::crc32(overlay.data(), overlay.size()) != 0x46628af1u)
            throw std::runtime_error("unsupported USA tutorial overlay identity");
        constexpr size_t kTableOffset = 0x1ad1cu, kSelectorOffset = 0x1ade4u;
        constexpr size_t kAreas = 5, kAuthoredRows = 25;
        l3::NegativeMaterialTables tables(kAreas);
        uint32_t known_rows = 0;
        for (size_t area = 0; area < kAreas; ++area) {
            const uint32_t selector = l3::rd_u32(overlay.data() + kSelectorOffset + area * 4u);
            const uint32_t end = area + 1u < kAreas ?
                l3::rd_u32(overlay.data() + kSelectorOffset + (area + 1u) * 4u) : kAuthoredRows;
            if (selector > end || end > kAuthoredRows || end - selector > 32)
                throw std::runtime_error("tutorial negative material selector range invalid");
            // Original 0x800371A0 copies 256 bytes, which also crosses into later
            // tables. Admit only this area's authored rows before the next selector;
            // the copied bytes beyond that region have no assigned material meaning.
            for (uint32_t slot = 0; slot < end - selector; ++slot) {
                const uint32_t word = l3::rd_u32(overlay.data() + kTableOffset + (selector + slot) * 8u);
                auto& binding = tables[area][slot];
                binding.material_word = (word & 0xffff0000u) | 0x1du;
                binding.atlas_tile = static_cast<uint8_t>(word & 0xffu);
                binding.known = true;
                ++known_rows;
            }
        }
        const auto cooked = l3::cook_l3negative_materials(tables);
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!out) throw std::runtime_error("failed writing negative material bindings");
        std::cout << "L3NM areas=" << tables.size() << " known_rows=" << known_rows
                  << " unknown_rows=" << tables.size() * 32u - known_rows
                  << " bytes=" << cooked.size() << " status=SOURCE_BACKED_PARTIAL\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3cook_negative_materials: " << error.what() << '\n';
        return 1;
    }
}

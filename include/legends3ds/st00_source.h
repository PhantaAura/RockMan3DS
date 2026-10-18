#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace l3 {
constexpr uint32_t kSt00MdtBase = 0x80164000u;
constexpr uint32_t kSt00StgBase = 0x80194000u;

struct SourceArea {
    uint8_t start_x = 0;
    uint8_t start_z = 0;
    uint8_t width = 0;
    uint8_t height = 0;
    std::vector<int16_t> cells;
};
struct SourceIdx {
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
    int16_t raw6 = 0;
    uint32_t descriptor_ptr = 0;
};
struct SourceContactPrimitive {
    int16_t y0 = 0, y1 = 0;
    uint16_t code = 0, flags = 0;
};
struct SourceContactGroup {
    int16_t x0 = 0, x1 = 0, z0 = 0, z1 = 0;
    std::vector<SourceContactPrimitive> primitives;
};
struct SourceContactList {
    uint32_t source_ptr = 0;
    std::vector<SourceContactGroup> groups;
};
struct SourceDescriptor {
    uint32_t source_ptr = 0;
    uint32_t word0 = 0;
    uint32_t alt_contact_ptr = 0;
    uint32_t default_contact_ptr = 0;
    std::array<int16_t, 6> raw_words3_5{};
    std::array<uint32_t, 6> render_ptrs{};
};
struct St00Source {
    std::vector<SourceArea> areas;
    std::vector<SourceIdx> idx;
    std::vector<SourceDescriptor> descriptors;
    std::vector<SourceContactList> contact_lists;
    // Original HD2 row pointers are retained only for bit7-set selectors used
    // by this source. Zero means missing/unknown; no HD2 tag semantics inferred.
    std::array<uint32_t, 32> special_negative_descriptor_ptrs{};
};

std::vector<uint8_t> read_file(const std::filesystem::path& path);
St00Source parse_st00(const std::vector<uint8_t>& mdt,
                      const std::vector<uint8_t>& idx,
                      const std::vector<uint8_t>& stg,
                      const std::vector<uint8_t>& hed,
                      const std::vector<uint8_t>& hd2 = {});
}

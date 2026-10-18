#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {
struct PbdVertex { int16_t x=0,y=0,z=0; uint16_t pad=0; };
struct PbdTri {
    std::array<uint8_t,3> index{};
    std::array<uint8_t,6> uv{};
    uint16_t clut=0,tpage=0;
    uint8_t r=0,g=0,b=0,code=0;
};
struct PbdQuad {
    std::array<uint8_t,4> index{};
    std::array<uint8_t,8> uv{};
    uint16_t clut=0,tpage=0;
    uint8_t r=0,g=0,b=0,code=0;
};
struct PbdPart {
    uint8_t part_id=0;
    std::vector<PbdVertex> vertices;
    std::vector<PbdTri> triangles;
    std::vector<PbdQuad> quads;
};
struct PbdModel {
    uint32_t inferred_load_base=0;
    std::vector<PbdPart> parts;
};
struct L3ModelStats {
    uint32_t part_count=0, vertex_count=0, triangle_count=0, quad_count=0;
};
struct L3ModelHeader {
    uint16_t version=0, header_bytes=0;
    uint32_t flags=0;
    L3ModelStats stats{};
    uint32_t off_parts=0, off_vertices=0, off_triangles=0, off_quads=0;
    uint32_t file_bytes=0, payload_crc32=0;
};
constexpr uint16_t kL3ModelVersion=1;
PbdModel parse_player_pbd_mesh(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> cook_l3model(const PbdModel& model, L3ModelStats* stats=nullptr);
L3ModelHeader inspect_l3model(const std::vector<uint8_t>& bytes, bool verify_crc=true);
// Runtime reader for pointer-free cooked L3MD data.
PbdModel load_l3model(const std::vector<uint8_t>& bytes, bool verify_crc=true);
}

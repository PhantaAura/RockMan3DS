#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

struct NegativeMaterialBinding {
    uint32_t material_word = 0;
    uint8_t atlas_tile = 0;
    bool known = false;
    // Second word of the 0x800BADB0 row (0x800BADB4 -> 0x1F800180): the atlas
    // tiles of the first faces of a Near-LOD stream (0x80021B14/0x8002235C).
    // L3NM v1 packs lack them (near_known false).
    std::array<uint8_t, 4> near_tiles{};
    bool near_known = false;
};
using NegativeMaterialTable = std::array<NegativeMaterialBinding, 32>;
using NegativeMaterialTables = std::vector<NegativeMaterialTable>;

// Pointer-free area-local bindings. These contain owner data and belong in the
// local cooked pack, never in a source release. Unknown rows are canonically zero.
std::vector<uint8_t> cook_l3negative_materials(const NegativeMaterialTables& tables);
NegativeMaterialTables load_l3negative_materials(const std::vector<uint8_t>& bytes,
                                                 bool verify_crc = true);

// Only the default negative branch (bit7 clear) uses this atlas table. A missing
// table/row remains unknown and must retain an untextured geometry fallback.
const NegativeMaterialBinding* find_negative_material_binding(
    const NegativeMaterialTables& tables, size_t area, uint16_t packed_cell) noexcept;

// Executable-backed Mid/Far default branch: a 32x32 atlas tile, preserving the
// source quad vertex order. Near subdivision uses additional state and is pending.
std::array<uint8_t, 8> negative_material_quad_uv(const NegativeMaterialBinding& binding);
// Near LOD (packed bit 15 kept): the stream UVs of face `face` of `count`,
// each offset by an atlas tile ((t & 7) * 32 in u, (t >> 3) * 32 in v). Face
// i uses near_tiles[i + 8 - count] while count - 1 - i > 3, else atlas_tile.
std::array<uint8_t, 8> negative_material_near_uv(const NegativeMaterialBinding& binding,
    const std::array<uint8_t, 8>& stream_uv, size_t face, size_t count);

} // namespace l3

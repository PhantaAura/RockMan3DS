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

} // namespace l3

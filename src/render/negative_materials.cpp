#include "legends3ds/render/negative_materials.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeader = 16, kRow = 8, kSlots = 32, kMaxAreas = 256;
void validate(const NegativeMaterialBinding& binding) {
    if (!binding.known) {
        if (binding.material_word || binding.atlas_tile)
            throw std::runtime_error("unknown negative material binding has nonzero fields");
        return;
    }
    if (binding.atlas_tile >= 64 || (binding.material_word & 0xffffu) != 0x1du)
        throw std::runtime_error("negative material binding has unsupported tile or texture page");
}
}

std::vector<uint8_t> cook_l3negative_materials(const NegativeMaterialTables& tables) {
    if (tables.empty() || tables.size() > kMaxAreas)
        throw std::runtime_error("negative material area count outside bounded format");
    std::vector<uint8_t> bytes;
    bytes.reserve(kHeader + tables.size() * kSlots * kRow);
    bytes.insert(bytes.end(), {'L', '3', 'N', 'M'});
    wr_u16(bytes, 1); wr_u16(bytes, static_cast<uint16_t>(tables.size()));
    wr_u32(bytes, static_cast<uint32_t>(kHeader + tables.size() * kSlots * kRow));
    wr_u32(bytes, 0);
    for (const auto& table : tables) for (const auto& binding : table) {
        validate(binding);
        wr_u32(bytes, binding.material_word);
        bytes.push_back(binding.atlas_tile);
        bytes.push_back(binding.known ? 1u : 0u);
        wr_u16(bytes, 0);
    }
    patch_u32(bytes, 12, crc32(bytes.data() + kHeader, bytes.size() - kHeader));
    return bytes;
}

NegativeMaterialTables load_l3negative_materials(const std::vector<uint8_t>& bytes,
                                                 bool verify_crc) {
    if (bytes.size() < kHeader || rd_u32(bytes.data()) != 0x4d4e334cu ||
        rd_u16(bytes.data() + 4) != 1)
        throw std::runtime_error("invalid L3NM header");
    const size_t areas = rd_u16(bytes.data() + 6);
    if (!areas || areas > kMaxAreas || bytes.size() != kHeader + areas * kSlots * kRow ||
        rd_u32(bytes.data() + 8) != bytes.size())
        throw std::runtime_error("invalid L3NM area count or byte length");
    if (verify_crc && rd_u32(bytes.data() + 12) !=
        crc32(bytes.data() + kHeader, bytes.size() - kHeader))
        throw std::runtime_error("L3NM payload CRC mismatch");
    NegativeMaterialTables tables(areas);
    size_t offset = kHeader;
    for (auto& table : tables) for (auto& binding : table) {
        binding.material_word = rd_u32(bytes.data() + offset);
        binding.atlas_tile = bytes[offset + 4];
        if (bytes[offset + 5] > 1 || rd_u16(bytes.data() + offset + 6))
            throw std::runtime_error("L3NM unknown flags or reserved field");
        binding.known = bytes[offset + 5] != 0;
        validate(binding);
        offset += kRow;
    }
    return tables;
}

const NegativeMaterialBinding* find_negative_material_binding(
    const NegativeMaterialTables& tables, size_t area, uint16_t packed_cell) noexcept {
    if (!(packed_cell & 0x8000u) || (packed_cell & 0x80u) || area >= tables.size())
        return nullptr;
    const size_t index = (packed_cell & 0x7cu) >> 2;
    const auto& binding = tables[area][index];
    return binding.known ? &binding : nullptr;
}

std::array<uint8_t, 8> negative_material_quad_uv(const NegativeMaterialBinding& binding) {
    validate(binding);
    if (!binding.known) throw std::runtime_error("negative material UV requested for unknown binding");
    const uint8_t u = static_cast<uint8_t>((binding.atlas_tile & 7u) * 32u);
    const uint8_t v = static_cast<uint8_t>((binding.atlas_tile >> 3) * 32u);
    return {{u, v, static_cast<uint8_t>(u + 31u), v,
             u, static_cast<uint8_t>(v + 31u),
             static_cast<uint8_t>(u + 31u), static_cast<uint8_t>(v + 31u)}};
}

} // namespace l3

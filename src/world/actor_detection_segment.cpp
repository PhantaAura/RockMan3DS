#include "legends3ds/actor_detection_segment.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
int16_t wrap(int32_t v) noexcept {
    const uint16_t word = static_cast<uint16_t>(v);
    return word < 0x8000u ? static_cast<int16_t>(word)
        : static_cast<int16_t>(static_cast<int32_t>(word) - 65536);
}
int32_t signed_word(uint32_t word) noexcept {
    return word < 0x80000000u ? static_cast<int32_t>(word)
        : static_cast<int32_t>(static_cast<int64_t>(word) - 0x100000000LL);
}
struct Candidates {
    std::array<RuntimeCellRef, 4> cells{};
    uint8_t count = 0;
    bool malformed = false;
};
Candidates gather(const RuntimeMap& map, size_t area_index, int16_t x, int16_t z) noexcept {
    Candidates out;
    if (area_index >= map.areas.size()) { out.malformed = true; return out; }
    const auto& area = map.areas[area_index];
    const uint16_t nx = (uint16_t(x) & 511u) < 257u ? 0x7e00u : 0x8200u;
    const uint16_t nz = (uint16_t(z) & 511u) < 257u ? 0x7e00u : 0x8200u;
    const std::array<std::array<uint16_t, 2>, 4> offsets{{
        {{0x8000u, 0x8000u}}, {{nx, 0x8000u}}, {{0x8000u, nz}}, {{nx, nz}}}};
    for (const auto& delta : offsets) {
        const auto shift9 = [](int32_t v) { return v >= 0 ? v / 512 : -((-v + 511) / 512); };
        const int32_t gx = shift9(int32_t(x) + delta[0]), gz = shift9(int32_t(z) + delta[1]);
        const int32_t lx = gx - area.start_x, lz = gz - area.start_z;
        // Source returns the candidates gathered so far immediately at an
        // invalid X or Z sample. It does not continue to later offsets.
        if (lx < 0 || lz < 0 || lx >= area.width || lz >= area.height) return out;
        const uint64_t local = uint64_t(lz) * area.width + uint64_t(lx);
        const uint64_t index = uint64_t(area.cell_first) + local;
        if (local >= area.cell_count || index >= map.cells.size() || index > UINT32_MAX) {
            out.malformed = true; return out;
        }
        const auto packed = map.cells[static_cast<size_t>(index)];
        bool duplicate = false;
        if (packed > 0) for (uint8_t i = 0; i < out.count; ++i)
            duplicate |= out.cells[i].packed_cell == packed;
        if (duplicate) continue;
        auto& cell = out.cells[out.count++];
        cell.area_index = area_index; cell.cell_index = static_cast<uint32_t>(index);
        cell.grid_x = static_cast<uint16_t>(gx); cell.grid_z = static_cast<uint16_t>(gz);
        cell.packed_cell = packed;
    }
    return out;
}
bool source_class_zero(uint32_t value) noexcept {
    if (value < 32u) return (value & 16u) == 0;
    return (value & 0xe0u) == 0;
}
struct Classifier {
    bool known = false;
    bool inside = false;
};
int32_t add_word(int32_t a, int32_t b) noexcept {
    return signed_word(uint32_t(a) + uint32_t(b));
}
int32_t sub_word(int32_t a, int32_t b) noexcept {
    return signed_word(uint32_t(a) - uint32_t(b));
}
bool divided_product(int32_t a, int32_t b, int32_t divisor, int32_t& out) noexcept {
    const int32_t product = signed_word(static_cast<uint32_t>(int64_t(a) * b));
    if (!divisor || (product == INT32_MIN && divisor == -1)) return false;
    out = product / divisor; // Original signed DIV truncates toward zero.
    return true;
}
Classifier classify(const RuntimeMapIdx& idx, const RuntimeContactGroup& group,
                    uint16_t code, RetailPlayerBounds bounds, int16_t x, int16_t z) noexcept {
    const unsigned family = (code & 0xf0u) >> 4;
    if (family > 7) return {};
    int16_t x0 = wrap(int32_t(idx.x) + group.x0);
    int16_t x1 = wrap(int32_t(idx.x) + group.x1);
    int16_t z0 = wrap(int32_t(idx.z) + group.z0);
    int16_t z1 = wrap(int32_t(idx.z) + group.z1);
    if (family < 2 || family > 5) {
        x0 = wrap(int32_t(x0) - bounds.x1); x1 = wrap(int32_t(x1) - bounds.x0);
        z0 = wrap(int32_t(z0) - bounds.z1); z1 = wrap(int32_t(z1) - bounds.z0);
    } else {
        // Each original diagonal classifier has an inverted bounds branch for
        // its 0x?5 code. Keep that branch even when its later Y helper is held.
        const bool special = code == uint16_t((family << 4) | 5u);
        const bool left = family == 2 || family == 4;
        const bool bottom = family == 2 || family == 3;
        const bool plus_x = left != special, plus_z = bottom == special;
        x0 = wrap(int32_t(x0) + (plus_x ? bounds.x1 : -int32_t(bounds.x1)));
        x1 = wrap(int32_t(x1) + (plus_x ? -int32_t(bounds.x0) : bounds.x0));
        z0 = wrap(int32_t(z0) + (plus_z ? bounds.z1 : -int32_t(bounds.z1)));
        z1 = wrap(int32_t(z1) + (plus_z ? -int32_t(bounds.z0) : bounds.z0));
    }
    if (family < 2) return {true, x0 < x && x < x1 && z0 < z && z < z1};
    if (family == 7) {
        // 8004F150 rejects outside its rectangle before its interior curved
        // arithmetic. That rejection is exact; intersecting family7 is held.
        if (!(x0 < x && x < x1 && z0 < z && z < z1)) return {true, false};
        return {};
    }
    const int32_t dx = int32_t(x1) - x0, dz = int32_t(z1) - z0;
    int32_t qx = 0, qz = 0;
    if (family == 6) {
        const int32_t center_x = int32_t(x0) + dx / 2;
        const int32_t center_z = int32_t(z0) + dz / 2;
        const int32_t distance_x = x >= center_x ? x - center_x : center_x - x;
        const int32_t distance_z = z >= center_z ? z - center_z : center_z - z;
        if (!divided_product(distance_z, dx, dz, qx) ||
            !divided_product(distance_x, dz, dx, qz)) return {};
        return {true, add_word(x0, qx) < x && x < sub_word(x1, qx) &&
                      add_word(z0, qz) < z && z < sub_word(z1, qz)};
    }
    if (!divided_product(int32_t(z) - z0, dx, dz, qx) ||
        !divided_product(int32_t(x) - x0, dz, dx, qz)) return {};
    int32_t low_x = x0, high_x = x1, low_z = z0, high_z = z1;
    switch (family) {
        case 2:
            high_x = std::min(int32_t(x1), add_word(add_word(x0, qx), 4));
            low_z = std::max(int32_t(z0), sub_word(add_word(z0, qz), 4));
            break;
        case 3:
            low_x = std::max(int32_t(x0), sub_word(sub_word(x1, qx), 4));
            low_z = std::max(int32_t(z0), sub_word(sub_word(z1, qz), 4));
            break;
        case 4:
            high_x = std::min(int32_t(x1), add_word(sub_word(x1, qx), 4));
            high_z = std::min(int32_t(z1), add_word(sub_word(z1, qz), 4));
            break;
        case 5:
            low_x = std::max(int32_t(x0), sub_word(add_word(x0, qx), 4));
            high_z = std::min(int32_t(z1), add_word(add_word(z0, qz), 4));
            break;
        default: return {};
    }
    // The original consumer uses classifier truthiness. Its further endpoint
    // classification and response scratch stores do not change this query.
    return {true, low_x < x && x < high_x && low_z < z && z < high_z};
}
bool plain_y_interval(uint16_t code) noexcept {
    // 8004A060 dispatches on the low byte. Retain every slope-specific branch
    // as unknown; all other branches reach the unchanged Y interval at AC30.
    const uint16_t low = code & 255u;
    return !((low >= 8u && low <= 14u) || (low >= 16u && low <= 19u) ||
        (low >= 24u && low <= 27u) || low == 36u || low == 37u ||
        low == 52u || low == 53u || low == 68u || low == 69u ||
        low == 84u || low == 85u);
}
void patch_u16(std::vector<uint8_t>& bytes, size_t at, uint16_t value) {
    if (at + 2 > bytes.size()) throw std::runtime_error("L3DB halfword write escaped header");
    bytes[at] = static_cast<uint8_t>(value); bytes[at + 1] = static_cast<uint8_t>(value >> 8);
}
bool valid_bounds(RetailPlayerBounds b) noexcept {
    return b.x0 < b.x1 && b.z0 < b.z1 && b.y0 < b.y1;
}
}
ActorDetectionSegment query_actor_detection_segment_80049c54(
    const RuntimeMap& map, size_t area, const std::array<int16_t, 4>& start,
    const std::array<int16_t, 4>& end, RetailPlayerBounds bounds, uint32_t source_class) noexcept {
    ActorDetectionSegment out;
    const auto overflow_axis = [](int16_t first, int16_t last) {
        return ((uint16_t(first) ^ uint16_t(last)) & 0x8000u) &&
               (first < -16384 || first > 16384);
    };
    if (overflow_axis(start[0], end[0]) || overflow_axis(start[2], end[2])) {
        out.status = RuntimeContactStatus::SourceBackedPartial;
        out.result = signed_word(0x80000000u); return out;
    }
    if (area >= map.areas.size()) return out;
    const auto& area_data = map.areas[area];
    const int32_t lx = (int32_t(start[0]) + 32768) / 512 - area_data.start_x;
    const int32_t lz = (int32_t(start[2]) + 32768) / 512 - area_data.start_z;
    if (lx < 0 || lz < 0 || lx >= area_data.width || lz >= area_data.height) {
        out.status = RuntimeContactStatus::SourceBackedPartial;
        out.result = 0x40000000; return out;
    }
    const uint64_t local = uint64_t(lz) * area_data.width + uint64_t(lx);
    const uint64_t containing_index = uint64_t(area_data.cell_first) + local;
    if (local >= area_data.cell_count || containing_index >= map.cells.size()) return out;
    const auto containing = map.cells[static_cast<size_t>(containing_index)];
    if (!containing) {
        out.status = RuntimeContactStatus::SourceBackedPartial;
        out.result = 0x40000000; return out;
    }
    const auto candidates = gather(map, area, start[0], start[2]);
    out.candidate_cells = candidates.count;
    if (candidates.malformed) return out;
    out.status = RuntimeContactStatus::SourceBackedPartial;
    const bool skip_flag8000 = !source_class_zero(source_class);
    for (uint8_t reverse = candidates.count; reverse > 0; --reverse) {
        const auto& cell = candidates.cells[reverse - 1];
        if (cell.packed_cell <= 0) {
            if (cell.cell_index != containing_index) continue;
            const uint16_t packed = static_cast<uint16_t>(cell.packed_cell);
            const int32_t top = -int32_t((packed & 0x1f00u) >> 1);
            const int32_t floor = top - 1 - bounds.y1;
            const uint32_t bands = (packed & 0x6000u) >> 13;
            const int32_t ceiling = top - int32_t(bands) * 384 + 1 - bounds.y0;
            if (floor < start[1] || (bands && start[1] < ceiling)) {
                out.result = packed; return out;
            }
            continue;
        }
        const auto* idx = runtime_map_cell_idx(map, cell);
        const auto* list = runtime_map_cell_contact_list(map, cell);
        if (!idx || !list || uint64_t(list->group_first) + list->group_count > map.contact_groups.size()) {
            out.status = RuntimeContactStatus::Unsupported; return out;
        }
        for (uint32_t gi = 0; gi < list->group_count; ++gi) {
            const auto& group = map.contact_groups[list->group_first + gi];
            ++out.groups_tested;
            if (!group.primitive_count || uint64_t(group.primitive_first) + group.primitive_count > map.contact_primitives.size()) {
                out.status = RuntimeContactStatus::Unsupported; return out;
            }
            const uint16_t first_code = map.contact_primitives[group.primitive_first].code;
            if (first_code >= 256u) continue;
            const auto classification = classify(*idx, group, first_code, bounds, start[0], start[2]);
            if (!classification.known) {
                out.unsupported_cell_index = cell.cell_index;
                out.unsupported_group_index = list->group_first + gi;
                out.unsupported_primitive_index = group.primitive_first;
                out.unsupported_code = first_code;
                ++out.unsupported_primitives; out.status = RuntimeContactStatus::Unsupported; return out;
            }
            if (!classification.inside) continue;
            for (uint32_t pi = 0; pi < group.primitive_count; ++pi) {
                const auto& primitive = map.contact_primitives[group.primitive_first + pi];
                ++out.primitives_tested;
                if (skip_flag8000 && (primitive.flags & 0x8000u)) continue;
                if (!plain_y_interval(primitive.code)) {
                    out.unsupported_cell_index = cell.cell_index;
                    out.unsupported_group_index = list->group_first + gi;
                    out.unsupported_primitive_index = group.primitive_first + pi;
                    out.unsupported_code = primitive.code;
                    ++out.unsupported_primitives; out.status = RuntimeContactStatus::Unsupported; return out;
                }
                const auto y0 = wrap(int32_t(idx->y) + primitive.y0 - bounds.y1);
                const auto y1 = wrap(int32_t(idx->y) + primitive.y1 - bounds.y0);
                if (y0 < start[1] && start[1] < y1) {
                    out.result = signed_word(uint16_t(cell.packed_cell) |
                        (uint32_t(primitive.flags & 15u) << 16));
                    return out;
                }
            }
        }
    }
    return out;
}
std::vector<uint8_t> encode_actor_detection_segment_bounds(RetailPlayerBounds bounds) {
    if (!valid_bounds(bounds)) throw std::runtime_error("L3DB bounds pairs are unordered");
    std::vector<uint8_t> out(44);
    out[0] = 'L'; out[1] = '3'; out[2] = 'D'; out[3] = 'B';
    patch_u16(out, 4, 1); patch_u16(out, 6, 32);
    patch_u32(out, 8, 0); patch_u32(out, 12, 0x80097d0cu);
    patch_u32(out, 16, 6); patch_u32(out, 20, 12); patch_u32(out, 28, 44);
    const std::array<int16_t, 6> values{{bounds.x0, bounds.x1, bounds.z0, bounds.z1, bounds.y0, bounds.y1}};
    for (size_t i = 0; i < values.size(); ++i) patch_u16(out, 32 + i * 2, static_cast<uint16_t>(values[i]));
    patch_u32(out, 24, crc32(out.data() + 32, 12));
    return out;
}
RetailPlayerBounds load_actor_detection_segment_bounds(const std::vector<uint8_t>& bytes) {
    if (bytes.size() != 44 || bytes[0] != 'L' || bytes[1] != '3' || bytes[2] != 'D' || bytes[3] != 'B' ||
        rd_u16(bytes.data() + 4) != 1 || rd_u16(bytes.data() + 6) != 32 || rd_u32(bytes.data() + 8) != 0 ||
        rd_u32(bytes.data() + 12) != 0x80097d0cu || rd_u32(bytes.data() + 16) != 6 || rd_u32(bytes.data() + 20) != 12 ||
        rd_u32(bytes.data() + 28) != bytes.size() || rd_u32(bytes.data() + 24) != crc32(bytes.data() + 32, 12))
        throw std::runtime_error("invalid L3DB detector segment bounds");
    RetailPlayerBounds out{wrap(rd_u16(bytes.data() + 32)), wrap(rd_u16(bytes.data() + 34)),
        wrap(rd_u16(bytes.data() + 36)), wrap(rd_u16(bytes.data() + 38)), wrap(rd_u16(bytes.data() + 40)), wrap(rd_u16(bytes.data() + 42))};
    if (!valid_bounds(out)) throw std::runtime_error("L3DB bounds pairs are unordered");
    return out;
}
} // namespace l3

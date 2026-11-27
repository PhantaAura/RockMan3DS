#include "legends3ds/actor_contact.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>

namespace l3 {
namespace {
int16_t wrap(int32_t value) noexcept {
    return static_cast<int16_t>(static_cast<uint16_t>(value));
}
bool fits_short(int32_t value) noexcept {
    return value >= INT16_MIN && value <= INT16_MAX;
}
struct Candidates {
    std::array<RuntimeCellRef, 4> cells{};
    uint8_t count = 0;
    bool malformed = false;
};
Candidates gather(const RuntimeMap& map, size_t area, int16_t x, int16_t z) noexcept {
    Candidates out;
    if (area >= map.areas.size()) { out.malformed = true; return out; }
    const auto& data = map.areas[area];
    const int32_t nx = (uint16_t(x) & 511u) < 257u ? -512 : 512;
    const int32_t nz = (uint16_t(z) & 511u) < 257u ? -512 : 512;
    const std::array<std::array<int32_t, 2>, 4> offsets{{{{0, 0}}, {{nx, 0}}, {{0, nz}}, {{nx, nz}}}};
    for (const auto& delta : offsets) {
        const auto grid = [](int32_t value) { return value >= 0 ? value / 512 : -((-value + 511) / 512); };
        const int32_t gx = grid(int32_t(x) + delta[0] + 32768);
        const int32_t gz = grid(int32_t(z) + delta[1] + 32768);
        const int32_t lx = gx - data.start_x, lz = gz - data.start_z;
        if (lx < 0 || lz < 0 || lx >= data.width || lz >= data.height) continue;
        const uint64_t index = uint64_t(data.cell_first) + uint64_t(lz) * data.width + uint64_t(lx);
        if (index >= map.cells.size() || index > UINT32_MAX) { out.malformed = true; continue; }
        const auto packed = map.cells[static_cast<size_t>(index)];
        if (!packed) continue;
        bool duplicate = false;
        if (packed > 0)
            for (uint8_t i = 0; i < out.count; ++i)
                if (out.cells[i].packed_cell == packed) duplicate = true;
        if (duplicate) continue;
        auto& cell = out.cells[out.count++];
        cell.area_index = area; cell.cell_index = static_cast<uint32_t>(index);
        cell.grid_x = static_cast<uint16_t>(gx); cell.grid_z = static_cast<uint16_t>(gz);
        cell.packed_cell = packed;
    }
    return out;
}
struct Rect {
    int16_t x0 = 0, x1 = 0, z0 = 0, z1 = 0, y0 = 0, y1 = 0;
    bool ordered_unwrapped = false;
};
Rect rectangle(const RuntimeMapIdx& idx, const RuntimeContactGroup& group,
               const RuntimeContactPrimitive& primitive, RetailPlayerBounds bounds) noexcept {
    const std::array<int32_t, 6> values{{int32_t(idx.x) + group.x0 - bounds.x1,
        int32_t(idx.x) + group.x1 - bounds.x0, int32_t(idx.z) + group.z0 - bounds.z1,
        int32_t(idx.z) + group.z1 - bounds.z0, int32_t(idx.y) + primitive.y0 - bounds.y1,
        int32_t(idx.y) + primitive.y1 - bounds.y0}};
    Rect out{wrap(values[0]), wrap(values[1]), wrap(values[2]), wrap(values[3]),
             wrap(values[4]), wrap(values[5]), true};
    for (const auto value : values) out.ordered_unwrapped &= fits_short(value);
    out.ordered_unwrapped &= values[0] < values[1] && values[2] < values[3] && values[4] < values[5];
    return out;
}
bool inside_xz(const Rect& rect, int16_t x, int16_t z) noexcept {
    return rect.x0 < x && x < rect.x1 && rect.z0 < z && z < rect.z1;
}
bool diagonal_excluded(const RuntimeMapIdx& idx, const RuntimeContactGroup& group,
                       uint16_t family, RetailPlayerBounds bounds, int16_t x, int16_t z) noexcept {
    if ((family & 0xf0u) < 0x20u || (family & 0xf0u) > 0x50u ||
        bounds.x1 < 0 || bounds.z1 < 0 || int32_t(bounds.x0) != -int32_t(bounds.x1) ||
        int32_t(bounds.z0) != -int32_t(bounds.z1)) return false;
    const int32_t x0 = int32_t(idx.x) + group.x0, x1 = int32_t(idx.x) + group.x1;
    const int32_t z0 = int32_t(idx.z) + group.z0, z1 = int32_t(idx.z) + group.z1;
    if (x0 >= x1 || z0 >= z1 || !fits_short(x0 - bounds.x1) ||
        !fits_short(x1 - bounds.x0) || !fits_short(z0 - bounds.z1) || !fits_short(z1 - bounds.z0)) return false;
    return !(x0 - bounds.x1 < x && x < x1 - bounds.x0 &&
             z0 - bounds.z1 < z && z < z1 - bounds.z0);
}
bool core_ported(uint16_t code) noexcept {
    // Primitive codes the shared 0x8004BDC8 port answers (player_contact_runtime.cpp).
    if (code >= 256u) return true;   // the dispatcher admits only codes below 0x100
    const unsigned family = (code & 0xf0u) >> 4, low = code & 0x0fu;
    if (family < 2u) return code == 1u || code == 3u || code == 15u || (code >= 8u && code <= 11u) ||
                            (code >= 0x10u && code <= 0x13u) || (code >= 0x1Cu && code <= 0x1Fu);
    if (family <= 5u) return low == 1u || low == 2u || low == 4u;
    return false;
}
bool allowed(const RuntimeContactPrimitive& primitive, uint8_t object_class) noexcept {
    return l3_obj_group_bits_8002fe74(object_class) == 0 || !(primitive.flags & 0x8000u);
}
} // namespace

ActorWallContact query_actor_wall_contact_8004b6ac(
    const RuntimeMap& map, size_t area, const ActorContactWork& work, RetailPlayerBounds bounds) noexcept {
    // 0x8004B414 calls the same core 0x8004B6AC (dispatcher 0x8004BDC8) for every
    // object, so actors share the player port: all primitive families it covers
    // (rectangles, slopes 8..11, diagonals x1/x2, sloped diagonals 0x24..0x54),
    // the 0x1F800100/106 correction history and the 0x80050400 deep-overlap
    // recovery. Only the work fields the core reads are filled: +14/+16/+18,
    // +2C/+2E/+30, the class byte (+1, the 0x8002FE74 filter) and +74.
    ActorWallContact out;
    const auto candidates = gather(map, area, work.raw[0], work.raw[2]);
    out.candidate_cells = candidates.count;
    if (candidates.malformed) return out;   // Unsupported: the grid itself is out of range
    if (!candidates.count) { out.status = RuntimeContactStatus::NoData; return out; }
    L3PlayerCompatWork w{};
    const auto put = [&](size_t o, int16_t v) {
        w.bytes[o] = uint8_t(uint16_t(v)); w.bytes[o + 1] = uint8_t(uint16_t(v) >> 8);
    };
    put(0x14, work.raw[0]); put(0x16, work.raw[1]); put(0x18, work.raw[2]);
    put(0x2C, work.previous_raw[0]); put(0x2E, work.previous_raw[1]); put(0x30, work.previous_raw[2]);
    w.bytes[1] = work.object_class; w.bytes[0x74] = work.state74;
    // Conservative actor rule kept from the earlier subset: an unported primitive
    // whose expanded box holds the actor in X/Z rejects the query (the original
    // handler might respond; the port cannot say).
    for (uint8_t i = 0; i < candidates.count; ++i) {
        const auto& cell = candidates.cells[i];
        if (cell.packed_cell <= 0) continue;
        const auto* idx = runtime_map_cell_idx(map, cell);
        const auto* list = runtime_map_cell_contact_list(map, cell);
        if (!idx || !list) continue;
        for (uint32_t gi = 0; gi < list->group_count; ++gi) {
            const uint64_t gix = uint64_t(list->group_first) + gi;
            if (gix >= map.contact_groups.size()) break;
            const auto& group = map.contact_groups[static_cast<size_t>(gix)];
            for (uint32_t pi = 0; pi < group.primitive_count; ++pi) {
                const uint64_t pix = uint64_t(group.primitive_first) + pi;
                if (pix >= map.contact_primitives.size()) break;
                const auto& primitive = map.contact_primitives[static_cast<size_t>(pix)];
                if (core_ported(primitive.code) || !allowed(primitive, work.object_class)) continue;
                if (inside_xz(rectangle(*idx, group, primitive, bounds), work.raw[0], work.raw[2])) {
                    out.unsupported_primitives = 1; return out;
                }
            }
        }
    }
    const auto core = runtime_player_contact_step(map, area, w, bounds);
    out.scratch = core.scratch;
    out.candidate_cells = core.candidate_cells;
    out.groups_tested = core.groups_tested;
    out.primitives_tested = core.primitives_tested;
    out.unsupported_primitives = core.unsupported_primitives;
    out.iterations = core.iterations;
    out.result = core.result;
    out.status = core.candidate_cells == 0 ? RuntimeContactStatus::NoData : core.status;
    // After nine passes 0x8004B6AC returns 0x80000000; 0x8004B414 then takes its
    // negative branch (previous pose restored), so the wrapper commits it.
    if (core.status == RuntimeContactStatus::IterationLimit) {
        out.status = RuntimeContactStatus::SourceBackedPartial;
        out.result = static_cast<int32_t>(0x80000000u);
    }
    return out;
}

bool apply_actor_contact_result_8004b414(ActorContactWork& work, const ActorWallContact& core) noexcept {
    if (core.status != RuntimeContactStatus::SourceBackedPartial) return false;
    if (core.result == 0) return true;
    if (core.result > 0) {
        const std::array<int16_t, 3> corrected{{core.scratch.x, core.scratch.y, core.scratch.z}};
        const std::array<int32_t, 3> masks{{0x300, 0x3000, 0xc00}};
        for (size_t i = 0; i < corrected.size(); ++i)
            if (core.result & masks[i]) { work.raw[i] = corrected[i]; work.fixed[i] = int32_t(corrected[i]) * 65536; }
        if (work.state74 != 2) work.state74 = core.scratch.contact_state;
    } else if (l3_obj_group_bits_8002fe74(work.object_class) != 0) {
        work.raw = work.previous_raw; work.fixed = work.previous_fixed;
    } else {
        for (const size_t i : {size_t(0), size_t(2)}) { work.raw[i] = work.previous_raw[i]; work.fixed[i] = work.previous_fixed[i]; }
        work.velocity[0] = 0; work.velocity[2] = 0;
        if (work.state74 != 0) {
            work.raw[1] = work.previous_raw[1]; work.fixed[1] = work.previous_fixed[1];
            work.velocity[1] = 0; work.state74 = 0;
        }
    }
    return true;
}

ActorFloorContact query_actor_floor_contact_80048ea4(
    const RuntimeMap& map, size_t area, const ActorContactWork& work, RetailPlayerBounds bounds) noexcept {
    ActorFloorContact out;
    const int16_t x = work.raw[0], y = work.raw[1], z = work.raw[2];
    const auto candidates = gather(map, area, x, z);
    out.candidate_cells = candidates.count;
    if (candidates.malformed) return out;
    if (!candidates.count) { out.status = RuntimeContactStatus::NoData; return out; }
    out.status = RuntimeContactStatus::SourceBackedPartial;
    const auto containing = runtime_map_find_cell(map, area, x, z);
    const int32_t dy = int32_t(y) - work.previous_raw[1];
    if (!fits_short(dy)) { out.status = RuntimeContactStatus::Unsupported; return out; }
    int32_t best = 32767;
    for (unsigned pass = 0; pass < 4; ++pass) {
        const int32_t previous = best;
        for (uint8_t reverse = candidates.count; reverse > 0; --reverse) {
            const auto& cell = candidates.cells[reverse - 1];
            if (cell.packed_cell < 0) {
                if (!containing || containing->cell_index != cell.cell_index) continue;
                const int32_t floor = -int32_t((uint16_t(cell.packed_cell) & 0x1f00u) >> 1) - 1 - bounds.y1;
                if (floor < best) { best = floor; out.cell_index = cell.cell_index; out.primitive_index.reset(); }
                continue;
            }
            const auto* idx = runtime_map_cell_idx(map, cell);
            const auto* list = runtime_map_cell_contact_list(map, cell);
            if (!idx || !list) { ++out.unsupported_primitives; continue; }
            if (uint64_t(list->group_first) + list->group_count > map.contact_groups.size()) {
                ++out.unsupported_primitives; continue;
            }
            for (uint32_t gi = 0; gi < list->group_count; ++gi) {
                const uint64_t index = uint64_t(list->group_first) + gi;
                if (index >= map.contact_groups.size()) { ++out.unsupported_primitives; continue; }
                const auto& group = map.contact_groups[static_cast<size_t>(index)];
                if (!group.primitive_count) continue;
                if (uint64_t(group.primitive_first) + group.primitive_count > map.contact_primitives.size()) {
                    ++out.unsupported_primitives; continue;
                }
                const auto family = map.contact_primitives[group.primitive_first].code;
                if (family >= 256u) continue;
                // Families 2..5 answered by the shared 0x8004A060: sloped diagonals (x4) and the
                // diagonal walls x1/x2/x3 (default case), each inside its clipped triangle.
                const unsigned fam = (family & 0xf0u) >> 4, low = family & 0x0fu;
                const bool sloped = fam >= 2u && fam <= 5u && low >= 1u && low <= 4u;
                if ((family & 0xf0u) >= 0x20u && !sloped) {
                    if (!diagonal_excluded(*idx, group, family, bounds, x, z)) ++out.unsupported_primitives;
                    continue;
                }
                for (uint32_t pi = 0; pi < group.primitive_count; ++pi) {
                    const uint64_t primitive_index = uint64_t(group.primitive_first) + pi;
                    if (primitive_index >= map.contact_primitives.size()) { ++out.unsupported_primitives; continue; }
                    const auto& primitive = map.contact_primitives[static_cast<size_t>(primitive_index)];
                    if (!allowed(primitive, work.object_class)) continue;
                    if (sloped && primitive.code != family) continue;
                    const auto rect = rectangle(*idx, group, primitive, bounds);
                    if (!rect.ordered_unwrapped) { ++out.unsupported_primitives; continue; }
                    // Sloped diagonals test their clipped triangle inside 0x8004A060.
                    if (!sloped && !inside_xz(rect, x, z)) continue;
                    const unsigned c = primitive.code;
                    if (!sloped && c != 1u && c != 3u && c != 15u && (c < 8u || c > 11u) &&
                        (c < 0x10u || c > 0x13u) && (c < 0x1Cu || c > 0x1Fu)) {
                        ++out.unsupported_primitives; continue;
                    }
                    const auto interval = runtime_contact_floor_interval_8004a060(*idx, group, primitive, bounds, x, z);
                    if (!interval) { if (!sloped) ++out.unsupported_primitives; continue; }
                    const int32_t compare = dy < 0 ? y : std::min<int32_t>(y, best);
                    if (interval->y0 < best && compare < interval->y1) {
                        best = interval->y0; out.cell_index = cell.cell_index;
                        out.primitive_index = static_cast<uint32_t>(primitive_index);
                    }
                }
            }
        }
        if (best == previous) break;
        if (pass == 3) {
            out.status = RuntimeContactStatus::IterationLimit;
            out.cell_index.reset(); out.primitive_index.reset();
            return out;
        }
    }
    if (out.unsupported_primitives) {
        out.status = RuntimeContactStatus::Unsupported; out.cell_index.reset(); out.primitive_index.reset();
    } else out.result = best;
    return out;
}
} // namespace l3

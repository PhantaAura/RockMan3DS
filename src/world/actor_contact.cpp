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
bool allowed(const RuntimeContactPrimitive& primitive, uint8_t object_class) noexcept {
    return l3_obj_group_bits_8002fe74(object_class) == 0 || !(primitive.flags & 0x8000u);
}
bool eligible(int16_t correction, int16_t delta, uint8_t blocked,
              uint8_t positive, uint8_t negative) noexcept {
    return delta != 0 && std::abs(int(correction)) <= std::abs(int(delta)) &&
           !(blocked & (correction > 0 ? positive : negative));
}
bool respond_rectangle(const Rect& rect, L3RetailContactScratch& point,
                       int16_t dx, int16_t dy, int16_t dz, uint8_t& blocked,
                       ActorWallContact& out) noexcept {
    const int16_t cx = wrap((dx < 0 ? rect.x1 : rect.x0) - int32_t(point.x));
    const int16_t cy = wrap((dy < 0 ? rect.y1 : rect.y0) - int32_t(point.y));
    const int16_t cz = wrap((dz < 0 ? rect.z1 : rect.z0) - int32_t(point.z));
    const bool ex = eligible(cx, dx, blocked, 2, 1), ez = eligible(cz, dz, blocked, 8, 4);
    const bool ey = eligible(cy, dy, blocked, 32, 16);
    if (!ex && !ey && !ez) { ++out.unsupported_primitives; return false; }
    int axis = !ex ? (ez ? 2 : 1) : (!ez ? 0 : (std::abs(int(cx)) < std::abs(int(cz)) ? 0 : 2));
    const int horizontal = axis == 0 ? std::abs(int(cx)) : std::abs(int(cz));
    if (ey && ((!ex && !ez) || ((ex && ez) ? std::abs(int(cy)) < horizontal :
                                                       std::abs(int(cy)) <= horizontal))) axis = 1;
    if (axis == 0) { point.x = wrap(int32_t(point.x) + cx); blocked = cx > 0 ? 1 : 2; }
    else if (axis == 2) { point.z = wrap(int32_t(point.z) + cz); blocked = cz > 0 ? 4 : 8; }
    else {
        point.y = wrap(int32_t(point.y) + cy); blocked = cy > 0 ? 16 : 32;
        if (cy <= 0) { point.contact_state = 0; return true; }
    }
    return false;
}
} // namespace

ActorWallContact query_actor_wall_contact_8004b6ac(
    const RuntimeMap& map, size_t area, const ActorContactWork& work, RetailPlayerBounds bounds) noexcept {
    ActorWallContact out;
    const L3RetailContactScratch requested{work.raw[0], work.raw[1], work.raw[2], work.state74};
    out.scratch = requested;
    const auto candidates = gather(map, area, requested.x, requested.z);
    out.candidate_cells = candidates.count;
    if (candidates.malformed) return out;
    if (!candidates.count) { out.status = RuntimeContactStatus::NoData; return out; }
    out.status = RuntimeContactStatus::SourceBackedPartial;
    uint8_t blocked = 0;
    for (unsigned pass = 0; pass < 9; ++pass) {
        out.iterations = static_cast<uint8_t>(pass + 1);
        bool repeat = false;
        for (uint8_t reverse = candidates.count; reverse > 0 && !repeat; --reverse) {
            const auto& cell = candidates.cells[reverse - 1];
            const int32_t vx = int32_t(out.scratch.x) - work.previous_raw[0];
            const int32_t vy = int32_t(out.scratch.y) - work.previous_raw[1];
            const int32_t vz = int32_t(out.scratch.z) - work.previous_raw[2];
            if (!fits_short(vx) || !fits_short(vy) || !fits_short(vz)) { ++out.unsupported_primitives; continue; }
            const auto dx = wrap(vx), dy = wrap(vy), dz = wrap(vz);
            if (cell.packed_cell < 0) {
                const auto containing = runtime_map_find_cell(map, area, out.scratch.x, out.scratch.z);
                if (!containing || containing->cell_index != cell.cell_index) continue;
                const int32_t top = -int32_t((uint16_t(cell.packed_cell) & 0x1f00u) >> 1);
                const int32_t floor = top - 1 - bounds.y1;
                if (!fits_short(floor)) { ++out.unsupported_primitives; continue; }
                if (floor < out.scratch.y) {
                    out.scratch.y = wrap(floor);
                    if (dy > 0) out.scratch.contact_state = 0;
                    repeat = true;
                } else if (uint16_t(cell.packed_cell) & 0x6000u) {
                    const int32_t ceiling = top - int32_t((uint16_t(cell.packed_cell) & 0x6000u) >> 13) * 384 + 1 - bounds.y0;
                    if (!fits_short(ceiling)) { ++out.unsupported_primitives; continue; }
                    if (out.scratch.y < ceiling) { out.scratch.y = wrap(ceiling); repeat = true; }
                }
                continue;
            }
            const auto* idx = runtime_map_cell_idx(map, cell);
            const auto* list = runtime_map_cell_contact_list(map, cell);
            if (!idx || !list) { ++out.unsupported_primitives; continue; }
            if (uint64_t(list->group_first) + list->group_count > map.contact_groups.size()) {
                ++out.unsupported_primitives; continue;
            }
            for (uint32_t gi = 0; gi < list->group_count && !repeat; ++gi) {
                const uint64_t index = uint64_t(list->group_first) + gi;
                if (index >= map.contact_groups.size()) { ++out.unsupported_primitives; continue; }
                const auto& group = map.contact_groups[static_cast<size_t>(index)];
                ++out.groups_tested;
                if (!group.primitive_count) continue;
                if (uint64_t(group.primitive_first) + group.primitive_count > map.contact_primitives.size()) {
                    ++out.unsupported_primitives; continue;
                }
                const auto family = map.contact_primitives[group.primitive_first].code;
                if (family >= 256u) continue;
                if ((family & 0xf0u) >= 0x20u) {
                    if (!diagonal_excluded(*idx, group, family, bounds, out.scratch.x, out.scratch.z)) ++out.unsupported_primitives;
                    continue;
                }
                for (uint32_t pi = 0; pi < group.primitive_count && !repeat; ++pi) {
                    const uint64_t primitive_index = uint64_t(group.primitive_first) + pi;
                    if (primitive_index >= map.contact_primitives.size()) { ++out.unsupported_primitives; continue; }
                    const auto& primitive = map.contact_primitives[static_cast<size_t>(primitive_index)];
                    ++out.primitives_tested;
                    if (!allowed(primitive, work.object_class)) continue;
                    const auto rect = rectangle(*idx, group, primitive, bounds);
                    if (!rect.ordered_unwrapped) { ++out.unsupported_primitives; continue; }
                    if (!inside_xz(rect, out.scratch.x, out.scratch.z)) continue;
                    if (primitive.code != 1u && primitive.code != 3u && primitive.code != 15u) { ++out.unsupported_primitives; continue; }
                    if (!(rect.y0 < out.scratch.y && out.scratch.y < rect.y1)) continue;
                    if (primitive.code == 3u) {
                        if (dy >= 0 || out.scratch.contact_state == 0) {
                            out.scratch.y = rect.y0; out.scratch.contact_state = 0; repeat = true;
                        } else ++out.unsupported_primitives;
                    } else if (primitive.code == 15u) { out.scratch.y = rect.y1; repeat = true; }
                    else if (out.scratch.contact_state == 0 && int32_t(out.scratch.y) - rect.y0 < 8 && !blocked) {
                        out.scratch.y = rect.y0; repeat = true;
                    } else repeat = respond_rectangle(rect, out.scratch, dx, dy, dz, blocked, out);
                }
            }
        }
        if (!repeat) break;
        if (pass == 8) { out.status = RuntimeContactStatus::IterationLimit; return out; }
    }
    if (out.unsupported_primitives) { out.status = RuntimeContactStatus::Unsupported; return out; }
    if (!fits_short(int32_t(requested.x) - out.scratch.x) ||
        !fits_short(int32_t(requested.y) - out.scratch.y) ||
        !fits_short(int32_t(requested.z) - out.scratch.z)) {
        out.status = RuntimeContactStatus::Unsupported; return out;
    }
    out.result = l3_collision_no_contact_result_8004bc2c(requested.x, requested.y, requested.z,
                                                       out.scratch.x, out.scratch.y, out.scratch.z);
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
                if ((family & 0xf0u) >= 0x20u) {
                    if (!diagonal_excluded(*idx, group, family, bounds, x, z)) ++out.unsupported_primitives;
                    continue;
                }
                for (uint32_t pi = 0; pi < group.primitive_count; ++pi) {
                    const uint64_t primitive_index = uint64_t(group.primitive_first) + pi;
                    if (primitive_index >= map.contact_primitives.size()) { ++out.unsupported_primitives; continue; }
                    const auto& primitive = map.contact_primitives[static_cast<size_t>(primitive_index)];
                    if (!allowed(primitive, work.object_class)) continue;
                    const auto rect = rectangle(*idx, group, primitive, bounds);
                    if (!rect.ordered_unwrapped) { ++out.unsupported_primitives; continue; }
                    if (!inside_xz(rect, x, z)) continue;
                    if (primitive.code != 1u && primitive.code != 3u && primitive.code != 15u) {
                        ++out.unsupported_primitives; continue;
                    }
                    const auto interval = runtime_contact_floor_interval_8004a060(*idx, group, primitive, bounds, x, z);
                    if (!interval) { ++out.unsupported_primitives; continue; }
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

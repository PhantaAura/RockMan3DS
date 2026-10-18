#include "legends3ds/actor_action3_contact.h"
#include <limits>

namespace l3 {
namespace {
int16_t decrement(int16_t value) noexcept {
    const uint16_t word = static_cast<uint16_t>(static_cast<uint16_t>(value) - 1u);
    return word < 0x8000u ? static_cast<int16_t>(word)
        : static_cast<int16_t>(static_cast<int32_t>(word) - 65536);
}
int32_t signed_word(uint32_t value) noexcept {
    return value <= INT32_MAX ? static_cast<int32_t>(value)
        : static_cast<int32_t>(int64_t(value) - 0x100000000LL);
}
bool all_neighbor_samples_admitted(const RuntimeMap& map, size_t area_index,
                                    int16_t x, int16_t z) noexcept {
    if (area_index >= map.areas.size()) return false;
    const auto& area = map.areas[area_index];
    const int32_t nx = (uint16_t(x) & 511u) < 257u ? -512 : 512;
    const int32_t nz = (uint16_t(z) & 511u) < 257u ? -512 : 512;
    const std::array<std::array<int32_t, 2>, 4> delta{{{{0, 0}}, {{nx, 0}}, {{0, nz}}, {{nx, nz}}}};
    const auto shift9 = [](int32_t v) { return v >= 0 ? v / 512 : -((-v + 511) / 512); };
    for (const auto& offset : delta) {
        const auto gx = shift9(int32_t(x) + offset[0] + 32768) - area.start_x;
        const auto gz = shift9(int32_t(z) + offset[1] + 32768) - area.start_z;
        if (gx < 0 || gz < 0 || gx >= area.width || gz >= area.height) return false;
        const uint64_t local = uint64_t(gz) * area.width + uint64_t(gx);
        if (local >= area.cell_count || uint64_t(area.cell_first) + local >= map.cells.size()) return false;
    }
    return true;
}
}
ActorAction3ContactPreparation prepare_actor_action3_contact(
    const Class20_03CallbackState& state, const ActorContactWork& motion,
    const RuntimeMap& map, size_t area, const ReferenceTables& references,
    RetailPlayerBounds bounds, std::optional<uint16_t> target) noexcept {
    ActorAction3ContactPreparation out;
    const auto& b = state.behavior;
    out.before = motion; out.after_wall = motion;
    out.initial_action = b.action_09; out.initial_phase = b.phase_0a;
    out.initial_timer = b.timer_32c; out.initial_work64 = state.work_64;
    out.initial_packet = state.packet_33c; out.initial_heading = b.heading_56;
    out.target_heading = target;
    if (b.action_09 != 3 || (b.phase_0a != 3 && b.phase_0a != 4) ||
        decrement(b.timer_32c) < 0 || (state.packet_33c & 0x02000000u)) return out;
    if (motion.object_class != 0x20 || motion.raw != state.raw_14 || motion.fixed != state.fixed_1c ||
        motion.previous_raw != state.previous_raw_2c || motion.previous_fixed != state.previous_fixed_34) {
        out.status = ActorAction3ContactStatus::StaleWork; return out;
    }
    std::optional<int16_t> limit = state.work_64;
    if (b.phase_0a == 3 && b.timer_32c < 4) limit = 256;
    if (!limit) { out.status = ActorAction3ContactStatus::NeedsTurnLimit; return out; }
    if (!target) { out.status = ActorAction3ContactStatus::NeedsTargetHeading; return out; }
    out.effective_turn_limit = *limit;
    out.source_turn = class20_03_turn_step(*target, b.heading_56, *limit);
    const auto heading = static_cast<uint16_t>((int32_t(b.heading_56) + out.source_turn) & 0xfff);
    const auto delta = reference_player_motion_delta(references, heading, 768, 0, 0);
    const std::array<int32_t, 3> values{{delta.dx_fixed, delta.dy_fixed, delta.dz_fixed}};
    for (size_t axis = 0; axis < values.size(); ++axis) {
        out.after_wall.fixed[axis] = signed_word(uint32_t(out.after_wall.fixed[axis]) + uint32_t(values[axis]));
        out.after_wall.raw[axis] = reference_fixed_to_short_80030150(out.after_wall.fixed[axis]);
    }
    if (!all_neighbor_samples_admitted(map, area, out.after_wall.raw[0], out.after_wall.raw[2])) {
        out.status = ActorAction3ContactStatus::UnsupportedBoundary; return out;
    }
    out.wall = query_actor_wall_contact_8004b6ac(map, area, out.after_wall, bounds);
    if (!apply_actor_contact_result_8004b414(out.after_wall, out.wall)) {
        out.status = ActorAction3ContactStatus::UnsupportedWall; return out;
    }
    if (!all_neighbor_samples_admitted(map, area, out.after_wall.raw[0], out.after_wall.raw[2])) {
        out.status = ActorAction3ContactStatus::UnsupportedBoundary; return out;
    }
    out.floor = query_actor_floor_contact_80048ea4(map, area, out.after_wall, bounds);
    if (out.floor.status != RuntimeContactStatus::SourceBackedPartial) {
        out.status = ActorAction3ContactStatus::UnsupportedFloor; return out;
    }
    Class20_03PreparedAction3 input;
    input.raw_14 = out.after_wall.raw; input.fixed_1c = out.after_wall.fixed;
    input.source_floor_word = out.floor.result; input.source_contact_word = uint32_t(out.wall.result);
    input.heading_after_turn = heading;
    out.inputs = input;
    out.status = ActorAction3ContactStatus::SourceBackedPartial;
    return out;
}
std::array<uint32_t, 2> actor_source_sound_position_words(
    const std::array<int16_t, 4>& raw) noexcept {
    return {{uint32_t(uint16_t(raw[0])) | (uint32_t(uint16_t(raw[1])) << 16),
             uint32_t(uint16_t(raw[2])) | (uint32_t(uint16_t(raw[3])) << 16)}};
}
} // namespace l3

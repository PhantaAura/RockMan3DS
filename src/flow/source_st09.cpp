#include "legends3ds/source_st09.h"

namespace l3 {

St09TrappedManTick step_st09_trapped_man_801080b8(St09TrappedMan& m, const St09TrappedManInputs& in) noexcept {
    St09TrappedManTick out;
    if (m.action_09 != 0) { out.action_unported = true; return out; }
    if (m.phase_0a == 0) {
        m.phase_0a = 1; m.request_ac = 20; m.timer_34c = 60; out.request = 20;
    } else if (m.phase_0a != 1) return out;
    const auto tile = [](int16_t v) { return int32_t(v) >> 9; };
    bool inside_any = false;
    for (const auto& e : in.enemies) {
        if (uint8_t(tile(e[0]) - in.box[0]) < in.box[2] && uint8_t(tile(e[1]) - in.box[1]) < in.box[3]) {
            inside_any = true; break;
        }
    }
    out.enemies_in_box = inside_any;
    if (inside_any) m.flags_348 &= uint16_t(~8u); else m.flags_348 |= 8u;
    if (m.timer_34c != 0) --m.timer_34c;
    const uint32_t dx = uint32_t(tile(in.player_x) - int32_t(int8_t(in.box[0])));
    const uint32_t dz = uint32_t(tile(in.player_z) - int32_t(int8_t(in.box[1])));
    if (in.player_targets_me) {
        out.clear_player_target = true;
        if (m.flags_348 & 8u) {
            if (!in.window_open) { out.flag_set = 0x60C; m.heading_56 = 1492; }
        } else if (!in.window_open) {
            m.timer_34c = 450; out.message = uint8_t(m.message_0c + 4);
        }
    } else if (in.hit) {
        if (!in.window_open) { m.timer_34c = 450; out.message = uint8_t(m.message_0c + 5); }
    } else if (dx < in.box[2] && dz < in.box[3]) {
        if (!(m.flags_348 & 4u)) {
            m.flags_348 |= 4u; out.flag_clears = {0x60A, 0x60B}; out.message = m.message_0c;
        }
    } else if (m.timer_34c == 0) {
        m.timer_34c = 450; m.flags_348 &= uint16_t(~4u); out.message = m.message_0c;
    }
    return out;
}

bool bind_st09_profile(const std::vector<StageDataBlock>& blocks, St03Event8Profile& out) {
    out = {};
    for (const auto& d : blocks)
        if (d.token == kSt09TrappedManBox && d.bytes.size() >= 4)
            out.st09_trapped_man_box = std::array<uint8_t,4>{{d.bytes[0], d.bytes[1], d.bytes[2], d.bytes[3]}};
    for (const auto& d : blocks) {
        if (d.token != kSt09Set0Record || d.bytes.size() < 20) continue;
        const auto& r = d.bytes;
        const auto s16 = [&](size_t o) { return int16_t(uint16_t(r[o] | (r[o + 1] << 8))); };
        St03ObjectRecord g;
        g.slot = r[0]; g.object_class = r[1]; g.subtype = r[2]; g.family = r[3]; g.byte_04 = r[4];
        g.word_08 = {r[8], r[9], r[10], r[11]};
        g.position = {s16(12), s16(14), s16(16)};
        g.heading = uint16_t(r[18] | (r[19] << 8));
        out.object_sets[0] = {g};
        out.generic_classes = St03GenericClasses{};   // no generic NPCs in ST09's set 0
        return true;
    }
    return false;
}

} // namespace l3

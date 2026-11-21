#include "legends3ds/source_st09.h"
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/reference_tables.h"

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

St09TrappedManStep step_st09_trapped_man_80107e54(St09TrappedMan& m, St03Npc& n, const St09TrappedManWorldInputs& in) noexcept {
    St09TrappedManStep out;
    auto& w = n.w;
    if (m.action_09 == 0) {
        out.action0 = step_st09_trapped_man_801080b8(m, in.action0);
        if (out.action0.flag_set) out.flag_set = out.action0.flag_set;
    } else if (m.action_09 == 1) {                       // 0x80108384
        if (m.phase_0a == 0) {
            if (in.frame_a4 == 15 || in.frame_a4 == 35) {
                m.phase_0a = 1; m.request_ac = 21; out.talk = uint8_t(m.message_0c + 6);
            }
        } else if (m.phase_0a == 1 && !in.action0.window_open) m.flags_348 |= 1u;
    } else if (m.action_09 == 2) {                       // 0x80108430
        if (m.phase_0a == 0) {
            m.phase_0a = 1; m.request_ac = 2; m.speed_44 = 384; m.vy_46 = 384; m.accel_4e = 192;
            m.timer_34c = 8; out.flag_set = 0x60D;
        } else if (m.phase_0a == 1) {
            if (!in.refs) out.refs_missing = true;
            else {
                const auto h = source_heading_80031198(w.raw_14[0], w.raw_14[2], kSt09TrappedManExit[0], kSt09TrappedManExit[1], in.angle_steps);
                if (h) w.rot_54[1] = uint16_t((w.rot_54[1] + class20_03_turn_step(uint16_t(*h), w.rot_54[1], 96)) & 0xFFFu);
                else out.refs_missing = true;
                // 0x8002FEA4(work, +0x44, +0x46, 0): vertical 0 here (no 0x8004B414 floor pass; CANDIDATE).
                const auto d = reference_player_motion_delta(*in.refs, w.rot_54[1], m.speed_44, 0, 0);
                const std::array<int32_t,3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
                for (size_t a = 0; a < 3; ++a) {
                    w.fixed_1c[a] = int32_t(uint32_t(w.fixed_1c[a]) + uint32_t(delta[a]));
                    w.raw_14[a] = reference_fixed_to_short_80030150(w.fixed_1c[a]);
                }
                out.moved = true;
            }
            if (m.timer_34c != 0) { if (--m.timer_34c == 0) out.dust = true; }
            else out.dust = true;
            const int32_t dx = int32_t(w.raw_14[0]) - kSt09TrappedManExit[0], dz = int32_t(w.raw_14[2]) - kSt09TrappedManExit[1];
            if (dx * dx + dz * dz < 2048) m.flags_348 |= 1u;
            if (in.anim_a6 == 1) out.sound = 278;
        }
    }
    if (m.flags_348 & 1u) {                              // 0x80107F04
        if (m.action_09 == 0) m.action_09 = 1;
        else if (m.action_09 == 1) m.action_09 = 2;
        else if (m.action_09 == 2) { out.released = true; out.flag_set = 12; }
        m.phase_0a = 0;
        m.flags_348 &= uint16_t(~1u);
    }
    return out;
}

namespace {
constexpr std::array<int16_t,3> kSt09RescuePoint{{22626, -577, 7563}};   // 0x58620000, 0xFDBF0000, 0x1D8B0000
}

St03Event8Step step_st09_event17_80100788(St03Event8& ev, const St03Event8Profile& p, const St09Event17Inputs& in) noexcept {
    St03Event8Step out;
    if (!ev.active) return out;
    ++ev.ticks;
    if (!p.st09_rescue_loaded || p.st09_rescue.records.empty()) { out.unsupported_scene = true; return out; }
    bool flag12_check = false;
    switch (ev.state) {
    case 0:
        if (!in.player_ready) break;
        ev.counters.e8 = ev.counters.e9 = 0;            // 0x8001F1DC
        ev.counters.c10 = ev.counters.c18 = ev.counters.c1c = 0;
        out.camera_reset_8001e6c0 = true;
        ev.camera_program = &p.st09_rescue_camera; ev.camera = {};
        ev.timeline = &p.st09_rescue; ev.record = 0;
        { St03PlayerTarget t; t.position = kSt09RescuePoint; t.walk = true; t.heading = 3540; out.player = t; }
        ev.countdown = 30; ++ev.state;
        break;
    case 1:
        if (--ev.countdown != 0) break;
        ev.countdown = 36; ++ev.state;
        out.objects[0].state_09 = 1;                     // *(0x8011EA60) +9 = 1, +0x0A = 0
        break;
    case 2:
        if (in.flag_60d) {
            if (ev.countdown == 36) out.player_anim_448 = 13;
            if (ev.countdown != 0 && --ev.countdown != 0) out.player_heading_add = in.turn_delta;
            else { out.player_anim_448 = 48; ++ev.state; }
        }
        flag12_check = true;
        break;
    case 3: flag12_check = true; break;
    default: break;
    }
    if (flag12_check && in.flag_12) {
        out.player_release = true; ev.active = false; out.ended = true;
        return out;
    }
    // 0x801009F8: states 1..2 run the timeline record, then the skip.
    const bool mid = ev.state != 0 && ev.state < 3;
    const auto at_end = [&] { return !ev.timeline || ev.record >= ev.timeline->records.size() || uint8_t(ev.timeline->records[ev.record][0]) == 0xFFu; };
    if (mid && !at_end() && ev.timeline->records[ev.record][1] == kSt09RescueRecordCallback) {
        if (ev.camera_program) step_source_camera_program_8001ec0c(ev.camera, *ev.camera_program, ev.counters);
        step_source_timeline_8001f070(ev.counters, ev.record, *ev.timeline);
    }
    if (mid && in.start_pressed) {
        out.release_set = true; out.skipped = true; out.flag_set = 12; ev.state = 3;
        St03PlayerTarget t; t.position = kSt09RescuePoint; t.heading = 2401; out.player = t;
        out.player_anim_448 = 48;
    }
    return out;
}

bool bind_st09_event17(const SourceScenePackage& package, St03Event8Profile& out) {
    if (!package.loaded || package.event != 0x17 || package.stage != 9) return false;
    const auto* t = package.timeline(kSt09RescueTimeline);
    const auto* c = package.camera(kSt09RescueCamera);
    if (!t || !c || t->records.empty()) return false;
    out.st09_rescue = *t; out.st09_rescue_camera = *c; out.st09_rescue_loaded = true;
    return true;
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

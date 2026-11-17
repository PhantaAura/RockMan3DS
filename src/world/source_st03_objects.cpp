#include "legends3ds/source_st03_objects.h"
#include "legends3ds/actor_animation_bank.h"
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_angle.h"

namespace l3 {
namespace {

uint32_t u32le(const std::vector<uint8_t>& b, size_t o) {
    return uint32_t(b[o]) | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) | (uint32_t(b[o + 3]) << 24);
}
int16_t s16le(const uint8_t* p) { return int16_t(uint16_t(p[0] | (p[1] << 8))); }

// A byte range [token, token + n) inside one cooked data block.
const uint8_t* bytes_at(const SourceScenePackage& pkg, uint32_t token, size_t n) {
    for (const auto& d : pkg.data)
        if (token >= d.token && token - d.token + n <= d.bytes.size()) return d.bytes.data() + (token - d.token);
    return nullptr;
}

void init_80107bc0(St03Npc& n) noexcept {
    auto& w = n.w;
    w.flags_00 = uint8_t(w.flags_00 | 2u);
    switch (n.object_class) {
    case 0x26: w.word_158 = 0x01D701D7u; w.word_15c = 471; break;
    case 0x24: case 0x30: case 0x42: case 0x53: case 0x5C: case 0x44: w.word_158 = 0x021A021Au; w.word_15c = 538; break;
    case 0x5E: w.word_158 = 0x020001C0u; w.word_15c = 384; break;
    default: w.word_158 = 0x02000200u; w.word_15c = 512; break;
    }
    w.colour_15e = 0x4210;
    n.flags_348 = 2; n.timer_350 = 0;
    n.radius_368 = (w.byte_0d & 0x10u) ? 10000 : 5000;
    n.home_354 = w.raw_14; n.heading_352 = w.rot_54[1];
    w.rot_54[0] = w.rot_54[2] = 0; w.rot_44[0] = w.rot_44[1] = 0; w.word_78 = 0;
    w.state_08 = 1; w.sub_0a = 0; w.byte_0b = 0;
    w.state_09 = uint8_t(w.byte_0d & 7u);
    w.pose_ac = 0;   // 0x800322A8(work, 0, 0)
}

// 0x8010A4EC(work, forward, limit): the player within `limit` of the point
// 0x80030058 puts `forward << 4` ahead of the NPC (x/z high halves).
bool in_range_8010a4ec(const St03Npc& n, int16_t px, int16_t pz, int16_t off_x, int16_t off_z, int32_t limit) noexcept {
    const int32_t lim = int32_t(uint16_t(limit)), twice = lim * 2;
    const int32_t dx = int32_t(uint16_t(n.w.raw_14[0] + off_x - px + lim));
    const int32_t dz = int32_t(uint16_t(n.w.raw_14[2] + off_z - pz + lim));
    return dx < twice && dz < twice;
}
bool in_range_8010a4ec(const St03Npc& n, int16_t px, int16_t pz) noexcept {
    return in_range_8010a4ec(n, px, pz, 0, 0, n.radius_368);   // offset 0: the rotated vector is zero
}

void behaviour0_80107f5c(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (w.sub_0a) {   // 0x80116E58
    case 0: {
        if (!in.rand || !in.idle_timers) { out.rand_missing = true; return; }
        w.pose_ac = st03_npc_idle_request(n.object_class);
        out.rand_used = true;
        const size_t i = size_t(n.byte_0e & 3u) * 16u + (*in.rand & 15u);
        n.timer_350 = int16_t((*in.idle_timers)[i]);
        ++w.sub_0a;
        return;
    }
    case 1:
        if (--n.timer_350 != 0) return;
        if (n.object_class == 73) n.timer_350 = 120;
        if (n.object_class == 0x53) {
            if (!in.rand) { out.rand_missing = true; ++n.timer_350; return; }
            out.rand_used = true;
        }
        w.pose_ac = st03_npc_fidget_request(n.object_class, in.rand.value_or(0));
        ++w.sub_0a;
        return;
    case 2:
        if (n.timer_350 != 0 && --n.timer_350 != 0) return;
        if (!in.animation_finished) return;
        if (n.object_class == 73) { w.pose_ac = 9; ++w.sub_0a; } else w.sub_0a = 0;
        return;
    case 3:
        if (in.animation_finished) w.sub_0a = 0;
        return;
    default: return;
    }
}

// 0x80031BEC's clamp part (as class20_03_turn_step) toward the player.
int16_t turn_to_player(const St03Npc& n, const St03NpcInputs& in, int16_t limit) noexcept {
    if (!in.heading_to_player) return 0;
    return class20_03_turn_step(uint16_t(*in.heading_to_player), n.w.rot_54[1], limit);
}
void add_heading(St03Npc& n, int16_t d) noexcept { n.w.rot_54[1] = uint16_t((n.w.rot_54[1] + uint16_t(d)) & 0xFFFu); }

// 0x801096AC: whether the saved request lets the NPC return at once.
bool may_return_801096ac(const St03Npc& n) noexcept {
    const uint8_t a = n.w.sub_0a;
    if (n.saved_34a == 1) return a >= 4 && (a < 6 || a == 8);
    if (n.saved_34a == 3) return a >= 4 && a < 6;
    return false;
}

void end_talk(St03Npc& n) noexcept {
    n.talk_34b = 3; n.flags_348 = uint16_t(n.flags_348 & ~4u);
    n.w.pose_ac = st03_npc_idle_request(n.object_class);
}

// Behaviour 4 (0x8010940C, sub-state +0x34B via 0x8011700C): the talk.
void behaviour4_8010940c(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (n.talk_34b) {
    case 0: {   // 0x80109448
        ++n.talk_34b; n.saved_heading_34c = w.rot_54[1]; n.saved_34a = w.pose_ac;
        w.pose_ac = st03_npc_idle_request(n.object_class);
        const int16_t d = turn_to_player(n, in, 2048);
        n.turn_step_34e = int16_t(d < 0 ? (uint16_t(-d) >> 3) : (d >> 3));
        out.talk_start = w.byte_0c;   // 0x80063BC8(npc + 0x14, 0, 0, +0x0C)
        return;
    }
    case 1:     // 0x80109534
        if (!(w.byte_0d & 0x40u)) add_heading(n, turn_to_player(n, in, n.turn_step_34e));
        if (in.window_open) { w.pose_ac = st03_npc_fidget_request(n.object_class, in.rand.value_or(0)); ++n.talk_34b; }
        if (!in.talk_active) end_talk(n);
        return;
    case 2:     // 0x80109608
        add_heading(n, turn_to_player(n, in, n.turn_step_34e));
        if (in.animation_finished) { w.pose_ac = st03_npc_idle_request(n.object_class); --n.talk_34b; }
        if (!in.talk_active) end_talk(n);
        return;
    case 3: {   // 0x80109730: back to the saved heading (0x80031D5C as the same clamp step)
        const int16_t d = class20_03_turn_step(n.saved_heading_34c, w.rot_54[1], n.turn_step_34e);
        if (d == 0 || (w.byte_0d & 0x80u) || may_return_801096ac(n)) {
            w.pose_ac = n.saved_34a; n.flags_348 = uint16_t(n.flags_348 | 2u);
            w.state_09 = uint8_t(w.byte_0d & 7u);
        } else add_heading(n, d);
        return;
    }
    default: return;
    }
}


// 0x80031D5C(target, heading, step): the signed turn toward target, clamped to step.
int16_t turn_80031d5c(uint16_t target, uint16_t heading, int16_t step) noexcept {
    int32_t a1 = int32_t(target) - int32_t(heading);
    if (int16_t(a1) < 0) a1 += 4096;
    const int16_t a0 = int16_t(a1);
    int32_t v = step;
    if (a0 < 2048) { if (a0 < step) v = a1; }
    else if (!(4096 - a0 < step)) v = -step; else v = a1 - 4096;
    return int16_t(v);
}

// Behaviour 7 (0x80109CF8, sub-state +0x34B via 0x80117040): Data's talk and menu
// (docs/research_01516/DATA_MENU.md). The event-0xFF camera (0x80039FAC / 0x80039E18,
// gated by 0x800A3962) and the player's talk pose are not ported: the camera is
// treated as settled at once (CANDIDATE).
void behaviour7_80109cf8(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (n.talk_34b) {
    case 0: {   // 0x80109D34
        n.saved_heading_34c = w.rot_54[1]; n.saved_34a = w.pose_ac;
        w.pose_ac = st03_npc_idle_request(n.object_class);
        const int16_t d = turn_to_player(n, in, 0x800);
        n.turn_step_34e = int16_t(d < 0 ? (uint16_t(-d) >> 3) : (d >> 3));
        ++n.talk_34b;
        return;
    }
    case 1:     // 0x80109E94
        out.talk_start = w.byte_0c;   // 0x80063BC8(npc + 0x14, 0xD, 0, +0x0C)
        ++n.talk_34b;
        add_heading(n, turn_to_player(n, in, n.turn_step_34e));
        return;
    case 2:     // 0x80109F1C
        add_heading(n, turn_to_player(n, in, n.turn_step_34e));
        if (in.talk_active) { if (in.save_phase) ++n.talk_34b; }
        else {
            n.flags_348 = uint16_t(n.flags_348 & ~4u);
            n.talk_34b = uint8_t(n.talk_34b + 2);
            w.pose_ac = st03_npc_idle_request(n.object_class);
        }
        return;
    case 3:     // 0x80109FD4
        if (!in.save_phase) { out.reopen_menu = 0xDC; --n.talk_34b; }
        return;
    case 4: {   // 0x8010A02C
        const int16_t d = turn_80031d5c(n.saved_heading_34c, w.rot_54[1], n.turn_step_34e);
        if (d == 0 || (w.byte_0d & 0x80u)) {   // (camera settled, see above)
            w.pose_ac = n.saved_34a; w.state_09 = uint8_t(w.byte_0d & 7u);
            n.flags_348 = uint16_t(n.flags_348 | 2u);
        }
        add_heading(n, d);
        return;
    }
    default: return;
    }
}

// 0x8010A45C(class): the walk request.
uint8_t walk_request_8010a45c(uint8_t c) noexcept {
    if (c == 57) return 0;
    if (c < 57) return 1;
    return (c >= 76 && c < 78) ? 0 : 1;
}

// 0x8010A49C(work, mode): walk speed (+0x44).
int16_t walk_speed_8010a49c(const St03Npc& n, uint8_t mode, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    if (mode == 1) {
        if (!in.walk_speeds) { out.table_missing = true; return 0; }
        return int16_t((*in.walk_speeds)[n.object_class < in.walk_speeds->size() ? n.object_class : 0]);
    }
    return n.object_class == 42 ? 64 : n.object_class == 5 ? 320 : 384;
}

// The behaviour-1 range check: 64 ahead (forward 1024), limit 100.
bool ahead_in_range(const St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    if (!in.refs) { out.refs_missing = true; return false; }
    const auto d = reference_player_motion_delta(*in.refs, n.w.rot_54[1], int16_t(64 << 4), 0, 0);
    return in_range_8010a4ec(n, in.player_x, in.player_z, int16_t(d.dx_fixed >> 16), int16_t(d.dz_fixed >> 16), 100);
}

// 0x8002FEA4(work, +0x44, 0, 0).
void walk_forward(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    if (!in.refs) { out.refs_missing = true; return; }
    auto& w = n.w;
    const auto d = reference_player_motion_delta(*in.refs, w.rot_54[1], n.speed_44, 0, 0);
    const std::array<int32_t,3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
    for (size_t a = 0; a < 3; ++a) {
        w.fixed_1c[a] = int32_t(uint32_t(w.fixed_1c[a]) + uint32_t(delta[a]));
        w.raw_14[a] = reference_fixed_to_short_80030150(w.fixed_1c[a]);
    }
    out.moved = true;
}

uint16_t walk_heading(const St03Npc& n) noexcept {
    return n.walk_dir_35c ? n.heading_352 : uint16_t((n.heading_352 + 2048) & 0xFFFu);
}

// Behaviour 1 (0x80108100, sub-state +0x0A via 0x80116EA8): walk out along
// the home heading and back, with stops, fidgets and the player-ahead stop.
void behaviour1_80108100(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (w.sub_0a) {
    case 0:   // 0x8010813C
        n.walk_dir_35c = 0; n.leg_len_366 = int16_t(n.byte_0e << 3); n.leg_left_35e = n.leg_len_366;
        n.speed_44 = walk_speed_8010a49c(n, 1, in, out);
        ++w.sub_0a;
        return;
    case 1: {   // 0x80108194: pick walk / idle / turn back, then a timer
        if (!in.rand || !in.rand2 || !in.walk_tables) { out.rand_missing = !in.rand || !in.rand2; out.table_missing |= !in.walk_tables; return; }
        out.rand_used = out.rand2_used = true;
        const auto& t = *in.walk_tables;   // 0x80116ED4: choices [0..63], timers [64..111]
        uint8_t choice = t[(w.byte_0d & 0x30u) + (*in.rand & 15u)];
        size_t timers = 64;   // 0x80116F14
        if (choice == 0) { w.pose_ac = walk_request_8010a45c(n.object_class); n.speed_44 = walk_speed_8010a49c(n, 1, in, out); }
        else if (choice == 1) { w.pose_ac = st03_npc_idle_request(n.object_class); timers = 80; }
        else if (n.leg_left_35e == n.leg_len_366) choice = 255;
        else { w.pose_ac = walk_request_8010a45c(n.object_class); n.target_heading_64 = walk_heading(n); timers = 96; }
        n.timer_350 = int16_t(t[timers + (*in.rand2 & 15u)]);
        if (choice != 255) w.sub_0a = uint8_t(w.sub_0a + choice + 1);
        return;
    }
    case 2:   // 0x801082F8: walk
        walk_forward(n, in, out);
        if (--n.leg_left_35e == 0) {
            n.target_heading_64 = uint16_t((n.walk_dir_35c ? (n.byte_0f & 15u) : (n.byte_0f >> 4)) << 8);
            w.sub_0a = 5;
        } else if (--n.timer_350 == 0) w.sub_0a = 1;
        else if (ahead_in_range(n, in, out)) { w.pose_ac = st03_npc_idle_request(n.object_class); n.timer_350 = 60; w.sub_0a = 9; }
        return;
    case 3:   // 0x801083DC
        if (--n.timer_350 == 0) w.sub_0a = 1;
        return;
    case 4: {   // 0x80108404: turn back, swap direction
        const int16_t d = turn_80031d5c(n.target_heading_64, w.rot_54[1], 64);
        if (d == 0) { w.sub_0a = 1; n.walk_dir_35c = int16_t(1 - n.walk_dir_35c); n.leg_left_35e = int16_t(n.leg_len_366 - n.leg_left_35e); }
        add_heading(n, d);
        return;
    }
    case 5: {   // 0x80108480: turn to the end heading
        const int16_t d = turn_80031d5c(n.target_heading_64, w.rot_54[1], 64);
        if (d == 0) { n.timer_350 = 20; w.pose_ac = st03_npc_idle_request(n.object_class); ++w.sub_0a; }
        add_heading(n, d);
        return;
    }
    case 6:   // 0x801084FC
        if (--n.timer_350 == 0) {
            if (n.object_class == 0x53) { if (!in.rand) { out.rand_missing = true; ++n.timer_350; return; } out.rand_used = true; }
            w.pose_ac = st03_npc_fidget_request(n.object_class, in.rand.value_or(0));
            n.timer_350 = 70; ++w.sub_0a;
        }
        return;
    case 7:   // 0x80108560
        if (in.animation_finished && n.object_class != 73) w.pose_ac = st03_npc_idle_request(n.object_class);
        if (--n.timer_350 == 0) {
            if (n.object_class == 73) { w.pose_ac = 9; w.sub_0a = 10; }
            else { w.pose_ac = walk_request_8010a45c(n.object_class); ++w.sub_0a; }
            n.target_heading_64 = walk_heading(n);
            n.leg_left_35e = n.leg_len_366;
        }
        return;
    case 8: {   // 0x80108648: turn for the next leg; snap home after the return leg
        const int16_t d = turn_80031d5c(n.target_heading_64, w.rot_54[1], 64);
        add_heading(n, d);
        if (d != 0) return;
        w.sub_0a = 1;
        if (n.walk_dir_35c != 0) {
            w.raw_14 = n.home_354;
            for (size_t a = 0; a < 3; ++a) w.fixed_1c[a] = int32_t(uint32_t(uint16_t(w.raw_14[a])) << 16);
            w.rot_54[1] = n.heading_352;
        }
        n.walk_dir_35c = int16_t(1 - n.walk_dir_35c);
        return;
    }
    case 9:   // 0x80108704: stopped for the player ahead
        if (--n.timer_350 != 0) return;
        if (!ahead_in_range(n, in, out)) { w.sub_0a = 1; return; }
        if (n.leg_left_35e < n.leg_len_366 - 5) {
            w.pose_ac = walk_request_8010a45c(n.object_class); w.sub_0a = 4;
            n.target_heading_64 = uint16_t((w.rot_54[1] + 2048) & 0xFFFu);
        } else n.timer_350 = 60;
        return;
    case 10:   // 0x801087A8 (class 73)
        if (in.animation_finished) { w.sub_0a = 8; w.pose_ac = walk_request_8010a45c(n.object_class); }
        return;
    default: return;
    }
}

// 0x80031BEC(work + 0x14, target, heading, limit): the clamped turn toward a point.
int16_t turn_toward(const St03Npc& n, int16_t tx, int16_t tz, int16_t limit, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    const auto h = source_heading_80031198(n.w.raw_14[0], n.w.raw_14[2], tx, tz, in.angle_steps);
    if (!h) { out.refs_missing = true; return 0; }
    return class20_03_turn_step(uint16_t(*h), n.w.rot_54[1], limit);
}
bool window_open(const St03NpcInputs& in) noexcept { return in.window_open; }   // 0x80098A58 & 0x00200000

// Shared pieces of the variant-1 scene NPCs (Barrell 0x8010D0F4, the
// Inspector 0x8010D968, the officer 0x8010C66C; behaviours by +9 from
// 0x80117110 / 0x8011714C / 0x801170EC, sub-state +0x0A).
void v1_stand(St03Npc& n) noexcept {   // 0x8010D3E4 / 0x8010DC54 / 0x8010C954
    if (n.w.sub_0a == 0) { n.w.sub_0a = 1; n.w.pose_ac = 0; }
}
void v1_gesture(St03Npc& n, const St03NpcInputs& in) noexcept {   // 0x8010D730 / 0x8010E014 / 0x8010CAB8
    auto& w = n.w;
    if (w.sub_0a == 0) { w.sub_0a = 1; w.pose_ac = 0; }
    if (w.sub_0a == 1) { if (window_open(in)) { w.pose_ac = 8; w.sub_0a = 2; } }
    else if (w.sub_0a == 2 && in.animation_finished) { w.sub_0a = 1; w.pose_ac = 0; }
}
// Gesture at sub-states `at` / `at + 1` (the tail of the Inspector's walk).
void v1_gesture_at(St03Npc& n, const St03NpcInputs& in, uint8_t at) noexcept {
    auto& w = n.w;
    if (w.sub_0a == at) { if (window_open(in)) { ++w.sub_0a; w.pose_ac = 8; } }
    else if (w.sub_0a == at + 1 && in.animation_finished) { --w.sub_0a; w.pose_ac = 0; }
}
// The walk toward +0x354 (turn by +0x64, 0x8002FEA4 at +0x44), flag 0x606
// within distance^2 2048. Returns true on arrival.
bool v1_walk_to_home(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    add_heading(n, turn_toward(n, n.home_354[0], n.home_354[2], int16_t(n.target_heading_64), in, out));
    walk_forward(n, in, out);
    const int32_t dx = int32_t(w.raw_14[0]) - n.home_354[0], dz = int32_t(w.raw_14[2]) - n.home_354[2];
    if (dx * dx + dz * dz >= 2048) return false;
    ++w.sub_0a; w.pose_ac = 0; n.speed_44 = 0; n.target_heading_64 = 0; out.flag_set = 0x606;
    return true;
}
// Barrell / officer behaviour 1: walk to the +0x0C object (0x8010D5EC / 0x8010C974).
void v1_walk_to_id(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out, int16_t speed) noexcept {
    auto& w = n.w;
    if (w.sub_0a == 0) {
        if (!in.id_target) { out.id_target_missing = true; return; }
        w.sub_0a = 1; w.pose_ac = 1; n.speed_44 = speed; n.target_heading_64 = 32;
        n.home_354 = *in.id_target;
    } else if (w.sub_0a != 1) return;
    v1_walk_to_home(n, in, out);
}

// Barrell variant 1 behaviours (0x80117110).
void barrell1_behaviour(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (w.state_09) {
    case 0: v1_stand(n); return;
    case 1: v1_walk_to_id(n, in, out, 48); return;   // sound 269 on +0xA6 == 1 (not ported)
    case 2:   // 0x8010D404: face the +0x0C object
        if (w.sub_0a > 1) return;
        if (w.sub_0a == 0) { ++w.sub_0a; w.pose_ac = 0; n.target_heading_64 = 32; }
        if (!in.id_target) { out.id_target_missing = true; return; }
        n.home_354 = *in.id_target;
        add_heading(n, turn_toward(n, n.home_354[0], n.home_354[2], int16_t(n.target_heading_64), in, out));
        return;
    case 3: v1_gesture(n, in); return;
    case 4:   // 0x8010D4C0: face the player, then gesture while a window is open
        switch (w.sub_0a) {
        case 0: w.sub_0a = 1; w.pose_ac = 0; n.target_heading_64 = 32; [[fallthrough]];
        case 1: {
            n.home_354 = {in.player_x, n.home_354[1], in.player_z};
            const int16_t d = turn_toward(n, in.player_x, in.player_z, int16_t(n.target_heading_64), in, out);
            add_heading(n, d);
            if (d != 0) return;
            w.pose_ac = 0; ++w.sub_0a;
            [[fallthrough]];
        }
        case 2: case 3: v1_gesture_at(n, in, 2); return;
        default: return;
        }
    case 5:   // 0x8010D7C4: wait for the player's check; flag 0x604; face the player
        switch (w.sub_0a) {
        case 0: {
            if (!in.player_targets_me) return;
            out.flag_set = 0x604; out.clear_player_target = true;
            w.byte_0b = 16; w.pose_ac = 0; ++w.sub_0a;
            const int16_t d = turn_toward(n, in.player_x, in.player_z, 2048, in, out);
            n.target_heading_64 = uint16_t((d < 0 ? -int32_t(d) : int32_t(d)) >> 3);
            n.speed_44 = 0;
            return;
        }
        case 1: case 2:
            add_heading(n, turn_toward(n, in.player_x, in.player_z, int16_t(n.target_heading_64), in, out));
            v1_gesture_at(n, in, 1);
            return;
        default: return;
        }
    default: out.behaviour_unported = true; return;
    }
}

// The Inspector's variant 1 behaviours (0x8011714C).
void inspector1_behaviour(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (w.state_09) {
    case 0: v1_stand(n); return;
    case 1:   // 0x8010DC74: walk to (-3003, -1, 1339), then gesture (sound 268 not ported)
        if (w.sub_0a == 0) {
            w.sub_0a = 1; w.pose_ac = 1; n.speed_44 = 60; n.target_heading_64 = 32;
            n.home_354 = {-3003, -1, 1339};
        }
        if (w.sub_0a == 1) { v1_walk_to_home(n, in, out); return; }
        v1_gesture_at(n, in, 2);
        return;
    case 2:   // 0x8010DE18: face the +0x0C object (0x606), then walk to the +0x0D object (0x606)
        switch (w.sub_0a) {
        case 0:
            if (!in.id_target) { out.id_target_missing = true; return; }
            w.pose_ac = 1; n.speed_44 = 60; n.target_heading_64 = 32; ++w.sub_0a;
            n.home_354 = *in.id_target;
            [[fallthrough]];
        case 1: {
            const int16_t d = turn_toward(n, n.home_354[0], n.home_354[2], int16_t(n.target_heading_64), in, out);
            add_heading(n, d);
            if (d == 0) { out.flag_set = 0x606; ++w.sub_0a; }
            return;
        }
        case 2:
            if (!in.id_target2) { out.id_target_missing = true; return; }
            ++w.sub_0a; n.home_354 = *in.id_target2;
            [[fallthrough]];
        case 3: v1_walk_to_home(n, in, out); return;
        default: return;
        }
    case 3: v1_gesture(n, in); return;
    default: out.behaviour_unported = true; return;
    }
}

// The officer's variant 1 behaviours (0x801170EC).
void officer1_behaviour(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    switch (n.w.state_09) {
    case 0: v1_stand(n); return;
    case 1: v1_walk_to_id(n, in, out, 44); return;   // sound 266 not ported
    case 2: v1_gesture(n, in); return;
    default: out.behaviour_unported = true; return;
    }
}

// 0x8010D130 / 0x8010D9A4 / 0x8010C6A8 (state 0): init; the tick follows the same frame.
void scene_v1_init(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    w.flags_00 = uint8_t(w.flags_00 | 2u);
    const uint16_t scale = n.object_class == 0x26 ? 471 : n.object_class == 0x24 ? 538 : 512;
    w.word_158 = (uint32_t(scale) << 16) | scale; w.word_15c = scale; w.colour_15e = 0x4210;
    n.flags_348 = 0;
    // 0x8002FFC4: the point 256 ahead of the spawn position.
    n.home_354 = w.raw_14;
    if (in.refs) {
        const auto d = reference_player_motion_delta(*in.refs, w.rot_54[1], 256, 0, 0);
        const std::array<int32_t,3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
        for (size_t a = 0; a < 3; ++a)
            n.home_354[a] = reference_fixed_to_short_80030150(int32_t(uint32_t(w.fixed_1c[a]) + uint32_t(delta[a])));
    } else out.refs_missing = true;
    w.rot_54[0] = w.rot_54[2] = 0; w.rot_44[0] = w.rot_44[1] = 0; n.speed_44 = 0;
    w.state_08 = 1; w.sub_0a = 0; w.byte_0b = 0;
    w.state_09 = n.object_class == 0x25 ? (w.byte_03 ? 5 : 0) : n.object_class == 0x26 ? 3 : 0;
    w.pose_ac = 0;   // 0x800322A8(work, 0, 0)
}

St03NpcTick step_scene_v1(St03Npc& n, const St03NpcInputs& in) noexcept {
    St03NpcTick out;
    auto& w = n.w;
    if (w.state_08 == 0) { scene_v1_init(n, in, out); out.anim_request = 0; }
    if (w.state_08 != 1) return out;
    out.in_range = true;
    n.pose_ad = w.pose_ac;
    w.colour_15e = 0x4210;
    out.rand_used = true;   // +0x34A = 0x8007FE50() every tick
    if (n.object_class == 0x25) barrell1_behaviour(n, in, out);
    else if (n.object_class == 0x26) inspector1_behaviour(n, in, out);
    else officer1_behaviour(n, in, out);
    if (n.flags_348 & 1u) {   // restart: Barrell keeps +9, the others go to 0
        if (n.object_class != 0x25) w.state_09 = 0;
        w.sub_0a = 0; w.byte_0b = 0; n.flags_348 = uint16_t(n.flags_348 & ~1u);
    }
    // 0x80051B68(work, 3, -1, 0): a check candidate only with a +0x10 box
    // (Barrell's 0x80117134; the others store 0). CANDIDATE: kind 3 checks.
    out.checkable = n.object_class == 0x25;
    if (n.pose_ad != w.pose_ac) out.anim_request = w.pose_ac; else if (!out.anim_request) out.anim_advance = true;
    return out;
}

// The police car 0x801104C0 (family 0x60, class 0x0E; states 0x80117254,
// behaviours 0x80117260).
void car_init_801105b8(St03Npc& n) noexcept {
    auto& w = n.w;
    w.rot_54 = {0, uint16_t((uint32_t(w.byte_0c) << 9) & 0xE00u), 0};
    w.rot_44 = {0, 0, 0}; n.speed_44 = 0; n.strafe_48 = 0;
    w.flags_00 = uint8_t(w.flags_00 | 4u);
    n.car_restart = false; n.car_timer_c0 = 0;
    w.word_158 = 0x02000200u; w.word_15c = 512;
    w.state_08 = 1; w.state_09 = 0; w.sub_0a = 0; w.byte_0b = 0;
}
void car_drive_801107fc(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    const bool back = (n.byte_0f & 0x40u) != 0;
    if (w.sub_0a == 0) {
        w.sub_0a = 1; n.car_timer_c0 = 30; w.flags_00 = uint8_t(w.flags_00 | 2u);
        if (back) { n.car_last = 4; n.speed_44 = 0; } else { n.car_last = 5; n.speed_44 = 576; }
        n.accel_4c = 22; n.strafe_48 = 0; n.car_index_c4 = 1;
    } else if (w.sub_0a != 1) return;
    if (!in.car_paths) { out.table_missing = true; return; }
    const size_t base = (back ? 48u : 0u) + size_t(n.car_index_c4) * 8u;
    if (base + 8 > in.car_paths->size()) { out.table_missing = true; return; }
    const auto* e = in.car_paths->data() + base;
    const int16_t ex = s16le(e), ez = s16le(e + 2), ew = s16le(e + 4), eh = s16le(e + 6);
    const int16_t d = turn_toward(n, ex, ez, 32, in, out);
    n.strafe_48 = int16_t(d * 4);
    w.rot_54[2] = uint16_t((uint32_t(int32_t(d)) << 19) >> 20);
    add_heading(n, d);
    if (in.refs) {
        const auto m = reference_player_motion_delta(*in.refs, w.rot_54[1], n.speed_44, 0, n.strafe_48);
        const std::array<int32_t,3> delta{{m.dx_fixed, m.dy_fixed, m.dz_fixed}};
        for (size_t a = 0; a < 3; ++a) {
            w.fixed_1c[a] = int32_t(uint32_t(w.fixed_1c[a]) + uint32_t(delta[a]));
            w.raw_14[a] = reference_fixed_to_short_80030150(w.fixed_1c[a]);
        }
        out.moved = true;
    } else out.refs_missing = true;
    const uint32_t bx = uint32_t(int32_t(w.raw_14[0]) - (int32_t(ex) - (ew >> 1)));
    const uint32_t bz = uint32_t(int32_t(w.raw_14[2]) - (int32_t(ez) - (eh >> 1)));
    if (bx < uint32_t(int32_t(ew)) && bz < uint32_t(int32_t(eh))) {
        if (n.car_index_c4 == n.car_last) {
            n.car_restart = true; n.speed_44 = 0; n.strafe_48 = 0; w.rot_54[0] = 0; w.rot_54[2] = 0;
        }
        ++n.car_index_c4;
    }
    const int32_t dx = int16_t(ex - w.raw_14[0]), dz = int16_t(ez - w.raw_14[2]);
    if (dx * dx + dz * dz > 16384) { if (n.speed_44 < 800) n.speed_44 = int16_t(n.speed_44 + n.accel_4c); }
    else if (n.speed_44 >= 353) n.speed_44 = int16_t(n.speed_44 - n.accel_4c);
    out.car_dust = true;   // 0x80031618 x 4 every 4th frame (not ported)
}
St03NpcTick step_car(St03Npc& n, const St03NpcInputs& in) noexcept {
    St03NpcTick out;
    auto& w = n.w;
    out.rand_used = true;   // +0xBE = 0x8007FE50()
    if (w.state_08 == 0) { car_init_801105b8(n); return out; }
    if (w.state_08 != 1) return out;
    out.in_range = true;
    if (in.flag4) w.flags_00 = uint8_t(w.flags_00 | 2u);
    if (w.state_09 == 0) {   // 0x80110788: idle; restarts every 30 frames
        if (w.sub_0a == 0) { w.sub_0a = 1; n.car_timer_c0 = 30; n.speed_44 = 0; n.strafe_48 = 0; }
        else if (w.sub_0a == 1 && --n.car_timer_c0 < 0) n.car_restart = true;
    } else if (w.state_09 == 1) car_drive_801107fc(n, in, out);
    else out.behaviour_unported = true;
    if (n.car_restart) { w.state_09 = 0; w.sub_0a = 0; w.byte_0b = 0; n.car_restart = false; }
    return out;
}

} // namespace

std::vector<St03ObjectRecord> st03_object_set(const SourceScenePackage& package, uint8_t n) {
    std::vector<St03ObjectRecord> out;
    const uint8_t* t = bytes_at(package, kSt03ObjectSetTable + 8u * n, 8);
    if (!t) return out;
    const uint32_t count = uint32_t(t[0] | (t[1] << 8) | (t[2] << 16) | (uint32_t(t[3]) << 24));
    const uint32_t at = uint32_t(t[4] | (t[5] << 8) | (t[6] << 16) | (uint32_t(t[7]) << 24));
    if (count > kSt03ObjectSlots) return out;
    const uint8_t* r = bytes_at(package, at, size_t(count) * 20u);
    if (!r && count) return out;
    for (uint32_t i = 0; i < count; ++i, r += 20) {
        St03ObjectRecord o;
        o.slot = r[0]; o.object_class = r[1]; o.subtype = r[2]; o.family = r[3]; o.byte_04 = r[4];
        o.word_08 = {r[8], r[9], r[10], r[11]};
        o.position = {s16le(r + 12), s16le(r + 14), s16le(r + 16)};
        o.heading = uint16_t(r[18] | (r[19] << 8));
        out.push_back(o);
    }
    (void)u32le;
    return out;
}

bool st03_generic_npc_class(uint8_t c, uint8_t variant) noexcept {
    return variant == 0 && (c == 0x05 || c == 0x25 || c == 0x11 || c == 0x26 || c == 0x24);
}

void st03_object_set_variant(St03Npc& n, uint8_t variant) noexcept {
    n.variant = variant;
    n.generic = n.family == 0x20 && st03_generic_npc_class(n.object_class, variant);
    n.scene_v1 = n.family == 0x20 && variant == 1 && (n.object_class == 0x25 || n.object_class == 0x26 || n.object_class == 0x24);
    n.check_box = n.scene_v1 && n.object_class == 0x25 ? kSt03Barrell1CheckBox : kSt03NpcCheckBox;
    n.car = n.family == 0x60 && n.object_class == 0x0E;
}

St03Npc spawn_st03_object(const St03ObjectRecord& r) {
    St03Npc n;
    auto& w = n.w;
    w.live = true; w.slot = r.slot; w.subtype = r.object_class;
    n.object_class = r.object_class; n.family = r.family;
    w.byte_03 = r.byte_04;
    w.byte_0c = r.word_08[0]; w.byte_0d = r.word_08[1]; n.byte_0e = r.word_08[2]; n.byte_0f = r.word_08[3];
    for (size_t a = 0; a < 3; ++a) { w.raw_14[a] = r.position[a]; w.fixed_1c[a] = int32_t(uint32_t(uint16_t(r.position[a])) << 16); }
    w.rot_54[1] = r.heading;
    w.state_08 = 0;
    st03_object_set_variant(n, r.subtype);   // work +4 = record [2]
    return n;
}

uint8_t st03_npc_idle_request(uint8_t c) noexcept { return c == 0x11 ? 1 : 0; }

uint8_t st03_npc_fidget_request(uint8_t c, uint16_t rand) noexcept {
    if (c == 42) return 9;
    if (c < 43) { if (c == 0) return 0; if (c == 17) return 1; return 8; }
    if (c == 80) return 0;
    if (c < 81) return c == 57 ? 0 : 8;
    if (c == 83) return uint8_t((rand & 1u) | 8u);
    return 8;
}

St03NpcTick step_st03_npc(St03Npc& n, const St03NpcInputs& in) noexcept {
    St03NpcTick out;
    auto& w = n.w;
    if (w.live && n.scene_v1) return step_scene_v1(n, in);
    if (w.live && n.car) return step_car(n, in);
    if (!w.live || !n.generic) return out;
    bool fresh = false;
    if (w.state_08 == 0) { init_80107bc0(n); fresh = true; out.anim_request = 0; }
    if (w.state_08 != 1) return out;
    if (!in_range_8010a4ec(n, in.player_x, in.player_z)) return out;   // the init request stays
    out.in_range = true;
    n.pose_ad = w.pose_ac;   // +0xAD = +0xAC
    w.colour_15e = 0x4210;
    if ((n.flags_348 & 2u) && in.player_targets_me) {   // 0x8010A344
        const uint8_t c = n.object_class;
        w.state_09 = c == 52 ? 6 : c == 17 ? 7 : c == 73 ? 8 : 4;
        n.talk_34b = 0; n.flags_348 = uint16_t((n.flags_348 | 4u) & ~2u);
        out.talk_target_taken = true;
    }
    if (w.state_09 == 0) behaviour0_80107f5c(n, in, out);
    else if (w.state_09 == 1) behaviour1_80108100(n, in, out);
    else if (w.state_09 == 4) behaviour4_8010940c(n, in, out);
    else if (w.state_09 == 7) behaviour7_80109cf8(n, in, out);
    else if (w.state_09 >= 6) out.talk_unported = true;
    else out.behaviour_unported = true;
    if (!(w.byte_0d & 0x20u) && in.floor_y) { w.raw_14[1] = *in.floor_y; w.fixed_1c[1] = int32_t(uint32_t(uint16_t(*in.floor_y)) << 16); }
    out.checkable = (n.flags_348 & 2u) != 0;   // 0x80051B68(work, +0x348 & 2 ? 1 : 3, -1, 0)
    // +0xAD (saved before the behaviour) against +0xAC.
    if (n.pose_ad != w.pose_ac) out.anim_request = w.pose_ac; else out.anim_advance = true;
    (void)fresh;
    return out;
}

St02ActorModel decode_st03_npc_model(const EbdDirectory& ebd, uint8_t object_class, size_t cap) noexcept {
    // The generic NPC requests 0 / 1 (idle) and 8 / 9 (fidget); a row whose
    // bank lacks some keeps the ones it has (request 0 at least).
    static const uint8_t all[] = {0, 1, 8, 9};
    St02ActorModel out;
    try {
        const auto rows = find_ebd_entries(ebd, 0x20, object_class);
        if (rows.empty() || ebd.source_payload.empty()) return out;
        auto mesh = decode_ebd_actor_mesh(ebd, rows[0], ActorMeshTier::NEAR, cap);
        const size_t mesh_bytes = mesh.footprint.decoded_bytes;
        if (mesh_bytes > cap) return out;
        std::vector<uint8_t> have;
        for (const uint8_t r : all) {
            const uint8_t one[] = {r};
            try { auto b = decode_ebd_actor_animation_bank(ebd, rows[0], mesh, one, 1, cap - mesh_bytes);
                  if (b.requests.size() == 1) have.push_back(r); } catch (...) {}
        }
        if (have.empty()) return out;
        auto bank = decode_ebd_actor_animation_bank(ebd, rows[0], mesh, have.data(), have.size(), cap - mesh_bytes);
        if (bank.requests.size() != have.size()) return out;
        out.decoded_bytes = mesh_bytes + bank.decoded_bytes;
        out.mesh = std::make_shared<const ActorMeshResource>(std::move(mesh));
        out.bank = std::make_shared<const ActorAnimationBank>(std::move(bank));
    } catch (...) { return St02ActorModel{}; }
    return out;
}

} // namespace l3

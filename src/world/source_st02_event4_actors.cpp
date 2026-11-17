#include "legends3ds/source_st02_event4_actors.h"

namespace l3 {
namespace {

int32_t s32(uint32_t v) noexcept { return static_cast<int32_t>(v); }
int32_t add(int32_t a, uint32_t b) noexcept { return s32(uint32_t(a) + b); }

// The overlay's float steps: __floatsidf (0x80069630), __adddf3 (0x80069038),
// __fixdfsi (0x80069530, truncating). The constants are the doubles the
// instructions load (a2 = low word, a3 = high word).
int32_t dadd(int32_t v, double c) noexcept { return static_cast<int32_t>(static_cast<double>(v) + c); }

void to_short(St02EventActorWork& w) noexcept {   // 0x80030150(+0x1C, +0x14)
    for (size_t a = 0; a < 3; ++a) w.raw_14[a] = reference_fixed_to_short_80030150(w.fixed_1c[a]);
}
void copy_rotation(St02EventActorWork& w) noexcept {   // lh +0x46/+0x4A/+0x4E -> +0x54..+0x58
    for (size_t a = 0; a < 3; ++a) w.rot_54[a] = static_cast<uint16_t>(uint32_t(w.rot_44[a]) >> 16);
}
void effect(St02EventActorTick& t, uint32_t fn, int16_t a1, int16_t a2, int16_t a3, int32_t a4) noexcept {
    if (t.effect_count < t.effects.size()) t.effects[t.effect_count++] = {fn, a1, a2, a3, a4};
}
// Decrements +0x0C and advances +0x0A when it reaches zero.
bool count_down(St02EventActorWork& w) noexcept {
    w.byte_0c = uint8_t(w.byte_0c - 1);
    if (w.byte_0c != 0) return false;
    ++w.sub_0a;
    return true;
}
void smoke_pair(St02EventActorTick& t, uint16_t frame) noexcept {   // (1F800002 & 3) == 3
    if ((frame & 3u) != 3u) return;
    effect(t, 0x80105724u, 400, -100, 0, 1);
    effect(t, 0x80105724u, -400, -100, 200, 1);
}

// ---- 20/0A ----
void flutter_state1(St02EventActorWork& w) noexcept {   // 0x801021D8, table 0x801074C0
    switch (w.sub_0a) {
    case 0:   // 0x80102080
        w.rot_44 = {{s32(0xFF9C0000u), s32(0xF4000000u), 0x01000000}};
        w.byte_0c = 0xB4; ++w.sub_0a;
        break;
    case 1:   // 0x801020B0
        w.fixed_1c[0] = dadd(w.fixed_1c[0], -233016.888918016);
        w.rot_44[0] = dadd(w.rot_44[0], 36408.88918016);
        w.rot_44[1] = dadd(w.rot_44[1], 279620.26622976);
        w.rot_44[2] = dadd(w.rot_44[2], -93206.755540992);
        count_down(w);
        break;
    default: break;   // 0x801021D0
    }
    copy_rotation(w); to_short(w);
}
void flutter_state2(St02EventActorWork& w) noexcept {   // 0x80102278, table 0x801074CC
    if (w.sub_0a == 0) ++w.sub_0a;                                   // 0x80102244
    else if (w.sub_0a == 1) w.fixed_1c[2] = add(w.fixed_1c[2], 0xFFF36886u);   // 0x80102258
    to_short(w);
}
bool flutter_state3(St02EventActorWork& w, const ReferenceTables* refs) noexcept {   // 0x80102338, table 0x801074D8
    if (w.sub_0a == 0) {             // 0x801022CC
        w.byte_0c = 75; w.rot_54 = {{0, 0, 0}}; ++w.sub_0a;
    } else if (w.sub_0a == 1) {      // 0x801022F0
        count_down(w);
        w.rot_54[1] = uint16_t(w.rot_54[1] + 5);
    }
    if (!refs) return false;
    // 0x8002FEA4(work, 256, 0, 0): heading-relative fixed add, then 0x80030150.
    const auto d = reference_player_motion_delta(*refs, w.rot_54[1], 256, 0, 0);
    w.fixed_1c[0] = add(w.fixed_1c[0], uint32_t(d.dx_fixed));
    w.fixed_1c[1] = add(w.fixed_1c[1], uint32_t(d.dy_fixed));
    w.fixed_1c[2] = add(w.fixed_1c[2], uint32_t(d.dz_fixed));
    to_short(w);
    return true;
}
void flutter_state4(St02EventActorWork& w, St02EventActorTick& t, uint16_t frame) noexcept {   // 0x80102458, table 0x801074E4
    switch (w.sub_0a) {
    case 0:   // 0x80102394
        w.rot_44 = {{0, 0x04000000, s32(0xFF000000u)}}; w.byte_0c = 1; ++w.sub_0a;
        break;
    case 1:   // 0x801023C0
        if (count_down(w)) w.byte_0c = 45;
        break;
    case 2:   // 0x801023F4
        if (count_down(w)) w.byte_0c = 45;
        w.rot_44[1] = add(w.rot_44[1], 0xFFE82223u);
        w.rot_44[2] = add(w.rot_44[2], 0x00111111u);
        break;
    default: break;   // 0x80102450
    }
    copy_rotation(w); to_short(w);
    smoke_pair(t, frame);
}
void flutter_state5(St02EventActorWork& w, St02EventActorTick& t, uint16_t frame) noexcept {   // 0x801026E8, table 0x801074F4
    switch (w.sub_0a) {
    case 0:   // 0x80102514
        w.rot_44 = {{0, 0x08000000, 0}}; w.byte_0c = 0; ++w.sub_0a;
        break;
    case 1: {  // 0x80102538
        w.byte_0c = uint8_t(w.byte_0c + 1);
        const int8_t c = int8_t(w.byte_0c);
        if (c >= 70 && (w.byte_0c & 1u)) {
            effect(t, 0x80105724u, 0, 500, 400, 0);
            effect(t, 0x80105724u, -100, 500, 400, 0);
            effect(t, 0x80105724u, 100, 500, 400, 0);
        }
        if (c == 90) effect(t, 0x80105C28u, 0, 500, 600, 0);
        if (c == 100) { w.byte_0c = 50; ++w.sub_0a; }
        w.fixed_1c[2] = add(w.fixed_1c[2], 0x00A00000u);
        w.fixed_1c[1] = add(w.fixed_1c[1], 0x000A0000u);
        break;
    }
    case 2:   // 0x8010262C
        count_down(w);
        w.fixed_1c[2] = add(w.fixed_1c[2], 0x00A00000u);
        if (w.byte_0c & 1u) {
            effect(t, 0x80105724u, 0, 500, 300, 0);
            effect(t, 0x80105724u, -100, 500, 300, 0);
            effect(t, 0x80105724u, 100, 500, 300, 0);
        }
        break;
    default: break;   // 0x801026E0
    }
    smoke_pair(t, frame);
    copy_rotation(w); to_short(w);
}
bool flutter_state6(St02EventActorWork& w, const ReferenceTables* refs) noexcept {   // 0x801027A8
    if (w.sub_0a == 0) { w.byte_0c = 0; ++w.sub_0a; to_short(w); return true; }
    w.byte_0c = uint8_t(w.byte_0c + 1);
    if (!refs || !refs->scalar_quarter_known) return false;
    // rsin(sign-extended +0x0C << 6) * 800 into +0x20.
    const int32_t s = reference_rsin_8007a060(*refs, int32_t(int8_t(w.byte_0c)) * 64);
    w.fixed_1c[1] = s32(uint32_t(s) * 800u);
    to_short(w);
    return true;
}

void step_flutter(St02EventActorWork& w, const ReferenceTables* refs, uint16_t frame, St02EventActorTick& t) noexcept {
    if (w.state_08 == 0) {   // 0x80101FD8
        t.model_init = true;
        w.word_158 = 0x02000200u; w.word_15c = 0x21080200u; w.word_78 = 0;
        t.pose_restart = true; ++w.state_08;
        return;
    }
    // 0x80102830: +9 through 0x80107504, then 0x80032538 and 0x800334C4.
    switch (w.state_09) {
    case 0: to_short(w); ++t.sequence_steps; ++t.draw_submits; break;   // 0x80102040
    case 1: flutter_state1(w); break;
    case 2: flutter_state2(w); break;
    case 3: if (!flutter_state3(w, refs)) t.status = St02ActorStatus::NoReferenceTables; break;
    case 4: flutter_state4(w, t, frame); break;
    case 5: flutter_state5(w, t, frame); break;
    case 6: if (!flutter_state6(w, refs)) t.status = St02ActorStatus::NoReferenceTables; break;
    default: t.status = St02ActorStatus::UnknownState; return;
    }
    ++t.sequence_steps; ++t.draw_submits;
}

// ---- 20/27 ----
void pose_on_change(St02EventActorWork& w, St02EventActorTick& t) noexcept {   // 0x80102914
    if (int8_t(w.byte_0c) != int8_t(w.byte_0d)) {
        t.model_init = true; t.model_variant = w.byte_0c; t.pose_restart = true;
    }
    w.byte_0d = w.byte_0c;
}
void step_27(St02EventActorWork& w, St02EventActorTick& t) noexcept {
    switch (w.state_08) {   // table 0x80107520
    case 0:   // 0x801028C8
        w.rot_54 = {{0, 0, 0}};
        w.word_158 = 0x02000200u; w.word_15c = 0x42100200u;
        ++w.state_08;
        pose_on_change(w, t);
        break;
    case 1: pose_on_change(w, t); break;
    case 2: w.released = true; w.live = false; return;   // 0x80102970 -> 0x80031824
    default: t.status = St02ActorStatus::UnknownState; return;
    }
    to_short(w); ++t.sequence_steps; ++t.draw_submits;
}

} // namespace

St02EventActorWork spawn_st02_event4_actor(const std::array<uint8_t,20>& r) noexcept {
    St02EventActorWork w;
    w.live = true; w.slot = r[0]; w.subtype = r[1];
    w.byte_0c = r[8]; w.byte_0d = r[9];
    for (size_t a = 0; a < 3; ++a) {
        const int16_t v = int16_t(uint16_t(r[12 + 2 * a] | (r[13 + 2 * a] << 8)));
        w.raw_14[a] = v; w.fixed_1c[a] = s32(uint32_t(uint16_t(v)) << 16);   // CANDIDATE: 0x8001E968 short -> 16.16
    }
    w.rot_54[1] = uint16_t(r[18] | (r[19] << 8));
    return w;
}

void apply_st02_event4_slot_writes(St02EventActorWork& w, SourceSceneSlotCommands& s) noexcept {
    w.flags_00 = uint8_t((w.flags_00 & ~2u) | (s.flags_00 & 2u));
    if (s.state_written) { w.state_09 = s.state_09; w.sub_0a = s.sub_0a; s.state_written = false; }
    if (s.fixed_1c) { w.fixed_1c[0] = *s.fixed_1c; s.fixed_1c.reset(); }
    if (s.fixed_20) { w.fixed_1c[1] = *s.fixed_20; s.fixed_20.reset(); }
    if (s.fixed_24) { w.fixed_1c[2] = *s.fixed_24; s.fixed_24.reset(); }
    if (s.heading_56) { w.rot_54[1] = *s.heading_56; s.heading_56.reset(); }
    if (s.colour_15e) w.colour_15e = *s.colour_15e;
}

St02EventActorTick step_st02_event4_actor(St02EventActorWork& w, const ReferenceTables* refs,
                                          uint16_t frame_1f800002) noexcept {
    St02EventActorTick t;
    if (!w.live) return t;
    if (w.subtype == 0x0A) step_flutter(w, refs, frame_1f800002, t);
    else if (w.subtype == 0x27) step_27(w, t);
    else t.status = St02ActorStatus::UnknownState;   // 40/05..07: next slice
    return t;
}

} // namespace l3

// ST02 event 4 family-0x20 actors: 20/0A 0x80102888 and 20/27 0x80102990.
#include "legends3ds/source_st02_event4_actors.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace l3;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

static std::array<uint8_t,20> record(uint8_t slot, uint8_t subtype, uint8_t b8, int16_t x, int16_t y, int16_t z) {
    std::array<uint8_t,20> r{};
    r[0] = slot; r[1] = subtype; r[2] = 4; r[3] = 0x20; r[8] = b8;
    const int16_t p[3] = {x, y, z};
    for (int a = 0; a < 3; ++a) { r[12 + 2 * a] = uint8_t(uint16_t(p[a])); r[13 + 2 * a] = uint8_t(uint16_t(p[a]) >> 8); }
    return r;
}
static void set_state(SourceSceneSlotCommands& s, uint8_t st) { s.state_09 = st; s.sub_0a = 0; s.state_written = true; }

static ReferenceTables synthetic_refs() {
    ReferenceTables r;
    for (size_t i = 0; i < kReferenceTrigEntries; ++i) {
        const double a = double(i) * 2.0 * M_PI / 4096.0;
        r.trig[i] = {int16_t(std::lround(std::sin(a) * 4096)), int16_t(std::lround(std::cos(a) * 4096))};
    }
    for (size_t i = 0; i < kReferenceQuarterEntries; ++i)
        r.scalar_quarter[i] = int16_t(std::lround(std::sin(double(i) * M_PI / 2048.0) * 4096));
    r.scalar_quarter_known = true;
    return r;
}

int main() {
    const auto refs = synthetic_refs();

    // ---- spawn (0x8001EAE8 / 0x8001E968) ----
    {
        auto w = spawn_st02_event4_actor(record(4, 0x27, 1, -2304, -193, -379));
        CHECK(w.live && w.slot == 4 && w.subtype == 0x27 && w.byte_0c == 1 && w.byte_0d == 0);
        CHECK(w.raw_14[0] == -2304 && w.fixed_1c[0] == int32_t(0xF7000000u) && w.fixed_1c[2] == int32_t(0xFE850000u));
    }

    // ---- 20/0A init 0x80101FD8: null variant, pose 0, no sequence step ----
    auto f = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
    {
        const auto t = step_st02_event4_actor(f, &refs, 0);
        CHECK(t.model_init && !t.model_variant && t.pose_restart && t.sequence_steps == 0 && t.draw_submits == 0);
        CHECK(f.state_08 == 1 && f.word_158 == 0x02000200u && f.word_15c == 0x21080200u && f.word_78 == 0);
        // State 0 (0x80102040) steps the sequence twice per update.
        const auto t0 = step_st02_event4_actor(f, &refs, 0);
        CHECK(t0.sequence_steps == 2 && t0.draw_submits == 2 && !t0.model_init);
    }

    // ---- slot writes: one-shot state/position, drawn bit follows the slot ----
    {
        SourceSceneSlotCommands s;
        s.flags_00 = 2; s.fixed_24 = 0x06500000; set_state(s, 2);
        apply_st02_event4_slot_writes(f, s);
        CHECK(f.state_09 == 2 && f.sub_0a == 0 && (f.flags_00 & 2) && f.fixed_1c[2] == 0x06500000);
        CHECK(!s.state_written && !s.fixed_24);
        // State 2: 0x80102244 advances, then 0x80102258 moves +0x24 every update.
        step_st02_event4_actor(f, &refs, 0);
        CHECK(f.sub_0a == 1 && f.fixed_1c[2] == 0x06500000);
        step_st02_event4_actor(f, &refs, 0);
        CHECK(f.fixed_1c[2] == int32_t(0x06500000u + 0xFFF36886u));
        // A visibility write alone keeps the actor's substate.
        s.flags_00 = 0; ++s.writes;
        apply_st02_event4_slot_writes(f, s);
        CHECK(f.sub_0a == 1 && !(f.flags_00 & 2));
    }

    // ---- state 1: 180 soft-float steps, truncation drift kept ----
    {
        auto w = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
        step_st02_event4_actor(w, &refs, 0);
        SourceSceneSlotCommands s; set_state(s, 1); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, &refs, 0);   // 0x80102080
        CHECK(w.sub_0a == 1 && w.byte_0c == 0xB4 && w.rot_54[0] == 0xFF9C && w.rot_54[1] == 0xF400 && w.rot_54[2] == 0x0100);
        for (int i = 0; i < 180; ++i) step_st02_event4_actor(w, &refs, 0);
        CHECK(w.sub_0a == 2);
        CHECK(w.fixed_1c[0] == -41942880 && w.raw_14[0] == -640);
        CHECK(w.rot_44[0] == 19 && w.rot_44[1] == -150994812 && w.rot_44[2] == -43);
        CHECK(w.rot_54[0] == 0 && w.rot_54[1] == 0xF700 && w.rot_54[2] == 0xFFFF);
        const auto x = w.fixed_1c[0];
        step_st02_event4_actor(w, &refs, 0);   // 0x801021D0: holds
        CHECK(w.fixed_1c[0] == x && w.sub_0a == 2);
    }

    // ---- state 3: 75-frame turn, forward 256 through 0x8002FEA4 ----
    {
        auto w = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
        step_st02_event4_actor(w, &refs, 0);
        SourceSceneSlotCommands s; set_state(s, 3); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, &refs, 0);   // 0x801022CC, then a move at heading 0
        const auto d0 = reference_player_motion_delta(refs, 0, 256, 0, 0);
        CHECK(w.byte_0c == 75 && w.sub_0a == 1 && w.fixed_1c[0] == d0.dx_fixed && w.fixed_1c[2] == d0.dz_fixed);
        for (int i = 0; i < 75; ++i) step_st02_event4_actor(w, &refs, 0);
        CHECK(w.sub_0a == 2 && w.rot_54[1] == 375);
        auto n = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
        step_st02_event4_actor(n, nullptr, 0);
        apply_st02_event4_slot_writes(n, s);   // s already consumed: no state
        CHECK(n.state_09 == 0);
        set_state(s, 3); apply_st02_event4_slot_writes(n, s);
        CHECK(step_st02_event4_actor(n, nullptr, 0).status == St02ActorStatus::NoReferenceTables);
    }

    // ---- state 4: smoke on (1F800002 & 3) == 3, 45-frame yaw ease ----
    {
        auto w = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
        step_st02_event4_actor(w, &refs, 0);
        SourceSceneSlotCommands s; set_state(s, 4); apply_st02_event4_slot_writes(w, s);
        auto t = step_st02_event4_actor(w, &refs, 2);   // 0x80102394
        CHECK(t.effect_count == 0 && w.rot_54[1] == 0x0400 && w.rot_54[2] == 0xFF00 && w.byte_0c == 1);
        t = step_st02_event4_actor(w, &refs, 3);        // 0x801023C0: 1 -> 0, +0x0C = 45
        CHECK(w.sub_0a == 2 && w.byte_0c == 45 && t.effect_count == 2);
        CHECK(t.effects[0].function == 0x80105724u && t.effects[0].a1 == 400 && t.effects[0].a2 == -100 && t.effects[0].a4 == 1);
        CHECK(t.effects[1].a1 == -400 && t.effects[1].a3 == 200);
        for (int i = 0; i < 45; ++i) step_st02_event4_actor(w, &refs, 0);
        CHECK(w.sub_0a == 3 && w.rot_44[1] == int32_t(0x04000000u + 45u * 0xFFE82223u));
    }

    // ---- state 5: climb with exhaust, 0x80105C28 at count 90 ----
    {
        auto w = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
        step_st02_event4_actor(w, &refs, 0);
        SourceSceneSlotCommands s; set_state(s, 5); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, &refs, 0);   // 0x80102514
        CHECK(w.sub_0a == 1 && w.byte_0c == 0 && w.rot_54[1] == 0x0800);
        int c105c28 = 0, smoke = 0;
        for (int i = 1; i <= 100; ++i) {
            const auto t = step_st02_event4_actor(w, &refs, 0);
            for (int e = 0; e < t.effect_count; ++e) {
                if (t.effects[e].function == 0x80105C28u) { ++c105c28; CHECK(i == 90 && t.effects[e].a3 == 600); }
                else { ++smoke; CHECK(i >= 70 && (i & 1)); }
            }
        }
        CHECK(c105c28 == 1 && smoke == 3 * 15 && w.sub_0a == 2 && w.byte_0c == 50);
        CHECK(w.fixed_1c[2] == 100 * 0x00A00000 && w.fixed_1c[1] == 100 * 0x000A0000);
        for (int i = 0; i < 50; ++i) step_st02_event4_actor(w, &refs, 0);
        CHECK(w.sub_0a == 3);
    }

    // ---- state 6: rsin bob, amplitude * 800 into +0x20 ----
    {
        auto w = spawn_st02_event4_actor(record(3, 0x0A, 0, 0, 0, 0));
        step_st02_event4_actor(w, &refs, 0);
        SourceSceneSlotCommands s; set_state(s, 6); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, &refs, 0);
        CHECK(w.sub_0a == 1 && w.byte_0c == 0);
        for (int i = 1; i <= 16; ++i) step_st02_event4_actor(w, &refs, 0);
        CHECK(w.byte_0c == 16 && w.fixed_1c[1] == reference_rsin_8007a060(refs, 16 << 6) * 800);
        for (int i = 0; i < 120; ++i) step_st02_event4_actor(w, &refs, 0);   // byte wraps negative
        CHECK(w.byte_0c == 136 && w.fixed_1c[1] == reference_rsin_8007a060(refs, -120 * 64) * 800);
    }

    // ---- 20/27: variant from +0x0C, pose restart only on change ----
    {
        auto w = spawn_st02_event4_actor(record(4, 0x27, 1, 0, 0, 0));
        auto t = step_st02_event4_actor(w, nullptr, 0);   // 0x801028C8 -> 0x80102914
        CHECK(t.model_init && t.model_variant && *t.model_variant == 1 && t.pose_restart);
        CHECK(w.state_08 == 1 && w.byte_0d == 1 && w.word_15c == 0x42100200u && t.sequence_steps == 1);
        t = step_st02_event4_actor(w, nullptr, 0);
        CHECK(!t.model_init && !t.pose_restart && t.sequence_steps == 1);
        w.byte_0c = 2;
        t = step_st02_event4_actor(w, nullptr, 0);
        CHECK(t.model_init && *t.model_variant == 2 && w.byte_0d == 2);
        w.state_08 = 2;
        step_st02_event4_actor(w, nullptr, 0);   // 0x80102970 -> 0x80031824
        CHECK(w.released && !w.live);
    }

    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("source_st02_event4_actors: ok\n");
    return 0;
}

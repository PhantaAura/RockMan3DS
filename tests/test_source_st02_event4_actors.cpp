// ST02 event 4 family-0x20 actors: 20/0A 0x80102888 and 20/27 0x80102990.
#include "legends3ds/source_st02_event4_actors.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

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

// Each advance moves one record; the key sticks at the last record.
struct FakeSequence : St02ActorSequence {
    std::map<uint8_t, std::vector<uint8_t>> keys;    // request -> record keys
    std::array<int,3> request{{-1, -1, -1}};
    std::array<size_t,3> at{};
    std::array<int,3> restarts{};
    void restart(uint8_t s, uint8_t r) override { request[s] = r; at[s] = 0; ++restarts[s]; }
    void advance(uint8_t s) override { ++at[s]; }
    std::optional<uint8_t> key(uint8_t s) const override {
        if (request[s] < 0) return std::nullopt;
        const auto it = keys.find(uint8_t(request[s]));
        if (it == keys.end() || it->second.empty()) return uint8_t(0);
        return it->second[std::min(at[s], it->second.size() - 1)];
    }
};
static std::array<uint8_t,20> record40(uint8_t slot, uint8_t subtype) {
    auto r = record(slot, subtype, 0, 0, 0, 0); r[3] = 0x40; return r;
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

    // ---- 40/05 (slot 0) ----
    {
        FakeSequence seq;
        seq.keys[16] = {1, 27, 27, 2, 27, 27, 3};
        seq.keys[17] = {5, 6, 7, 8, 17};
        St02ActorContext c; c.sequence = &seq;
        auto w = spawn_st02_event4_actor(record40(0, 0x05));
        auto t = step_st02_event4_actor(w, c);             // 0x80102D54
        CHECK(t.model_init && t.pose_request && *t.pose_request == 16 && t.sequence_steps == 0 && t.face_tick);
        CHECK(w.rot_54[1] == 0x800 && w.state_08 == 1 && w.pose_ac == 16);
        t = step_st02_event4_actor(w, c);                  // state 0 sub 0: pose 16 again
        CHECK(*t.pose_request == 16 && w.sub_0a == 1 && t.sequence_steps == 1);
        // Sub 1 rises 0x23333 a frame and counts frames on key 27 (keys 27,27,2,27,27 -> 4th hit).
        int frames = 0;
        while (w.sub_0a == 1 && frames < 20) { step_st02_event4_actor(w, c); ++frames; }
        CHECK(frames == 5 && w.byte_0b == 4 && w.pose_ac == 17);
        CHECK(w.fixed_1c[1] == 5 * 0x00023333);
        while (w.sub_0a == 2 && frames < 40) { step_st02_event4_actor(w, c); ++frames; }   // key 7
        CHECK(w.sub_0a == 3);
        const auto y = w.fixed_1c[1];
        while (w.sub_0a == 3 && frames < 60) { step_st02_event4_actor(w, c); ++frames; }   // key 17, +0x30000
        CHECK(w.sub_0a == 4 && w.fixed_1c[1] - y == 2 * 0x00030000);
        // +0x0D freezes the step (event 0x80100970).
        w.byte_0d = 1; t = step_st02_event4_actor(w, c);
        CHECK(t.sequence_steps == 0);
        w.byte_0d = 0;
        // States 1..8 request poses 18..25 once.
        SourceSceneSlotCommands s; set_state(s, 5); apply_st02_event4_slot_writes(w, s);
        t = step_st02_event4_actor(w, c); CHECK(*t.pose_request == 22);
        t = step_st02_event4_actor(w, c); CHECK(!t.pose_request);
        // State 9: pose 26, then rise in z after key 16 until key 24.
        seq.keys[26] = {0, 16, 24};
        set_state(s, 9); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, c); CHECK(w.pose_ac == 26 && w.sub_0a == 1);   // stepped onto key 16
        step_st02_event4_actor(w, c); CHECK(w.sub_0a == 2);                      // key 16 seen; stepped onto 24
        const auto z = w.fixed_1c[2];
        step_st02_event4_actor(w, c); CHECK(w.sub_0a == 3 && w.fixed_1c[2] - z == 0x00268000);
        // Without sequence data the key tests wait.
        auto n = spawn_st02_event4_actor(record40(0, 0x05));
        St02ActorContext none;
        step_st02_event4_actor(n, none); step_st02_event4_actor(n, none);
        t = step_st02_event4_actor(n, none);
        CHECK(t.status == St02ActorStatus::SequenceUnavailable && n.sub_0a == 1);
        // Spawn record [4] != 0: the handler does nothing.
        auto r = record40(0, 0x05); r[4] = 1;
        auto off = spawn_st02_event4_actor(r);
        t = step_st02_event4_actor(off, c);
        CHECK(!t.model_init && !t.face_tick && off.state_08 == 0);
    }

    // ---- 40/06 (slot 1) and the slot-4 prop ----
    {
        FakeSequence seq;
        seq.keys[2] = {9, 9, 34};
        St02ActorContext c; c.sequence = &seq;
        std::vector<uint8_t> roll(0xC6);
        for (size_t i = 0; i < roll.size() / 2; ++i) { roll[2 * i] = uint8_t(i + 100); roll[2 * i + 1] = 0; }   // synthetic
        c.roll_801079e4 = &roll;
        auto prop = spawn_st02_event4_actor(record(4, 0x27, 1, 0, 0, 0));
        c.prop = &prop;
        auto w = spawn_st02_event4_actor(record40(1, 0x06));
        auto t = step_st02_event4_actor(w, c);             // 0x80103704 then state 0: pose 1
        CHECK(t.model_init && *t.pose_request == 1 && w.byte_143 == 0xFF && w.byte_147 == 0xFF && t.sequence_steps == 1);
        CHECK(t.prop_written && prop.byte_0c == 1);
        // Request 1 -> -0x801079E4[key]; the key is read after this frame's step.
        CHECK(prop.rot_54[2] == uint16_t(-(100 + 0)));
        // State 3: wait for key 0, pose 2, then key 34 -> pose 4.
        SourceSceneSlotCommands s; set_state(s, 3); apply_st02_event4_slot_writes(w, s);
        t = step_st02_event4_actor(w, c); CHECK(*t.pose_request == 2 && w.sub_0a == 1);
        t = step_st02_event4_actor(w, c); CHECK(!t.pose_request);
        t = step_st02_event4_actor(w, c); CHECK(*t.pose_request == 4 && w.sub_0a == 2);
        // Request 4 -> -0x80107A26[key] (table offset 0x42).
        CHECK(prop.rot_54[2] == uint16_t(-(100 + 0x21)));
        // +0x0C freezes the step (event 0x80100D78).
        w.byte_0c = 1; t = step_st02_event4_actor(w, c); CHECK(t.sequence_steps == 0); w.byte_0c = 0;
        // Request 7: -0x80107A6C[key] below 31, then -284 a frame and variant 0.
        seq.keys[7] = {3, 3, 31};
        set_state(s, 2); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, c);
        CHECK(w.pose_ac == 7 && prop.rot_54[2] == uint16_t(-(100 + 0x44 + 3)) && prop.byte_0c == 1);
        const uint16_t before = prop.rot_54[2];
        step_st02_event4_actor(w, c);
        CHECK(prop.rot_54[2] == uint16_t(before - 284) && prop.byte_0c == 0);
        // Request 9: -318, variant 0 from key 18.
        seq.keys[9] = {17, 18};
        set_state(s, 9); apply_st02_event4_slot_writes(w, s);
        step_st02_event4_actor(w, c);                      // restart 9 (key 17), stepped to 18
        CHECK(prop.rot_54[2] == uint16_t(-318) && prop.byte_0c == 0);
        // Requests 5 and 6 are constants.
        set_state(s, 5); apply_st02_event4_slot_writes(w, s); step_st02_event4_actor(w, c);
        CHECK(prop.rot_54[2] == 318 && prop.byte_0c == 1);
        set_state(s, 6); apply_st02_event4_slot_writes(w, s); step_st02_event4_actor(w, c);
        CHECK(prop.rot_54[2] == uint16_t(-63));
        // The prop then re-resolves its model when +0x0C changed (0x80102914).
        prop.byte_0c = 0; prop.byte_0d = 1; prop.state_08 = 1;
        t = step_st02_event4_actor(prop, nullptr, 0);
        CHECK(t.model_init && *t.model_variant == 0);
        // No cooked tables: request 1 reports and leaves the roll alone.
        St02ActorContext bare = c; bare.roll_801079e4 = nullptr;
        set_state(s, 0); apply_st02_event4_slot_writes(w, s);
        prop.rot_54[2] = 7;
        t = step_st02_event4_actor(w, bare);
        CHECK(t.status == St02ActorStatus::RollTableUnavailable && prop.rot_54[2] == 7);
    }

    // ---- 40/07 (slot 2): pose 1 for 30 frames, pose 2 until key 71 ----
    {
        FakeSequence seq;
        seq.keys[2] = {0, 0, 71};
        St02ActorContext c; c.sequence = &seq;
        auto w = spawn_st02_event4_actor(record40(2, 0x07));
        auto t = step_st02_event4_actor(w, c);             // 0x80103F08 only
        CHECK(t.model_init && !t.pose_request && w.state_08 == 1);
        t = step_st02_event4_actor(w, c); CHECK(*t.pose_request == 1 && t.sequence_steps == 0);
        int frames = 0;
        while (w.state_09 == 0 && frames < 40) { t = step_st02_event4_actor(w, c); ++frames; }
        CHECK(frames == 30 && w.state_09 == 1 && w.sub_0a == 0 && t.sequence_steps == 1);
        t = step_st02_event4_actor(w, c); CHECK(*t.pose_request == 2);
        step_st02_event4_actor(w, c); CHECK(w.state_09 == 1);
        step_st02_event4_actor(w, c); CHECK(w.state_09 == 0 && w.sub_0a == 0);   // stepped onto key 71
        SourceSceneSlotCommands s; set_state(s, 2); apply_st02_event4_slot_writes(w, s);
        t = step_st02_event4_actor(w, c); CHECK(*t.pose_request == 3 && t.sequence_steps == 0);
        t = step_st02_event4_actor(w, c); CHECK(t.sequence_steps == 1);
    }

    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("source_st02_event4_actors: ok\n");
    return 0;
}

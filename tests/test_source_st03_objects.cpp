// ST03 object sets (0x80103AB0) and the generic NPC 0x80107B84 on synthetic data.
#include "legends3ds/source_st03_objects.h"
#include "legends3ds/reference_tables.h"
#include <cstdio>

using namespace l3;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

static void put32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) b[o + i] = uint8_t(v >> (8 * i)); }
template <size_t N> static void put16v(std::array<uint8_t,N>& b, size_t o, int16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(uint16_t(v) >> 8); }
static void put16(std::vector<uint8_t>& b, size_t o, int16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(uint16_t(v) >> 8); }

int main() {
    // Set 0: Data (0x11, +0x0D = 0 -> behaviour 0) and a family-0x60 object.
    SourceScenePackage pkg; pkg.loaded = true;
    SourceSceneData table{kSt03ObjectSetTable, std::vector<uint8_t>(64)};
    put32(table.bytes, 0, 2); put32(table.bytes, 4, kSt03ObjectSetRecordsA);
    SourceSceneData recs{kSt03ObjectSetRecordsA, std::vector<uint8_t>(40)};
    recs.bytes[0] = 2; recs.bytes[1] = 0x11; recs.bytes[3] = 0x20; recs.bytes[8] = 9; recs.bytes[9] = 0; recs.bytes[10] = 1;
    put16(recs.bytes, 12, -5196); put16(recs.bytes, 14, -1); put16(recs.bytes, 16, 1269); put16(recs.bytes, 18, 0x0E77);
    recs.bytes[20] = 3; recs.bytes[21] = 0x0E; recs.bytes[23] = 0x60;
    pkg.data = {table, recs};
    const auto set = st03_object_set(pkg, 0);
    CHECK(set.size() == 2 && set[0].slot == 2 && set[0].object_class == 0x11 && set[0].position[0] == -5196 &&
          set[0].position[2] == 1269 && set[0].heading == 0x0E77 && set[1].family == 0x60);
    CHECK(st03_object_set(pkg, 1).empty());

    auto data = spawn_st03_object(set[0]);
    auto car = spawn_st03_object(set[1]);
    CHECK(data.generic && !car.generic && data.w.raw_14[0] == -5196 && data.w.rot_54[1] == 0x0E77);

    std::array<uint8_t,64> timers{};
    for (size_t i = 0; i < 64; ++i) timers[i] = uint8_t(10 + i);
    St03NpcInputs in; in.idle_timers = &timers; in.player_x = -5000; in.player_z = 1300; in.floor_y = -2;
    // Init + tick: scale 512, radius 5000, behaviour 0, idle request 1 for Data
    // (init's request 0, then 1 the same frame), timer from row +0x0E & 3 = 1.
    in.rand = 0x0003;
    auto t = step_st03_npc(data, in);
    CHECK(data.w.state_08 == 1 && data.w.state_09 == 0 && data.radius_368 == 5000 && data.w.word_158 == 0x02000200u &&
          data.w.word_15c == 512 && (data.w.flags_00 & 2));
    CHECK(t.in_range && t.rand_used && t.anim_request && *t.anim_request == 1 && data.w.pose_ac == 1);
    CHECK(data.timer_350 == int16_t(timers[16 + 3]) && data.w.sub_0a == 1 && data.w.raw_14[1] == -2 && t.checkable);
    // Count down to the fidget (Data: request 1, unchanged -> advance).
    in.rand.reset();
    int n = 0;
    while (data.w.sub_0a == 1 && n < 100) { t = step_st03_npc(data, in); ++n; }
    CHECK(n == timers[19] && data.w.sub_0a == 2 && data.w.pose_ac == 1 && t.anim_advance);
    t = step_st03_npc(data, in); CHECK(data.w.sub_0a == 2);           // waits for +0xA7 < 0
    in.animation_finished = true;
    t = step_st03_npc(data, in); CHECK(data.w.sub_0a == 0);
    t = step_st03_npc(data, in); CHECK(t.rand_missing && data.w.sub_0a == 0);   // rand needed again
    // Out of range: nothing runs.
    in.player_x = 2000;
    t = step_st03_npc(data, in); CHECK(!t.in_range && !t.anim_request && !t.anim_advance);
    // Behaviour other than 0 is reported.
    auto roll_rec = set[0]; roll_rec.object_class = 0x05; roll_rec.word_08[1] = 0x21;
    auto roll = spawn_st03_object(roll_rec);
    in.player_x = -5000; in.rand = 1;
    t = step_st03_npc(roll, in);
    CHECK(roll.w.state_09 == 1 && roll.radius_368 == 5000 && t.table_missing && *t.anim_request == 0);
    // Behaviour 1 (walk) with tables: Roll, +0x0E = 1 -> leg length 8.
    {
        ReferenceTables refs; refs.trig.fill({0, 4096});
        std::array<uint8_t,112> speeds{}, tables{};
        speeds[5] = 64;
        tables[0x20 + 1] = 0;                 // choice 0: walk
        tables[0x20 + 2] = 1;                 // choice 1: idle
        for (size_t i = 64; i < 112; ++i) tables[i] = 50;
        in.refs = &refs; in.walk_speeds = &speeds; in.walk_tables = &tables;
        auto r = spawn_st03_object(roll_rec);
        in.rand = 1; in.rand2 = 0;
        t = step_st03_npc(r, in);             // init + state 0
        CHECK(r.w.sub_0a == 1 && r.leg_len_366 == 8 && r.leg_left_35e == 8 && r.speed_44 == 64);
        t = step_st03_npc(r, in);             // state 1: walk, timer 50
        CHECK(t.rand_used && t.rand2_used && r.w.sub_0a == 2 && r.w.pose_ac == 1 && r.timer_350 == 50 && *t.anim_request == 1);
        const auto start = r.w.raw_14;
        int k = 0;
        while (r.w.sub_0a == 2 && k < 20) { t = step_st03_npc(r, in); ++k; }
        CHECK(k == 8 && r.w.sub_0a == 5 && r.target_heading_64 == 0 && r.timer_350 == 50 - 7 && r.w.raw_14 != start);
        k = 0;
        while (r.w.sub_0a == 5 && k < 100) { t = step_st03_npc(r, in); ++k; }
        CHECK(r.w.rot_54[1] == 0 && r.w.sub_0a == 6 && r.timer_350 == 20 && r.w.pose_ac == 0);
        k = 0;
        while (r.w.sub_0a == 6 && k < 100) { t = step_st03_npc(r, in); ++k; }
        CHECK(k == 20 && r.w.sub_0a == 7 && r.timer_350 == 70 && r.w.pose_ac == 8);
        k = 0;
        while (r.w.sub_0a == 7 && k < 100) { t = step_st03_npc(r, in); ++k; }
        CHECK(k == 70 && r.w.sub_0a == 8 && r.w.pose_ac == 1 && r.leg_left_35e == 8 && r.target_heading_64 == ((0x0E77 + 2048) & 0xFFF));
        k = 0;
        while (r.w.sub_0a == 8 && k < 100) { t = step_st03_npc(r, in); ++k; }
        CHECK(r.w.sub_0a == 1 && r.walk_dir_35c == 1 && r.w.rot_54[1] == ((0x0E77 + 2048) & 0xFFF));
        // Second leg back, then state 8 snaps home.
        in.rand = 1;
        t = step_st03_npc(r, in); CHECK(r.w.sub_0a == 2);
        k = 0;
        while (r.w.sub_0a == 2 && k < 20) { t = step_st03_npc(r, in); ++k; }
        CHECK(k == 8 && r.w.sub_0a == 5);
        // Choice 2 with a full leg left re-rolls (stays in state 1).
        tables[0x20 + 3] = 2; r.w.sub_0a = 1; r.leg_left_35e = r.leg_len_366; in.rand = 3;
        t = step_st03_npc(r, in); CHECK(r.w.sub_0a == 1);
        // The player 64 ahead stops the walk (state 9, timer 60).
        auto a = spawn_st03_object(roll_rec);
        in.rand = 1;
        step_st03_npc(a, in); step_st03_npc(a, in);
        CHECK(a.w.sub_0a == 2);
        const auto d = reference_player_motion_delta(refs, a.w.rot_54[1], int16_t(64 << 4), 0, 0);
        in.player_x = int16_t(a.w.raw_14[0] + (d.dx_fixed >> 16)); in.player_z = int16_t(a.w.raw_14[2] + (d.dz_fixed >> 16));
        t = step_st03_npc(a, in);
        CHECK(a.w.sub_0a == 9 && a.timer_350 == 60 && a.w.pose_ac == 0);
        // Still ahead after 60 with only 1 step walked: wait again (0x35E >= 0x366 - 5).
        k = 0;
        while (a.timer_350 > 1 && k < 100) { t = step_st03_npc(a, in); ++k; }
        t = step_st03_npc(a, in);
        CHECK(k == 59 && a.w.sub_0a == 9 && a.timer_350 == 60 && a.leg_left_35e == 7);
        // With 6 steps walked: turn back (state 4), then the leg is swapped.
        a.leg_left_35e = 2; a.timer_350 = 1;
        t = step_st03_npc(a, in);
        CHECK(a.w.sub_0a == 4 && a.w.pose_ac == 1 && a.target_heading_64 == ((0x0E77 + 2048) & 0xFFF));
        k = 0;
        while (a.w.sub_0a == 4 && k < 100) { t = step_st03_npc(a, in); ++k; }
        CHECK(a.w.sub_0a == 1 && a.walk_dir_35c == 1 && a.leg_left_35e == 6);
        // Player gone: state 9 resumes at state 1.
        a.w.sub_0a = 9; a.timer_350 = 1; in.player_x = -5000; in.player_z = 1300;
        t = step_st03_npc(a, in); CHECK(a.w.sub_0a == 1);
        in.refs = nullptr; in.walk_speeds = nullptr; in.walk_tables = nullptr; in.rand2.reset();
        in.player_x = -5000; in.player_z = 1300;
    }
    auto big = roll_rec; big.word_08[1] = 0x10; big.object_class = 0x26;
    auto insp = spawn_st03_object(big); step_st03_npc(insp, in);
    CHECK(insp.radius_368 == 10000 && insp.w.word_15c == 471);
    // Talk (0x8010A344 + behaviour 4) on the officer (class 0x24, behaviour 0).
    {
        auto rec = set[0]; rec.object_class = 0x24; rec.word_08 = {28, 0, 0, 0}; rec.heading = 0;
        auto o = spawn_st03_object(rec);
        St03NpcInputs k; k.idle_timers = &timers; k.player_x = -5000; k.player_z = 1300; k.rand = 5;
        auto r = step_st03_npc(o, k); CHECK(r.checkable && o.w.state_09 == 0);
        k.player_targets_me = true; k.heading_to_player = 1024;                  // player to one side
        r = step_st03_npc(o, k);
        CHECK(r.talk_target_taken && o.w.state_09 == 4 && r.talk_start && *r.talk_start == 28 && !r.checkable);
        CHECK(o.talk_34b == 1 && o.saved_heading_34c == 0 && o.turn_step_34e > 0 && o.w.pose_ac == 0);
        k.player_targets_me = false; k.talk_active = true;
        const uint16_t h0 = o.w.rot_54[1];
        r = step_st03_npc(o, k); CHECK(o.talk_34b == 1 && o.w.rot_54[1] != h0);   // turning
        k.window_open = true;
        r = step_st03_npc(o, k); CHECK(o.talk_34b == 2 && o.w.pose_ac == 8);      // talk (fidget) request
        k.talk_active = false;
        r = step_st03_npc(o, k); CHECK(o.talk_34b == 3 && o.w.pose_ac == 0);
        int n = 0;
        while (o.w.state_09 == 4 && n < 64) { r = step_st03_npc(o, k); ++n; }
        CHECK(o.w.state_09 == 0 && o.w.rot_54[1] == 0 && (o.flags_348 & 2u) && !(o.flags_348 & 4u));
        // Data's talk is behaviour 7 (reported).
        auto d2 = spawn_st03_object(set[0]); step_st03_npc(d2, k);
        k.player_targets_me = true; k.talk_active = true; r = step_st03_npc(d2, k);
        CHECK(d2.w.state_09 == 7 && !r.talk_unported && d2.talk_34b == 1 && !r.talk_start);   // state 0 ran
        k.player_targets_me = false;
        r = step_st03_npc(d2, k); CHECK(d2.talk_34b == 2 && r.talk_start && *r.talk_start == d2.w.byte_0c);   // state 1 starts the talk
        r = step_st03_npc(d2, k); CHECK(d2.talk_34b == 2);                       // waits while the talk runs
        k.save_phase = true; r = step_st03_npc(d2, k); CHECK(d2.talk_34b == 3);   // Save: main phase 6
        r = step_st03_npc(d2, k); CHECK(d2.talk_34b == 3 && !r.reopen_menu);
        k.save_phase = false; r = step_st03_npc(d2, k);
        CHECK(d2.talk_34b == 2 && r.reopen_menu && *r.reopen_menu == 0xDC);       // back to the menu
        k.talk_active = false; r = step_st03_npc(d2, k); CHECK(d2.talk_34b == 4);   // talk over
        int m = 0; while (d2.w.state_09 == 7 && m < 64) { step_st03_npc(d2, k); ++m; }
        CHECK(d2.w.state_09 == (d2.w.byte_0d & 7u) && (d2.flags_348 & 2u));
    }
    // Barrell variant 1 (set 1 record: class 0x25, [2] = 1, [4] = 1).
    {
        ReferenceTables refs; refs.trig.fill({0, 4096});
        St03ObjectRecord br; br.slot = 1; br.object_class = 0x25; br.subtype = 1; br.family = 0x20; br.byte_04 = 1;
        br.word_08 = {0x2D, 0, 0, 0}; br.position = {-2816, -1, 1280}; br.heading = 2048;
        auto b = spawn_st03_object(br);
        CHECK(b.scene_v1 && !b.generic && b.check_box[5] == 32);
        St03NpcInputs k; k.refs = &refs; k.player_x = -2816 + 400; k.player_z = 1280;   // on the x axis (no angle table needed)
        auto r = step_st03_npc(b, k);
        const auto ahead = reference_player_motion_delta(refs, 2048, 256, 0, 0);
        CHECK(b.w.state_08 == 1 && b.w.state_09 == 5 && b.w.sub_0a == 0 && r.anim_request && *r.anim_request == 0 && r.checkable);
        CHECK(b.home_354[0] == reference_fixed_to_short_80030150(int32_t(uint32_t(-2816) << 16) + ahead.dx_fixed));
        r = step_st03_npc(b, k); CHECK(b.w.sub_0a == 0 && !r.flag_set && r.anim_advance);
        k.player_targets_me = true;
        r = step_st03_npc(b, k);
        CHECK(r.flag_set && *r.flag_set == 0x604 && r.clear_player_target && b.w.sub_0a == 1 && b.w.byte_0b == 16);
        k.player_targets_me = false;
        // Facing the player (heading 0xC00 from 2048) with step |d| >> 3.
        const uint16_t h0 = b.w.rot_54[1];
        r = step_st03_npc(b, k); CHECK(b.w.rot_54[1] != h0 && b.w.sub_0a == 1);
        k.window_open = true; r = step_st03_npc(b, k); CHECK(b.w.sub_0a == 2 && b.w.pose_ac == 8 && r.anim_request && *r.anim_request == 8);
        k.animation_finished = true; r = step_st03_npc(b, k); CHECK(b.w.sub_0a == 1 && b.w.pose_ac == 0);
        k.window_open = false; k.animation_finished = false;
        // Behaviour 3: gesture while a window is open.
        b.w.state_09 = 3; b.w.sub_0a = 0;
        r = step_st03_npc(b, k); CHECK(b.w.sub_0a == 1 && b.w.pose_ac == 0);
        k.window_open = true; r = step_st03_npc(b, k); CHECK(b.w.sub_0a == 2 && b.w.pose_ac == 8);
        k.window_open = false;
        // Behaviour 1: walk to the +0x0C object's position; 0x606 on arrival.
        b.w.state_09 = 1; b.w.sub_0a = 0; b.w.rot_54[1] = 0;
        r = step_st03_npc(b, k); CHECK(r.id_target_missing && b.w.sub_0a == 0);
        k.id_target = std::array<int16_t,3>{b.w.raw_14[0], -1, int16_t(b.w.raw_14[2] - 100)};   // the test trig walks -z
        int n = 0;
        while (b.w.sub_0a < 2 && n < 200) { r = step_st03_npc(b, k); ++n; if (r.flag_set) break; }
        CHECK(r.flag_set && *r.flag_set == 0x606 && b.w.sub_0a == 2 && b.speed_44 == 0 && b.w.pose_ac == 0 && n > 1);
        // Behaviour 2 needs the target too.
        b.w.state_09 = 2; b.w.sub_0a = 0; k.id_target.reset();
        r = step_st03_npc(b, k); CHECK(r.id_target_missing && b.w.sub_0a == 1);
    }
    // Variant switch (+4 = 1): the Inspector and the officer as scene NPCs.
    {
        ReferenceTables refs; refs.trig.fill({0, 4096});
        St03ObjectRecord ir; ir.slot = 3; ir.object_class = 0x26; ir.family = 0x20; ir.word_08 = {0x2A, 0, 0, 0};
        ir.position = {-2816, -1, 1536};
        auto in0 = spawn_st03_object(ir);
        CHECK(in0.generic && !in0.scene_v1);
        st03_object_set_variant(in0, 1); in0.w.state_08 = 0;
        CHECK(!in0.generic && in0.scene_v1);
        St03NpcInputs k; k.refs = &refs; k.player_x = -2000; k.player_z = 1536;
        auto r = step_st03_npc(in0, k);
        CHECK(in0.w.state_09 == 3 && in0.w.word_15c == 471 && !r.checkable);   // starts gesturing
        k.window_open = true; r = step_st03_npc(in0, k); CHECK(in0.w.pose_ac == 8 && in0.w.sub_0a == 2);
        k.window_open = false;
        // Behaviour 1: walk to (-3003, -1, 1339); the test trig walks -z, so put him just above it.
        in0.w.state_09 = 1; in0.w.sub_0a = 0;
        in0.w.raw_14 = {-3003, -1, 1400}; in0.w.fixed_1c = {int32_t(uint32_t(uint16_t(-3003)) << 16), int32_t(0xFFFF0000u), int32_t(1400u << 16)};
        int n = 0; r = {};
        r = step_st03_npc(in0, k); ++n; CHECK(!r.sound);                       // +0xA6 != 1: no step sound
        k.anim_a6 = 1; r = step_st03_npc(in0, k); ++n; CHECK(r.sound && *r.sound == 268);   // 0x8010DD4C
        k.anim_a6 = 0;
        while (!r.flag_set && n < 100) { r = step_st03_npc(in0, k); ++n; }
        CHECK(r.flag_set && *r.flag_set == 0x606 && in0.w.sub_0a == 2 && in0.speed_44 == 0);
        // Behaviour 2: needs +0x0C, then walks to +0x0D.
        in0.w.state_09 = 2; in0.w.sub_0a = 0;
        r = step_st03_npc(in0, k); CHECK(r.id_target_missing);
        k.id_target = std::array<int16_t,3>{int16_t(in0.w.raw_14[0] + 300), -1, in0.w.raw_14[2]};   // x axis: heading 0xC00
        n = 0; r = {};
        while (!r.flag_set && n < 100) { r = step_st03_npc(in0, k); ++n; }
        CHECK(r.flag_set && in0.w.sub_0a == 2 && in0.w.rot_54[1] == 0xC00);
        r = step_st03_npc(in0, k); CHECK(r.id_target_missing && in0.w.sub_0a == 2);
        // Officer: starts standing, scale 538; behaviour 2 is the gesture.
        St03ObjectRecord orr = ir; orr.slot = 4; orr.object_class = 0x24; orr.subtype = 1;
        auto of = spawn_st03_object(orr); CHECK(of.scene_v1);
        St03NpcInputs k2; k2.refs = &refs;
        r = step_st03_npc(of, k2); CHECK(of.w.state_09 == 0 && of.w.word_15c == 538 && of.w.sub_0a == 1);
        of.w.state_09 = 2; of.w.sub_0a = 0; k2.window_open = true;
        r = step_st03_npc(of, k2); CHECK(of.w.pose_ac == 8);
        // Restart bit: the officer goes back to behaviour 0.
        of.flags_348 = 1; r = step_st03_npc(of, k2); CHECK(of.w.state_09 == 0 && of.w.sub_0a == 0);
    }
    // The police car (family 0x60, class 0x0E): idle restarts, then the
    // drive to the path-B waypoints (test trig: forward is -z).
    {
        ReferenceTables refs; refs.trig.fill({0, 4096});
        std::array<uint8_t,88> paths{};
        // Path B entry 1: 200 below the car, box 128 x 128.
        put16v(paths, 48 + 8, 0); put16v(paths, 48 + 10, -200); put16v(paths, 48 + 12, 128); put16v(paths, 48 + 14, 128);
        St03ObjectRecord cr; cr.slot = 5; cr.object_class = 0x0E; cr.family = 0x60; cr.word_08 = {0, 0, 0, 0x80};
        auto c = spawn_st03_object(cr);
        CHECK(c.car && !c.generic && !c.scene_v1);
        St03NpcInputs k; k.refs = &refs; k.car_paths = &paths;
        auto r = step_st03_npc(c, k);                      // init
        CHECK(c.w.state_08 == 1 && c.w.state_09 == 0 && !(c.w.flags_00 & 2));
        k.flag4 = true;
        r = step_st03_npc(c, k); CHECK(c.w.sub_0a == 1 && c.car_timer_c0 == 30 && (c.w.flags_00 & 2));
        int n = 0; while (c.w.sub_0a == 1 && n < 40) { step_st03_npc(c, k); ++n; }
        CHECK(n == 31 && c.w.sub_0a == 0 && c.w.state_09 == 0);   // restart after the countdown
        // Scene 2: +0x0F |= 0x40, +9 = 1: path B, from speed 0 accelerating by 22.
        c.byte_0f |= 0x40; c.w.state_09 = 1; c.w.sub_0a = 0;
        r = step_st03_npc(c, k);
        CHECK(r.moved && c.car_last == 4 && c.car_index_c4 == 1 && c.speed_44 == 22);
        CHECK(r.car_dust && r.dust_count == 4);                               // frame word 0: dust at the wheels
        k.frame_1f800002 = 1; r = step_st03_npc(c, k); CHECK(r.car_dust && r.dust_count == 0);
        k.frame_1f800002 = 4; r = step_st03_npc(c, k);
        CHECK(r.dust_count == 4 && (r.dust_points[0][0] != r.dust_points[2][0] || r.dust_points[0][2] != r.dust_points[2][2]));   // front vs rear
        k.frame_1f800002 = 0;
        n = 0; while (c.car_index_c4 == 1 && n < 400) { step_st03_npc(c, k); ++n; }
        CHECK(c.car_index_c4 == 2 && c.w.raw_14[2] < -136 && c.w.raw_14[2] > -264);
    }
    // Requests.
    CHECK(st03_npc_idle_request(0x11) == 1 && st03_npc_idle_request(0x05) == 0);
    CHECK(st03_npc_fidget_request(0x05, 0) == 8 && st03_npc_fidget_request(0x11, 0) == 1 && st03_npc_fidget_request(42, 0) == 9 &&
          st03_npc_fidget_request(83, 1) == 9 && st03_npc_fidget_request(83, 2) == 8 && st03_npc_fidget_request(57, 0) == 0);
    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("source_st03_objects: ok\n");
    return 0;
}

// ST03 event 8 scene 0 (0x80101D54) on synthetic timelines.
#include "legends3ds/source_st03_event8.h"
#include <cstdio>

using namespace l3;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

static St03Event8Profile profile() {
    constexpr uint32_t cb = 0x80103A88u;
    St03Event8Profile p;
    p.intro.token = 0x80115FB4u; p.intro.records = {{{0xFFFF0000u, cb}}, {{0xFFu, 0}}};
    p.crash.token = 0x8011600Cu;
    p.crash.records = {{{0x00030000u, cb}}, {{0xFFFF0100u, cb}}, {{0xFFFF0200u, cb}}, {{0x00020300u, cb}}, {{0xFFu, 0}}};
    p.intro_camera.token = 0x80115FC4u; p.crash_camera.token = 0x80116034u;
    p.loaded = true;
    return p;
}

int main() {
    const auto p = profile();
    St03Event8Inputs in;
    {
        auto ev = start_st03_event8(0);
        auto s = step_st03_event8(ev, p, in); CHECK(ev.state == 1);
        s = step_st03_event8(ev, p, in); CHECK(ev.state == 1 && !s.player);   // waits for 0x80042238
        in.player_ready = true;
        s = step_st03_event8(ev, p, in);                                      // states 1 and 2 in one tick
        CHECK(ev.state == 3 && s.camera_reset_8001e6c0 && ev.timeline == &p.intro && ev.camera_program == &p.intro_camera);
        CHECK(s.player && s.player->walk && s.player->position[0] == -4584 && s.player->position[2] == 1431 && *s.player->heading == 0);
        CHECK(*s.objects[0].variant_04 == 2 && *s.objects[0].heading_56 == 3703 && *s.objects[1].variant_04 == 1 && *s.objects[1].heading_56 == 3790);
        CHECK(*s.objects[0].state_08 == 0 && *s.objects[1].state_08 == 0);
        s = step_st03_event8(ev, p, in); CHECK(ev.state == 3);                // subphase not 2 yet
        in.subphase_2 = true;
        s = step_st03_event8(ev, p, in);                                      // crash timeline, record 0 (+9 = 0)
        CHECK(ev.state == 4 && ev.byte_07 == 1 && ev.timeline == &p.crash && ev.record == 0 && ev.counters.c10 == 1);
        s = step_st03_event8(ev, p, in); s = step_st03_event8(ev, p, in);     // deadline 3 -> record 1 (+9 = 1)
        CHECK(ev.record == 1 && ev.state == 4);
        s = step_st03_event8(ev, p, in);
        CHECK(ev.state == 5 && ev.countdown == 15 && s.player && !s.player->walk && *s.player->heading == 1255);
        int n = 0;
        while (ev.state == 5 && n < 40) { s = step_st03_event8(ev, p, in); ++n; }
        CHECK(n == 16 && s.message_count == 1 && s.messages[0] == 1 && *s.objects[1].state_09 == 3);
        in.message_open = true;
        s = step_st03_event8(ev, p, in); CHECK(ev.state == 6 && s.message_count == 0);
        in.message_open = false;
        s = step_st03_event8(ev, p, in);                                      // message 2; 7 sees the window open
        CHECK(ev.state == 7 && s.message_count == 1 && s.messages[0] == 2 && *s.objects[0].state_09 == 1 && *s.objects[1].state_09 == 0);
        s = step_st03_event8(ev, p, in);                                      // next record, message 3
        CHECK(ev.state == 8 && ev.record == 2 && s.messages[0] == 3 && *s.objects[0].state_09 == 0 && *s.objects[1].state_09 == 3);
        s = step_st03_event8(ev, p, in);                                      // next record, object 1 state 4
        CHECK(ev.state == 9 && ev.record == 3 && s.message_count == 0 && *s.objects[1].state_09 == 4);
        n = 0;
        while (ev.state == 9 && n < 10) { s = step_st03_event8(ev, p, in); ++n; }
        CHECK(ev.state == 10 && s.messages[0] == 4 && ev.record == 4);       // end record -> message 4
        in.message_open = true; s = step_st03_event8(ev, p, in); CHECK(ev.state == 10);
        in.message_open = false; s = step_st03_event8(ev, p, in); CHECK(ev.state == 11 && ev.countdown == 8);
        n = 0;
        while (ev.active && n < 20) { s = step_st03_event8(ev, p, in); ++n; }
        CHECK(n == 8 && s.ended && s.player_release && *s.flag_set == 3 && s.release_set && *s.spawn_set == 0);
        s = step_st03_event8(ev, p, in); CHECK(!s.ended && !s.flag_set);   // inactive
    }
    // Start skips from state 3 on: flag 10, state 11, the target placed with heading 1255.
    {
        St03Event8Inputs k; k.player_ready = true;
        auto ev = start_st03_event8(0);
        step_st03_event8(ev, p, k); step_st03_event8(ev, p, k);
        CHECK(ev.state == 3);
        k.start_pressed = true;
        auto s = step_st03_event8(ev, p, k);
        CHECK(s.skipped && *s.flag_set == 10 && ev.state == 11 && *s.player->heading == 1255);
        s = step_st03_event8(ev, p, k);
        CHECK(s.ended && *s.flag_set == 3);
        // Before state 3 Start does nothing.
        auto e2 = start_st03_event8(0);
        s = step_st03_event8(e2, p, k); CHECK(!s.skipped);
    }
    // Scenes 3..4 are not ported; scenes 1 and 2 need their timelines.
    {
        auto ev = start_st03_event8(2);
        CHECK(step_st03_event8(ev, p, in).unsupported_scene);   // profile without 0x801161D0
        auto e3 = start_st03_event8(3);
        CHECK(step_st03_event8(e3, p, in).unsupported_scene);
        auto e1 = start_st03_event8(1);
        CHECK(step_st03_event8(e1, p, in).unsupported_scene);   // profile without 0x8011614C
    }
    // Scene 1 (0x80102224), the police car, on a synthetic 0x8011614C.
    {
        auto q = p;
        q.police.token = 0x8011614Cu; q.police_camera.token = 0x80116164u;
        q.police.records = {{{0x00030000u, 0x80103A88u}}, {{0x00040100u, 0x80103A88u}}, {{0xFFu, 0}}};
        q.police_loaded = true;
        St03Event8Inputs k;
        auto ev = start_st03_event8(1);
        auto s = step_st03_event8(ev, q, k); CHECK(ev.state == 0 && !s.player);   // waits for 0x80042238
        k.player_ready = true;
        s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 2 && ev.byte_07 == 10 && s.player && s.player->from_pose && s.player->walk && *s.player_anim_448 == 48);
        int n = 0;
        while (ev.byte_07 && n < 20) { s = step_st03_event8(ev, q, k); ++n; }
        CHECK(n == 10);
        s = step_st03_event8(ev, q, k); CHECK(ev.state == 2);                     // not on the floor yet
        k.player_on_floor = true; k.turn_delta = 32;
        s = step_st03_event8(ev, q, k); CHECK(ev.state == 2 && *s.player_anim_448 == 13 && *s.player_heading_add == 32);
        k.turn_delta = 0;
        s = step_st03_event8(ev, q, k); CHECK(ev.state == 3 && *s.player_anim_448 == 48 && s.messages[0] == 31);
        k.message_open = true; s = step_st03_event8(ev, q, k); CHECK(ev.state == 3);
        k.message_open = false;
        s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 4 && *s.sound_8001a008 == 270 && ev.timeline == &q.police && s.player && !s.player->walk &&
              s.player->position[0] == -2496 && s.player->position[2] == 5065 && *s.player->heading == 2240);
        CHECK(*s.objects[3].state_09 == 1 && s.objects[3].clear_0a_0b && *s.objects[3].heading_56 == 1536 && ev.byte_07 == 10);
        n = 0;
        while (ev.state == 4 && n < 40) { s = step_st03_event8(ev, q, k); ++n; }
        CHECK(ev.state == 5 && s.messages[0] == 32);
        k.message_open = true; s = step_st03_event8(ev, q, k); CHECK(ev.state == 5);
        k.message_open = false; s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 7 && s.player_heading_zero && s.flag_8009395c && *s.flag_set == 4 && s.release_set && *s.spawn_set == 1 && !s.ended);
        s = step_st03_event8(ev, q, k);
        CHECK(!ev.active && s.ended && s.player_release);
        // Start in states 2..3 jumps to state 6 (flag 4, set 1), then 7.
        auto e2 = start_st03_event8(1);
        St03Event8Inputs r; r.player_ready = true;
        step_st03_event8(e2, q, r); CHECK(e2.state == 2);
        r.start_pressed = true;
        s = step_st03_event8(e2, q, r); CHECK(s.skipped && e2.state == 6 && s.player && s.player->position[0] == -2496);
        r.start_pressed = false;
        s = step_st03_event8(e2, q, r); CHECK(e2.state == 7 && *s.flag_set == 4 && *s.spawn_set == 1);
        s = step_st03_event8(e2, q, r); CHECK(!e2.active && s.ended);
    }
    // Scene 2 (0x8010263C), the Inspector talk, on a synthetic 0x801161D0.
    {
        auto q = p;
        q.inspector.token = 0x801161D0u; q.inspector_camera.token = 0x801161F8u;
        q.inspector.records = {{{0x00030000u, 0x80103A88u}}, {{0x00040100u, 0x80103A88u}}, {{0x00050000u, 0x80103A88u}}, {{0xFFu, 0}}};
        q.inspector_loaded = true;
        St03Event8Inputs k; k.barrell = {-2816, -1, 1280}; k.heading_barrell_to_player = 1000;
        auto ev = start_st03_event8(2);
        auto s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 1 && s.camera_focus && s.camera_focus->point[1] == -81 && s.camera_focus->distance == 300 &&
              s.camera_focus->yaw == 1032 && s.player_hide);
        k.camera_busy = true; s = step_st03_event8(ev, q, k); CHECK(ev.state == 1);
        k.camera_busy = false; s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 2 && s.message_count == 1 && s.messages[0] == 45);
        k.message_open = true; s = step_st03_event8(ev, q, k); CHECK(ev.state == 2);   // waits for the window
        k.message_open = false; s = step_st03_event8(ev, q, k); CHECK(ev.state == 2);  // and for 0x80042238
        k.player_ready = true; s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 3 && s.messages[0] == 46 && s.player && s.player->walk && s.player->position[0] == -2924 &&
              *s.player->heading == 2092 && s.release_set && *s.spawn_set == 1 && s.flag_clear_count == 2 &&
              s.flag_clears[0] == 0x606 && s.flag_clears[1] == 0x607 && *s.objects[3].variant_04 == 1 &&
              *s.objects[4].state_08 == 0 && *s.objects[1].variant_04 == 1 && s.camera_restore_80039e50 && ev.timeline == &q.inspector);
        k.player_ready = false;
        s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 4 && s.player_show && *s.objects[3].state_09 == 1 && *s.objects[1].state_09 == 2 && *s.objects[1].byte_0c_from == 3);
        s = step_st03_event8(ev, q, k); CHECK(ev.state == 4);   // waits for 0x606
        k.flag_606 = true; s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 5 && s.messages[0] == 47 && s.flag_clears[0] == 0x606 && *s.objects[3].state_09 == 3 && *s.objects[1].state_09 == 0);
        k.flag_606 = false;
        s = step_st03_event8(ev, q, k); CHECK(ev.state == 6 && s.messages[0] == 49 && *s.objects[4].state_09 == 2);
        s = step_st03_event8(ev, q, k); CHECK(ev.state == 7 && s.messages[0] == 51 && *s.objects[3].state_09 == 3);
        s = step_st03_event8(ev, q, k);
        CHECK(ev.state == 8 && ev.countdown == 200 && *s.objects[3].byte_0c_from == 4 && *s.objects[3].byte_0d_from == 5);
        int n = 0; bool barrell = false;
        while (ev.state == 8 && n < 300) { s = step_st03_event8(ev, q, k); ++n; if (s.objects[1].state_09) barrell = n == 60; }
        CHECK(n == 120 && barrell && *s.objects[4].state_09 == 1 && *s.objects[4].byte_0c_from == 5 && ev.countdown == 80);
        n = 0; while (ev.state == 9 && n < 300) { s = step_st03_event8(ev, q, k); ++n; }
        CHECK(n == 81 && ev.state == 10 && s.release_slots == 0x1A && *s.sound_8001a008 == 271 && *s.player_heading_56 == 1740 &&
              *s.objects[5].state_09 == 1 && *s.objects[5].byte_0f_or == 0x40 && *s.objects[5].heading_56 == 2048 && !s.player->walk);
        n = 0; while (ev.state == 10 && n < 300) { s = step_st03_event8(ev, q, k); ++n; }
        CHECK(n == 181 && *s.player_heading_442 == 2048);
        s = step_st03_event8(ev, q, k);
        CHECK(!ev.active && s.ended && *s.flag_set == 0 && s.player_release && (s.release_slots & 0x20));
        // Start during the talk: releases 1, 3, 4, 5 and ends through state 11.
        auto e2 = start_st03_event8(2);
        St03Event8Inputs r; r.barrell = k.barrell;
        step_st03_event8(e2, q, r); step_st03_event8(e2, q, r); r.player_ready = true; step_st03_event8(e2, q, r);
        CHECK(e2.state == 3);
        r.start_pressed = true; s = step_st03_event8(e2, q, r);
        CHECK(s.skipped && e2.state == 11 && s.release_slots == 0x3A && s.player->walk && s.player->position[0] == -2601);
        r.start_pressed = false; s = step_st03_event8(e2, q, r); CHECK(!e2.active && *s.flag_set == 0);
    }
    // Door locks: 0x8001E4C4 on ST03's list and rows (synthetic copy of 0x801152E4).
    {
        St03Event8Profile d; d.doors_loaded = true;
        d.doors = {3, 4, 8, 2, 1, 0, 0, 0};
        d.door_states[0] = {0, 1, 1, 1, 1, 0, 0, 0};
        d.door_states[14] = {0, 1, 1, 0, 0, 0, 0, 0};
        d.door_states[13] = {0, 1, 1, 0, 0, 0, 0, 0};
        d.door_states[12] = {0, 1, 1, 1, 0, 0, 0, 0};
        std::array<bool, 0x800> fl{}; fl[0x7E3] = true; fl[0x7C1] = true;
        const auto get = [&](uint16_t f) { return fl[f]; };
        const auto set = [&](uint16_t f, bool v) { fl[f] = v; };
        st03_stage_init_door_locks_801007b4(d, 0, get, set);
        CHECK(fl[0x7E1] && fl[0x7E2] && !fl[0x7E3] && fl[0x7E4] && fl[0x7E8] && !fl[0x7C1] && !fl[0x7E0]);
        fl[2] = true; st03_stage_init_door_locks_801007b4(d, 0, get, set);   // flag 2: row 14 unlocks 2 and 1
        CHECK(!fl[0x7E1] && !fl[0x7E2] && fl[0x7E4] && fl[0x7E8]);
        // 0xFF leaves a door alone; bit 1 sets 0x7C0 + id.
        std::array<uint8_t,8> st{0xFF, 3, 0, 0, 0, 0, 0, 0};
        fl[0x7E3] = true; source_door_locks_8001e4c4(d.doors, st, set);
        CHECK(fl[0x7E3] && fl[0x7E4] && fl[0x7C4]);
        // Area enter: 0x1FF once; 0x235 adds the locks 2/3/4/8 in areas 0..2.
        std::array<bool, 0x800> a{};
        const auto ag = [&](uint16_t f) { return a[f]; };
        const auto as = [&](uint16_t f, bool v) { a[f] = v; };
        CHECK(st03_area_enter_8010089c(1, 0, ag, as) && a[0x1FF] && !a[0x7E2]);
        a[0x1FF] = false; a[0x235] = true;
        CHECK(st03_area_enter_8010089c(2, 0, ag, as) && a[0x1FF] && a[0x7E2] && a[0x7E3] && a[0x7E4] && a[0x7E8]);
        a = {}; CHECK(st03_area_enter_8010089c(0, 0xFF, ag, as) && !a[0x1FF]);
        CHECK(!st03_area_enter_8010089c(7, 0, ag, as) && a[0x1FF]);   // reported, still sets 0x1FF
    }
    // 0x80100990 state machine: trigger box (cells x -5..-4, z 11..12).
    {
        uint8_t st = 0;
        bool f3 = true, f4 = false, f605 = false, f0 = false, f604 = false;
        const auto flags = [&](uint16_t f) { return (f == 3 && f3) || (f == 4 && f4) || (f == 0x605 && f605) || (f == 0 && f0) || (f == 0x604 && f604); };
        auto o = st03_area1_setup_step(st, flags, 0, 0);
        CHECK(st == 1 && *o.object_set == 0 && !o.event8_scene && *o.call_80101918 == 0);
        o = st03_area1_setup_step(st, flags, -1536, 6000); CHECK(!o.event8_scene);    // x cell -3: outside
        o = st03_area1_setup_step(st, flags, -2000, 6656); CHECK(!o.event8_scene);    // z cell 13: outside
        o = st03_area1_setup_step(st, flags, -2561, 6000); CHECK(!o.event8_scene);    // x cell -6: outside
        o = st03_area1_setup_step(st, flags, -2560, 6655);
        CHECK(*o.event8_scene == 1 && *o.flag_set == 0x605);
        f605 = true; o = st03_area1_setup_step(st, flags, -2560, 6655); CHECK(!o.event8_scene);
        f4 = true; o = st03_area1_setup_step(st, flags, 0, 0); CHECK(st == 2);
        f604 = true; o = st03_area1_setup_step(st, flags, 0, 0); CHECK(*o.event8_scene == 2 && *o.flag_clear == 0x604);
        f0 = true; o = st03_area1_setup_step(st, flags, 0, 0); CHECK(st == 3);
        uint8_t s0 = 0; o = st03_area1_setup_step(s0, [](uint16_t) { return false; }, 0, 0); CHECK(s0 == 1 && *o.event8_scene == 0);
        uint8_t s12 = 0; o = st03_area1_setup_step(s12, [](uint16_t f) { return f == 12; }, 0, 0); CHECK(s12 == 255 && !o.call_80101918 && *o.object_set == 5);
    }
    // Area 1 set-up 0x80100990.
    {
        const auto none = st03_area1_setup_80100990([](uint16_t) { return false; });
        CHECK(none.start_event8 && none.object_set == 0);
        CHECK(st03_area1_setup_80100990([](uint16_t f) { return f == 3; }).object_set == 0 &&
              !st03_area1_setup_80100990([](uint16_t f) { return f == 3; }).start_event8);
        CHECK(st03_area1_setup_80100990([](uint16_t f) { return f == 4 || f == 3; }).object_set == 1);
        CHECK(st03_area1_setup_80100990([](uint16_t f) { return f == 12 || f == 0; }).object_set == 5);
    }
    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("source_st03_event8: ok\n");
    return 0;
}

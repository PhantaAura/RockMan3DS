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
    // Scenes 1..4 are not ported.
    {
        auto ev = start_st03_event8(2);
        CHECK(step_st03_event8(ev, p, in).unsupported_scene);
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

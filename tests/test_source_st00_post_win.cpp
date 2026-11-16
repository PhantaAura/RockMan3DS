// ST00 post-win events 3 (0x80102058) and 2 (0x8010194C) on synthetic
// packages: the timelines carry the documented record headers and callback
// tokens; camera programs and spawn records are synthetic stand-ins.
#include "legends3ds/source_st00_post_win.h"
#include <cstdio>
#include <cstdlib>

namespace {
int failures = 0;
void require(bool ok, const char* what) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); ++failures; } }

std::array<uint8_t,20> record(uint8_t slot, uint8_t cls, uint8_t family, int16_t x, int16_t y, int16_t z, uint16_t heading) {
    std::array<uint8_t,20> r{};
    r[0] = slot; r[1] = cls; r[2] = 3; r[3] = family;
    const int16_t v[4] = {x, y, z, int16_t(heading)};
    for (int i = 0; i < 4; ++i) { r[12 + 2 * i] = uint8_t(uint16_t(v[i])); r[13 + 2 * i] = uint8_t(uint16_t(v[i]) >> 8); }
    return r;
}

l3::SourceScenePackage event3_package() {
    l3::SourceScenePackage p; p.loaded = true; p.event = 3; p.stage = 0;
    l3::SourceSceneTimeline t; t.token = 0x8011BE44u;
    t.records = {{{0x2d0000u, 0x80101B6Cu}}, {{0x1540100u, 0x80101B9Cu}}, {{0x4b0200u, 0x80101D80u}}, {{0x2d0300u, 0x80101D88u}},
                 {{0x430001u, 0x80101DACu}}, {{0x270201u, 0x80101F74u}}, {{0x320301u, 0x80101DDCu}}, {{0x5a0002u, 0x80101DE4u}},
                 {{0x3c0102u, 0x80101EF0u}}, {{0x730202u, 0x80101F44u}}, {{0xffu, 0u}}};
    p.timelines.push_back(t);
    l3::SourceSceneCameraProgram c; c.token = 0x8011BB3Cu; c.words = {0xFFFFFFFFu};
    p.cameras.push_back(c);
    p.spawns.push_back({0x8011BB14u, record(0, 5, 0x40, -256, -384, -640, 0)});
    p.spawns.push_back({0x8011BB28u, record(1, 8, 0x40, -256, -384, -1024, 0x800)});
    return p;
}

l3::SourceScenePackage event2_package() {
    l3::SourceScenePackage p; p.loaded = true; p.event = 2; p.stage = 0;
    l3::SourceSceneTimeline t; t.token = 0x8011BA4Cu;
    // Two documented records then the terminator keep the test short.
    t.records = {{{0x2580000u, 0x80101364u}}, {{0x9c0100u, 0x8010141Cu}}, {{0xffu, 0u}}};
    p.timelines.push_back(t);
    l3::SourceSceneData d; d.token = 0x8011B388u;
    const std::array<std::array<uint8_t,20>,4> head{{record(0, 5, 0x40, 500, -7169, 512, 0), record(1, 8, 0x40, 0, 0, 0, 0),
                                                     record(2, 2, 0xE0, 0, 0, 0, 0), record(3, 2, 0xE0, 0, 0, 0, 0)}};
    for (size_t i = 0; i < 14; ++i) {
        const auto r = i < head.size() ? head[i] : record(uint8_t(i < 7 ? i : 5), 0x1D, 0x20, 0, 0, 0, 0);
        d.bytes.insert(d.bytes.end(), r.begin(), r.end());
    }
    p.data.push_back(d);
    return p;
}

void test_bind() {
    l3::SourcePostWinProfile p3, p2;
    require(l3::bind_source_post_win_profile(event3_package(), 3, p3) && p3.spawns.size() == 2 && p3.camera, "event 3 binds");
    require(l3::bind_source_post_win_profile(event2_package(), 2, p2) && p2.spawns.size() == 14 && !p2.camera, "event 2 binds without a camera");
    auto wrong = event3_package(); wrong.stage = 7;
    require(!l3::bind_source_post_win_profile(wrong, 3, p3), "another stage is refused");
    auto no_spawn = event3_package(); no_spawn.spawns.pop_back();
    require(!l3::bind_source_post_win_profile(no_spawn, 3, p3), "missing spawn refused");
}

void test_event3() {
    l3::SourcePostWinProfile p; (void)l3::bind_source_post_win_profile(event3_package(), 3, p);
    auto ev = l3::start_source_post_win_event(3);
    l3::SourcePostWinInputs in;
    // Phase 0 waits for 0x80042238.
    auto s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 0 && !s.cd_request, "phase 0 waits for the player");
    in.player_ready = true; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 1, "phase 0 -> 1");
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 2 && s.stop_sequences && s.cd_request == 0x20000001u, "phase 1: 0x8001B33C, XA 0x20000001");
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 2, "phase 2 waits for 0x8001D878");
    in.cd_done = true; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 3 && ev.countdown == 30, "phase 2: countdown 30");
    for (int i = 0; i < 29; ++i) s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 3 && ev.countdown == 1, "phase 3 counts");
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 4, "phase 3 -> 4");
    // Phase 4: the constructor, then 5/6 in the same tick.
    s = l3::step_source_post_win_event(ev, p, in);
    require(s.constructed && s.player_hidden && s.camera_program == 0x8011BB3Cu && s.spawn_count == 2 && ev.phase == 5, "constructor");
    require(ev.slots[0].spawned && ev.slots[0].object_class == 5 && ev.slots[0].spawn[2] == -640, "slot 0 = 40/05");
    require(ev.slots[1].spawned && ev.slots[1].object_class == 8 && ev.slots[1].spawn_heading == 0x800, "slot 1 = 40/08");
    require(s.callback == 0x80101B6Cu && ev.counters.c10 == 1, "first callback runs at +0x10 = 0");
    // 0x80101B6C at +0x10 = 15: slot 1 +9 = 1.
    for (int i = 0; i < 15; ++i) s = l3::step_source_post_win_event(ev, p, in);
    require(ev.slots[1].state_09 == 1, "0x80101B6C slot 1 state 1");
    // Into record 1 (0x80101B9C): +0x10 = 0 sets slot 1 state 3, 0xE6 two smoke emitters.
    while (ev.record < 1) s = l3::step_source_post_win_event(ev, p, in);
    s = l3::step_source_post_win_event(ev, p, in);
    require(s.callback == 0x80101B9Cu && ev.slots[1].state_09 == 3, "0x80101B9C slot 1 state 3");
    in.slot_position[1] = std::array<int16_t,3>{{-256, -384, -1024}};
    while (ev.counters.c10 != 0xE6) s = l3::step_source_post_win_event(ev, p, in);
    s = l3::step_source_post_win_event(ev, p, in);
    require(s.smoke_count == 2 && s.smoke[0][0] == -256 - 0x40 && s.smoke[0][1] == -384 - 0x174 && s.smoke[1][0] == -0x15E, "0xE6 smoke pair");
    // Run to the timeline's end, collecting effects.
    bool byte128_c = false, released = false, object = false, sound = false;
    while (ev.counters.e8 != 0xFF && ev.ticks < 2000) {
        s = l3::step_source_post_win_event(ev, p, in);
        byte128_c |= ev.slots[1].byte_128 == 0x0C;
        released |= s.smoke_release; object |= s.object.has_value(); sound |= s.sound_8002ee90.has_value();
    }
    require(ev.counters.e8 == 0xFF && byte128_c && released && object && sound, "timeline effects 0x80101D88/DE4/EF0");
    require(ev.slots[1].byte_128 == 0x0D && ev.slots[0].state_09 == 0, "0x80101EF0 then 0x80101F44");
    // Phase 5: fade 10 when not busy; the same tick sees it busy.
    in.fade_busy = 1; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 5 && !s.fade_type, "busy holds the fade");
    in.fade_busy = 0; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 6 && s.fade_type == 10 && !s.handoff && ev.active, "fade 10, no hand-off this tick");
    in.fade_busy = 0x10; s = l3::step_source_post_win_event(ev, p, in);
    require(!s.handoff, "phase 6 waits for the fade");
    in.fade_busy = 0; s = l3::step_source_post_win_event(ev, p, in);
    require(s.handoff && !ev.active && s.handoff->area == 4 && s.handoff->stage == 0 && s.handoff->cleared_flag == 0x606 &&
            s.handoff->raw[12] == 2 && s.handoff->raw[15] == 4 && s.handoff->raw[16] == 0xFF && s.handoff->raw[0] == 0 && s.handoff->raw[10] == 0,
            "hand-off: area 4 at (0,0,0), 0x606");
    s = l3::step_source_post_win_event(ev, p, in);
    require(!s.callback && !s.handoff, "an ended event does nothing");
}

void test_event3_skip() {
    l3::SourcePostWinProfile p; (void)l3::bind_source_post_win_profile(event3_package(), 3, p);
    auto ev = l3::start_source_post_win_event(3);
    ev.phase = 4;
    l3::SourcePostWinInputs in; in.start_skip = true;
    auto s = l3::step_source_post_win_event(ev, p, in);
    require(s.constructed && s.fade_type == 10 && ev.phase == 6, "Start skip fades on the constructor tick");
}

void test_event2() {
    l3::SourcePostWinProfile p; (void)l3::bind_source_post_win_profile(event2_package(), 2, p);
    auto ev = l3::start_source_post_win_event(2);
    l3::SourcePostWinInputs in; in.fade_busy = 1;
    auto s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 0, "phase 0 waits for the fade");
    in.fade_busy = 0; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 1, "phase 0 -> 1");
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 1 && !s.player_hidden, "phase 1 waits for the player");
    in.player_ready = true; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 2 && s.player_hidden, "phase 1 hides the player");
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 3 && s.cd_request == 0x20000002u && s.stop_sequences, "phase 2: XA 0x20000002");
    in.cd_done = true; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 4 && ev.countdown == 30, "phase 3: countdown 30");
    for (int i = 0; i < 30; ++i) s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 5 && s.fade_type == 2, "phase 4: fade 2 at zero");
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 6 && s.constructed && s.spawn_count == 14 && s.camera_program == 0x8011B4B4u && !s.callback, "constructor 0x80101888");
    require(ev.slots[0].spawn[0] == 500 && ev.slots[0].spawn[1] == -7169 && ev.slots[0].spawn[2] == 512 && ev.slots[5].spawned, "spawn slots");
    s = l3::step_source_post_win_event(ev, p, in);
    require(s.callback == 0x80101364u && ev.slots[0].colour_15e == 0x2108, "0x80101364 at +0x10 = 0");
    while (ev.counters.c10 != 0x14A) s = l3::step_source_post_win_event(ev, p, in);
    s = l3::step_source_post_win_event(ev, p, in);
    require(ev.slots[2].byte_0c == 1 && ev.slots[3].byte_0c == 1, "0x80101364 at 0x14A");
    while (ev.record < 1) s = l3::step_source_post_win_event(ev, p, in);
    require(ev.slots[0].state_09 == 2, "0x80101364 at 0x219: slot 0 state 2");
    s = l3::step_source_post_win_event(ev, p, in);
    require(s.callback == 0x8010141Cu && (ev.slots[1].flags_00 & 2) && ev.slots[0].state_09 == 3, "0x8010141C");
    while (ev.counters.e8 != 0xFF && ev.ticks < 2000) s = l3::step_source_post_win_event(ev, p, in);
    in.fade_busy = 0; s = l3::step_source_post_win_event(ev, p, in);
    require(ev.phase == 7 && s.fade_type == 10 && !s.handoff, "phase 6: fade 10");
    s = l3::step_source_post_win_event(ev, p, in);
    require(s.handoff && s.handoff->stage == 2 && s.handoff->area == 0 && s.handoff->cleared_flag == 0x605 && !ev.active,
            "hand-off: stage 2 area 0, 0x605");
}

void test_event2_callbacks() {
    l3::SourcePostWinEvent ev = l3::start_source_post_win_event(2);
    l3::SourcePostWinInputs in; l3::SourcePostWinStep out;
    l3::run_source_post_win_callback(ev, 0x801014ECu, in, out);
    require(ev.slots[1].fixed_1c == 0x01F40000 && ev.slots[1].fixed_20 == int32_t(0xE3FF0000u) && ev.slots[1].colour_15e == 0x4210, "0x801014EC");
    l3::run_source_post_win_callback(ev, 0x801016ECu, in, out);
    require(out.fx_8011877c_count == 2 && out.fx_8011877c[1][4] == 0x8E0, "0x801016EC");
    ev.counters.c10 = 0x3C; l3::run_source_post_win_callback(ev, 0x80101794u, in, out);
    require(ev.slots[0].state_09 == 11 && (ev.slots[4].flags_00 & 2) && ev.slots[4].state_09 == 1, "0x80101794");
    ev.counters.c10 = 0; l3::run_source_post_win_callback(ev, 0x80101844u, in, out);
    require(ev.slots[4].sub_0a == 2 && ev.slots[1].state_09 == 0, "0x80101844");
}
} // namespace

int main() {
    test_bind(); test_event3(); test_event3_skip(); test_event2(); test_event2_callbacks();
    if (failures) return EXIT_FAILURE;
    std::puts("source_st00_post_win: ok");
    return 0;
}

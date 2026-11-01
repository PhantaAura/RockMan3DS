// ST07 event 10: camera program 0x8001EC0C, timeline 0x8001F070 and the
// handler 0x80100330 on an authored profile.
#include "legends3ds/source_attract_event10.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
namespace {
void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); } }
uint32_t rec(uint8_t e8, uint8_t e9, uint16_t ticks) { return uint32_t(e8) | (uint32_t(e9) << 8) | (uint32_t(ticks) << 16); }
}
int main() {
    // Camera: set target, velocity, wait for e9 == 1 on its first tick, then a cut.
    SourceAttractCameraProgram cam{0x80105E2Cu, {
        0x20000000u, 0x10000000u, 0x00010000u, 0x00020000u, 0x00030000u,
        0x12000000u, 0x00008000u, 0u, 0u,
        0x02010000u, 0x21000000u, 0x10000000u, 0x00640000u, 0u, 0u, 0x12000000u, 0u, 0u, 0u, 0xFFFFFFFFu}};
    SourceCameraBlock c; SourceEventCounters ev;
    step_source_camera_program_8001ec0c(c, cam, ev);
    auto v = source_camera_view(c);
    require(c.pc == 9 && v.target[0] == 1 && c.target[0] == 0x00018000 && v.target[1] == 2, "set, velocity added, waits at op 2");
    step_source_camera_program_8001ec0c(c, cam, ev);
    require(c.pc == 9 && c.target[0] == 0x00020000, "pc keeps its place; motion continues");
    ev.e9 = 1; ev.c1c = 0;
    step_source_camera_program_8001ec0c(c, cam, ev);
    v = source_camera_view(c);
    require(c.pc == 19 && v.target[0] == 100 && c.target_velocity[0] == 0 && v.shift == 1, "cut on the shot's first tick");
    // Timeline: 3 ticks of (0,0), then (1,0) resets +0x18/+0x1C; terminator stops.
    SourceAttractTimeline tl{0x801060D4u, {{{rec(0, 0, 3), 0x801007E0u}}, {{rec(1, 0, 2), 0x80100944u}}, {{0xFFu, 0}}}};
    SourceEventCounters e; size_t r = 0;
    for (int i = 0; i < 3; ++i) step_source_timeline_8001f070(e, r, tl);
    require(r == 1 && e.e8 == 1 && e.c10 == 0 && e.c18 == 0 && e.c14 == 3, "record advance at its tick count");
    step_source_timeline_8001f070(e, r, tl); step_source_timeline_8001f070(e, r, tl);
    require(r == 2 && e.e8 == 0xFF, "terminator record");
    // Handler: phase 0 (CD, fade 2 at 10, switch at 30), phase 1 (area 1 on the
    // callback's first tick, fade 20 at the end), phase 2 done.
    SourceAttractProfile p; p.loaded = true;
    p.timelines[0] = {0x80105E1Cu, {{{0xFFFF0000u, 0x801007E0u}}, {{0xFFu, 0}}}};
    p.timelines[1] = tl;
    p.cameras[0] = {0x80105DD0u, {0x20000000u, 0xFFFFFFFFu}};
    p.cameras[1] = cam;
    for (auto& row : p.bg_rows) row = {{3, 0, 0, 0, 0x80, 0xFF, 0, 0}};   // flags 3, vx -128
    SourceBgLayers L{}; for (auto& l : L) l.present = true;
    SourceAttractEvent10State st;
    auto s = step_source_attract_event10_80100330(st, p, L, false, 0, false);
    require(s.cd_command && st.sub == 1 && L[0].flags == 3 && L[0].vx == -128 && L[7].y == 240, "phase 0 setup");
    s = step_source_attract_event10_80100330(st, p, L, true, 0, false);
    require(st.sub == 2 && s.callback == 0x801007E0u, "CD done");
    unsigned fade2 = 0;
    while (st.phase == 0) fade2 += step_source_attract_event10_80100330(st, p, L, true, 0, false).fade_type == 2;
    require(fade2 == 1 && st.timeline == 1 && st.camera_program == 1, "fade 2, then the main tables");
    int area = -1; uint8_t end_fade = 0;
    for (int i = 0; i < 20 && st.phase == 1; ++i) {
        const auto t = step_source_attract_event10_80100330(st, p, L, true, 0, false);
        if (t.area_request >= 0) area = t.area_request;
        if (t.fade_type) end_fade = t.fade_type;
    }
    require(area == 1 && end_fade == 20 && st.phase == 2, "area 1 request, fade 20 at the end");
    step_source_attract_event10_80100330(st, p, L, true, 0, false);
    step_source_attract_event10_80100330(st, p, L, true, 0, false);
    require(st.done, "phase 2 ends after the fade");
    // Start during phase 1 -> fade 10.
    SourceAttractEvent10State sk;
    while (!(sk.phase == 1 && sk.sub == 1)) step_source_attract_event10_80100330(sk, p, L, true, 0, false);
    step_source_attract_event10_80100330(sk, p, L, true, 0, true);
    require(step_source_attract_event10_80100330(sk, p, L, true, 0, false).fade_type == 10 && sk.phase == 2, "Start skips with fade 10");
    std::puts("source_attract_event10: camera waits/cuts/velocity, timeline, phases, area request, end and skip fades OK");
}

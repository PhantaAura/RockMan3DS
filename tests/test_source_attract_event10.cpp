// ST07 event 10 on the shared scene runtime: the L3CS package loader and
// binding, camera program 0x8001EC0C, timeline 0x8001F070 and the handler
// 0x80100330, on authored data.
#include "legends3ds/source_attract_event10.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
namespace {
void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); } }
uint32_t rec(uint8_t e8, uint8_t e9, uint16_t ticks) { return uint32_t(e8) | (uint32_t(e9) << 8) | (uint32_t(ticks) << 16); }
void u32(std::vector<uint8_t>& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(uint8_t(v >> (8 * i))); }
uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
// An authored L3CS in tools/cook_cutscene.py's layout.
std::vector<uint8_t> package() {
    std::vector<uint8_t> p(64, 0);   // EXE + overlay SHA-256
    p.push_back(10); p.push_back(0); p.push_back(7); p.push_back(0); u32(p, 0x80100330u);
    u32(p, 2);
    u32(p, 0x80105E1Cu); u32(p, 2); u32(p, 0xFFFF0000u); u32(p, 0x801007E0u); u32(p, 0xFFu); u32(p, 0);
    u32(p, 0x801060D4u); u32(p, 1); u32(p, 0xFFu); u32(p, 0);
    u32(p, 2);
    u32(p, 0x80105DD0u); u32(p, 1); u32(p, 0xFFFFFFFFu);
    u32(p, 0x80105E2Cu); u32(p, 2); u32(p, 0x20000000u); u32(p, 0xFFFFFFFFu);
    u32(p, 1); u32(p, 0x80105B8Cu); for (int i = 0; i < 20; ++i) p.push_back(uint8_t(i));
    u32(p, 1); u32(p, 0x20000005u);
    u32(p, 1); u32(p, 0x80100944u); u32(p, 1);
    u32(p, 0x80105658u);
    u32(p, 1); u32(p, 0x80106124u); u32(p, 64); for (int i = 0; i < 64; ++i) p.push_back(uint8_t(i));
    std::vector<uint8_t> b = {'L', '3', 'C', 'S', 1, 0, 16, 0};
    u32(b, uint32_t(16 + p.size())); u32(b, crc32(p.data(), p.size()));
    b.insert(b.end(), p.begin(), p.end());
    return b;
}
}
int main() {
    // L3CS: load, look up by original address, bind the event-10 profile.
    auto bytes = package();
    SourceScenePackage pkg;
    require(load_source_scene_package(bytes, pkg) && pkg.event == 10 && pkg.stage == 7 && pkg.handler == 0x80100330u &&
            pkg.timelines.size() == 2 && pkg.camera(0x80105E2Cu) && pkg.camera(0x80105E2Cu)->words.size() == 2 &&
            pkg.spawns.size() == 1 && pkg.spawns[0].bytes[19] == 19 && pkg.cd_requests[0] == 0x20000005u &&
            pkg.area_requests[0].area == 1 && pkg.bg_table == 0x80105658u && pkg.data_at(0x80106124u), "package loads");
    SourceAttractProfile bound;
    require(bind_source_attract_profile(pkg, bound) && bound.bg_rows[1][0] == 8 && bound.area_requests.size() == 1, "profile binds");
    auto bad = bytes; bad.back() ^= 1;
    require(!load_source_scene_package(bad, pkg = {}), "CRC rejects");
    bytes.push_back(0);
    require(!load_source_scene_package(bytes, pkg), "size rejects");
    // Camera: set target, velocity, wait for e9 == 1 on its first tick, then a cut.
    SourceSceneCameraProgram cam{0x80105E2Cu, {
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
    SourceSceneTimeline tl{0x801060D4u, {{{rec(0, 0, 3), 0x801007E0u}}, {{rec(1, 0, 2), 0x80100944u}}, {{0xFFu, 0}}}};
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
    p.area_requests = {{0x80100944u, 1}};
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
    // Servbots: spawn fields from the record, the id-4 walk, first-tick request.
    SourceAttractProfile cast; cast.loaded = true;
    cast.servbot_behaviours.assign(25, 0x80102238u); cast.servbot_behaviours[4] = 0x8010241Cu;
    std::array<uint8_t,20> r4{{1, 0x00, 4, 0x20, 0, 0, 0, 0, 4, 1, 1, 0, 0, 0, 0, 0, 0xF4, 0x01, 0x00, 0x02}};   // pos (0, 0, 500), heading 512
    SourceAttractServbot sb;
    require(spawn_source_attract_servbot(r4, cast, sb) && sb.slot == 1 && sb.id == 4 && sb.request == 1 && sb.big == 1 &&
            sb.raw[2] == 500 && sb.fixed[2] == (500 << 16) && sb.heading == 512 && sb.scale()[0] == 1024 && sb.behaviour == 0x8010241Cu,
            "servbot spawn fields");
    ReferenceTables refs;
    auto an = update_source_attract_servbot_8010205c(sb, 1, refs);
    require(an.reset && an.request == 1 && sb.raw[0] == -3 && sb.raw[2] == 497 && sb.fixed[0] == int32_t(0xFFFC6A7Fu), "first tick: request + walk step");
    for (uint32_t t = 2; t < 20; ++t) update_source_attract_servbot_8010205c(sb, t, refs);
    require(sb.raw[0] == -57 && sb.raw[2] == 443, "x/z -3 per tick for 19 ticks");
    an = update_source_attract_servbot_8010205c(sb, 20, refs);
    require(an.step && sb.raw[0] == -57 && sb.raw[2] == 442, "then z -1 only");
    std::array<uint8_t,20> ship{{0, 0x0B, 10, 0x20}};
    require(!spawn_source_attract_servbot(ship, cast, sb), "other classes are not Servbots");
    // The corridor callback (record 1, so its first tick has +0x10 == 0)
    // spawns records 4..6 after releasing three slots.
    SourceAttractProfile pc = p; pc.timelines[1] = {0x801060D4u, {{{rec(0, 0, 2), 0x801007E0u}}, {{rec(2, 0, 5), 0x801009B8u}}, {{0xFFu, 0}}}};
    SourceAttractEvent10State cs;
    int first = -1; unsigned count = 0, released = 0;
    while (cs.phase != 2 && first < 0) {
        const auto t = step_source_attract_event10_80100330(cs, pc, L, true, 0, false);
        if (t.spawn_first >= 0) { first = t.spawn_first; count = t.spawn_count; released = t.release_slots; }
    }
    require(first == 4 && count == 3 && released == 3, "corridor spawn list");
    // The bridge's class 0x28 / 0x2B: request 19 / 0, record placement.
    std::array<uint8_t,20> b28{{11, 0x28, 10, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0x88, 0x03, 0xFF, 0xFF, 0x54, 0x01, 0, 0}};
    SourceAttractStatic st28;
    require(spawn_source_attract_static(b28, st28) && st28.object_class == 0x28 && st28.request == 19 &&
            st28.raw[0] == 904 && st28.raw[1] == -1 && st28.raw[2] == 340 && st28.slot == 11, "class 0x28 spawn");
    b28[1] = 0x2B; require(spawn_source_attract_static(b28, st28) && st28.request == 0, "class 0x2B request 0");
    b28[1] = 0x0B; require(!spawn_source_attract_static(b28, st28), "other classes rejected");
    std::puts("source_attract_event10: camera waits/cuts/velocity, timeline, phases, area request, end and skip fades OK");
}

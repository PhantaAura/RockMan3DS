// RuntimeSession owning ST00 post-win events 3 and 2: the poll start from
// flag 0x606, the CD acknowledgement, the hand-off offered as a transition,
// the loader commit into area 4 (prefix flag 0x605) and event 2's start.
#include "legends3ds/runtime_session.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;

namespace {
int failures = 0;
void require(bool ok, const char* what) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); ++failures; } }

RuntimeSession make_session() {
    RuntimeMap map;
    for (int i = 0; i < 5; ++i) {
        RuntimeMapArea a; a.start_x = 63; a.start_z = 63; a.width = 2; a.height = 2; a.cell_count = 4; map.areas.push_back(a);
    }
    map.cells.assign(4, int16_t(-32768));
    TutorialRuntimeArea runtime; L3QolSettings settings{}; l3_qol_settings_defaults(&settings);
    return RuntimeSession(std::move(map), std::move(runtime), {}, settings);
}
SourceEventPollProfile poll() {
    SourceEventPollProfile p; p.decoder = 1;
    p.records = {{0x604, 1, 0}, {0x605, 2, 0}, {0x606, 3, 0}, {0x7B0, 0, 0}};
    return p;
}
SourcePostWinProfile profile(uint8_t event) {
    SourcePostWinProfile p; p.loaded = true; p.event = event;
    p.timeline.token = event == 3 ? 0x8011BE44u : 0x8011BA4Cu;
    // One short record each, then the terminator.
    p.timeline.records = {{{0x50000u, event == 3 ? 0x80101DACu : 0x80101364u}}, {{0xffu, 0u}}};
    if (event == 3) {
        SourceSceneCameraProgram c; c.token = 0x8011BB3Cu; c.words = {0xFFFFFFFFu}; p.camera = c;
        std::array<uint8_t,20> a{}, b{}; b[0] = 1; b[1] = 8; a[1] = 5; p.spawns = {a, b};
    } else {
        for (uint8_t i = 0; i < 14; ++i) { std::array<uint8_t,20> r{}; r[0] = i < 7 ? i : 5; p.spawns.push_back(r); }
    }
    return p;
}
} // namespace

int main() {
    auto s = make_session();
    s.bind_source_event_profile(poll(), 0);
    require(s.bind_source_post_win_profile(profile(3)) && s.bind_source_post_win_profile(profile(2)), "profiles bind");
    s.tick({});
    require(!s.source_post_win_event().active, "no flag, no event");
    // 0x604 earlier in the table wins over 0x606.
    s.set_story_flag(0x604, true); s.set_story_flag(0x606, true); s.tick({});
    require(!s.source_post_win_event().active, "0x604 first: event 1 is not a native post-win event");
    s.set_story_flag(0x604, false); s.tick({});
    require(s.source_post_win_event().active && s.source_post_win_event().event == 3, "0x606 starts event 3");
    // Phase 0 (0x80042238 takes the player), phase 1 (CD request), phase 2 waits.
    int guard = 0;
    while (!s.pending_source_post_win_cd() && guard++ < 10) s.tick({});
    require(s.pending_source_post_win_cd() == 0x20000001u, "XA 0x20000001 requested");
    s.tick({}); s.tick({});
    require(s.source_post_win_event().phase == 2, "phase 2 waits for the CD acknowledgement");
    s.acknowledge_source_post_win_cd(0x20000001u);
    guard = 0;
    while (s.source_post_win_event().phase < 5 && guard++ < 50) s.tick({});
    require(s.source_post_win_event().constructed && (s.compat_player().bytes[0] & 3) == 0, "constructor hides the player");
    guard = 0;
    while (!s.source_post_win_handoff() && guard++ < 100) s.tick({});
    // No fade profile is bound, so 0x80017C30 is a held request and busy stays 0.
    const auto t = s.source_post_win_transition();
    require(t && t->destination_area == 4 && t->source_function == 0x80102058u && !s.story_flag(0x606) && (s.compat_player().bytes[0] & 1),
            "hand-off: area 4 offered, 0x606 cleared, player visible");
    require(!s.can_commit_source_post_win_transition(), "unregistered area 4 refused");
    TutorialRuntimeArea a4; s.register_area(4, a4);
    require(s.can_commit_source_post_win_transition() && s.commit_source_post_win_transition(), "commit");
    const auto p = s.player_position();
    require(s.map_area_index() == 4 && p.x == 0 && p.y == 0 && p.z == 0 && s.story_flag(0x605) && (s.compat_player().bytes[0] & 3) == 0,
            "area 4 at (0,0,0), prefix hides the player and sets 0x605");
    require(!s.source_post_win_transition(), "the hand-off is consumed");
    s.bind_source_event_profile(poll(), 0);   // main.cpp rebinds the poll after every transition
    s.tick({});
    require(s.source_post_win_event().active && s.source_post_win_event().event == 2, "0x605 starts event 2");
    guard = 0;
    while (!s.pending_source_post_win_cd() && guard++ < 10) s.tick({});
    require(s.pending_source_post_win_cd() == 0x20000002u, "XA 0x20000002 requested");
    s.acknowledge_source_post_win_cd(0x20000002u);
    guard = 0;
    while (!s.source_post_win_handoff() && guard++ < 200) s.tick({});
    const auto& h = s.source_post_win_handoff();
    require(h && h->stage == 2 && h->area == 0 && !s.story_flag(0x605) && !s.source_post_win_transition(),
            "event 2 asks for stage 2; retained, not offered");
    s.tick({});
    require(!s.source_post_win_event().active, "no restart while the stage-2 request is retained");
    if (failures) return EXIT_FAILURE;
    std::puts("runtime_post_win: ok");
    return 0;
}

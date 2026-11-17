#include "legends3ds/source_st03_event8.h"

namespace l3 {
namespace {

constexpr uint32_t kIntroTimeline = 0x80115FB4u, kCrashTimeline = 0x8011600Cu;
constexpr uint32_t kIntroCamera = 0x80115FC4u, kCrashCamera = 0x80116034u;
constexpr uint32_t kRecordCallback = 0x80103A88u;   // 0x8001EC0C + 0x8001F070
constexpr std::array<int16_t,3> kCrashTarget{{-4584, -1, 1431}};   // 0xEE180000, 0xFFFF0000, 0x05970000
constexpr uint32_t kPoliceTimeline = 0x8011614Cu, kPoliceCamera = 0x80116164u;
constexpr std::array<int16_t,3> kPoliceTarget{{-2496, -1, 5065}};  // 0xF6400000, 0xFFFF0000, 0x13C90000

uint32_t header(const St03Event8& ev) noexcept {
    return ev.timeline && ev.record < ev.timeline->records.size() ? ev.timeline->records[ev.record][0] : 0xFFu;
}
bool at_end(const St03Event8& ev) noexcept { return uint8_t(header(ev)) == 0xFFu; }

void reset_8001f1dc(St03Event8& ev) noexcept {   // +8, +9, +0x10, +0x18, +0x1C = 0 (+0x14 kept)
    ev.counters.e8 = ev.counters.e9 = 0;
    ev.counters.c10 = ev.counters.c18 = ev.counters.c1c = 0;
}
void next_record_8001f158(St03Event8& ev) noexcept {
    if (!ev.timeline || ev.record + 1 >= ev.timeline->records.size()) return;
    ++ev.record;
    const uint32_t h = ev.timeline->records[ev.record][0];
    const uint8_t b8 = uint8_t(h), b9 = uint8_t(h >> 8);
    if (ev.counters.e8 != b8) { ev.counters.c18 = 0; ev.counters.c1c = 0; }
    ev.counters.e8 = b8;
    if (ev.counters.e9 != b9) ev.counters.c1c = 0;
    ev.counters.c10 = 0;
    ev.counters.e9 = b9;
}
void begin(St03Event8& ev, St03Event8Step& out, const SourceSceneTimeline& t, const SourceSceneCameraProgram& c) noexcept {
    reset_8001f1dc(ev);
    out.camera_reset_8001e6c0 = true;
    ev.camera_program = &c; ev.camera = {};
    ev.timeline = &t; ev.record = 0;
}
void target(St03Event8Step& out, bool walk, std::optional<uint16_t> heading) noexcept {
    St03PlayerTarget p; p.position = kCrashTarget; p.walk = walk; p.heading = heading;
    out.player = p;
}
void message(St03Event8Step& out, bool& open, uint8_t block) noexcept {
    // 0x8005382C sets the slot-0 window flag before returning.
    if (out.message_count < out.messages.size()) out.messages[out.message_count++] = block;
    open = true;
}
void object_state(St03Event8Step& out, size_t slot, uint8_t s9) noexcept { out.objects[slot].state_09 = s9; }

void scene0(St03Event8& ev, const St03Event8Profile& p, const St03Event8Inputs& in, St03Event8Step& out) noexcept {
    bool open = in.message_open;
    bool end_check = false;
    switch (ev.state) {   // table 0x80100004
    case 0: ++ev.state; break;
    case 1:
        if (!in.player_ready) break;
        begin(ev, out, p.intro, p.intro_camera); ++ev.state;
        [[fallthrough]];
    case 2:   // 0x80101DFC
        target(out, true, uint16_t(0));
        out.objects[0].variant_04 = 2; out.objects[0].state_08 = 0; out.objects[0].heading_56 = 3703;
        out.objects[1].variant_04 = 1; out.objects[1].state_08 = 0; out.objects[1].heading_56 = 3790;
        ++ev.state;
        break;
    case 3:   // 0x80101E90
        if (!in.subphase_2) break;
        ev.byte_07 = 1; ++ev.state;
        begin(ev, out, p.crash, p.crash_camera);
        [[fallthrough]];
    case 4:   // 0x80101EE4
        if (uint8_t(header(ev) >> 8) != 1) break;
        ++ev.state; target(out, false, uint16_t(1255)); ev.countdown = 15;
        break;
    case 5:   // 0x80101F40
        if (--ev.countdown >= 0) break;
        message(out, open, 1); ++ev.state; object_state(out, 1, 3);
        break;
    case 6:   // 0x80101F88
        if (open) { end_check = true; break; }
        message(out, open, 2); ++ev.state; object_state(out, 0, 1); object_state(out, 1, 0);
        [[fallthrough]];
    case 7:   // 0x80101FE0
        if (open) { end_check = true; break; }
        next_record_8001f158(ev); message(out, open, 3); ++ev.state; object_state(out, 0, 0); object_state(out, 1, 3);
        [[fallthrough]];
    case 8:   // 0x80102044
        if (open) { end_check = true; break; }
        next_record_8001f158(ev); ++ev.state; object_state(out, 1, 4);
        end_check = true;
        break;
    case 9: end_check = true; break;   // 0x80102088
    case 10:  // 0x801020B8
        if (open) break;
        ev.countdown = 8; ++ev.state;
        break;
    case 11:  // 0x801020E4
        if (--ev.countdown != 0) break;
        out.player_release = true; ev.active = false; out.ended = true;
        out.flag_set = 3; out.release_set = true; out.spawn_set = 0;
        break;
    default: break;
    }
    if (end_check && at_end(ev)) { message(out, open, 4); ++ev.state; }   // 0x80102088
    // 0x80102128: the current record's callback from state 2 on.
    if (ev.state >= 2 && !at_end(ev) && ev.timeline->records[ev.record][1] == kRecordCallback) {
        if (ev.camera_program) step_source_camera_program_8001ec0c(ev.camera, *ev.camera_program, ev.counters);
        step_source_timeline_8001f070(ev.counters, ev.record, *ev.timeline);
    }
    // States 3..10: Start skips the scene.
    if (ev.state >= 3 && ev.state <= 10 && in.start_pressed) {
        out.skipped = true; out.flag_set = 10;
        ev.state = 11; ev.countdown = 1;
        target(out, false, uint16_t(1255));
    }
}

// Scene 1 (0x80102224, states via table 0x80100034): the police car.
void scene1(St03Event8& ev, const St03Event8Profile& p, const St03Event8Inputs& in, St03Event8Step& out) noexcept {
    bool open = in.message_open;
    const auto place = [&](std::optional<uint16_t> heading) {
        St03PlayerTarget t; t.position = kPoliceTarget; t.heading = heading; out.player = t;   // 0x80030150
    };
    const auto finish = [&] {   // 0x8010251C
        out.flag_set = 4; out.release_set = true; out.spawn_set = 1;
    };
    switch (ev.state) {
    case 0: case 1:   // 0x8010226C
        if (!in.player_ready) break;
        ev.byte_07 = 10; ev.state = uint8_t(ev.state + 2);
        { St03PlayerTarget t; t.from_pose = true; t.walk = true; out.player = t; }   // +8 = 3, +9 = 1, +0xBD |= 0x10
        out.player_anim_448 = 48;
        break;
    case 2:   // 0x80102310
        if (ev.byte_07) { --ev.byte_07; break; }
        if (!in.player_on_floor) break;
        if (in.turn_delta != 0) { out.player_anim_448 = 13; out.player_heading_add = in.turn_delta; break; }
        out.player_anim_448 = 48; ++ev.state; message(out, open, 31);
        break;
    case 3:   // 0x80102384
        if (open) break;
        out.sound_8001a008 = 270;
        begin(ev, out, p.police, p.police_camera);
        place(uint16_t(2240));
        out.objects[3].state_09 = 1; out.objects[3].clear_0a_0b = true; out.objects[3].heading_56 = 1536;
        ev.byte_07 = 10; ++ev.state;
        break;
    case 4: {  // 0x80102450
        if (uint8_t(header(ev) >> 8) == 1) {
            if (ev.byte_07) --ev.byte_07; else out.player_heading_add = 32;
        } else if (at_end(ev)) { ++ev.state; message(out, open, 32); }
        break;
    }
    case 5:   // 0x801024C8
        if (open) break;
        ev.state = 7; out.player_heading_zero = true; out.flag_8009395c = true; finish();
        break;
    case 6:   // 0x80102508
        ++ev.state; out.player_heading_zero = true; finish();
        break;
    case 7:   // 0x8010253C
        ev.active = false; out.player_release = true; out.ended = true;
        break;
    default: break;
    }
    if (ev.state == 4 && !at_end(ev) && ev.timeline->records[ev.record][1] == kRecordCallback) {
        if (ev.camera_program) step_source_camera_program_8001ec0c(ev.camera, *ev.camera_program, ev.counters);
        step_source_timeline_8001f070(ev.counters, ev.record, *ev.timeline);
    }
    if (ev.state >= 2 && ev.state <= 5 && in.start_pressed) {   // 0x8010259C
        out.skipped = true;
        ev.state = ev.state < 4 ? 6 : 5;
        place(std::nullopt);
    }
}

} // namespace

bool bind_st03_event8_profile(const SourceScenePackage& package, St03Event8Profile& out) {
    out = {};
    if (!package.loaded || package.event != 8 || package.stage != 3) return false;
    const auto* a = package.timeline(kIntroTimeline);
    const auto* b = package.timeline(kCrashTimeline);
    const auto* ca = package.camera(kIntroCamera);
    const auto* cb = package.camera(kCrashCamera);
    if (!a || !b || !ca || !cb || a->records.empty() || b->records.empty()) return false;
    out.intro = *a; out.crash = *b; out.intro_camera = *ca; out.crash_camera = *cb;
    out.loaded = true;
    const auto* pt = package.timeline(kPoliceTimeline);
    const auto* pc = package.camera(kPoliceCamera);
    if (pt && pc && !pt->records.empty()) { out.police = *pt; out.police_camera = *pc; out.police_loaded = true; }
    for (uint8_t n = 0; n < out.object_sets.size(); ++n) out.object_sets[n] = st03_object_set(package, n);
    if (const auto* t = package.data_at(kSt03NpcIdleTimers); t && t->bytes.size() >= 64) {
        for (size_t i = 0; i < 64; ++i) out.idle_timers[i] = t->bytes[i];
        out.idle_timers_loaded = true;
    }
    const auto* ws = package.data_at(kSt03NpcWalkSpeeds); const auto* wt = package.data_at(kSt03NpcWalkTables);
    if (ws && wt && ws->bytes.size() >= 112 && wt->bytes.size() >= 112) {
        for (size_t i = 0; i < 112; ++i) { out.walk_speeds[i] = ws->bytes[i]; out.walk_tables[i] = wt->bytes[i]; }
        out.walk_loaded = true;
    }
    return true;
}

St03Event8 start_st03_event8(uint8_t scene) noexcept {
    St03Event8 ev; ev.active = true; ev.scene = scene;
    return ev;
}

St03Event8Step step_st03_event8(St03Event8& ev, const St03Event8Profile& profile, const St03Event8Inputs& in) noexcept {
    St03Event8Step out;
    if (!ev.active) return out;
    ++ev.ticks;
    if (!profile.loaded || (ev.scene == 1 && !profile.police_loaded) || ev.scene > 1) { out.unsupported_scene = true; return out; }
    if (ev.scene == 0) scene0(ev, profile, in, out); else scene1(ev, profile, in, out);
    return out;
}

} // namespace l3

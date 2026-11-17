#include "legends3ds/source_st03_event8.h"

namespace l3 {
namespace {

constexpr uint32_t kIntroTimeline = 0x80115FB4u, kCrashTimeline = 0x8011600Cu;
constexpr uint32_t kIntroCamera = 0x80115FC4u, kCrashCamera = 0x80116034u;
constexpr uint32_t kRecordCallback = 0x80103A88u;   // 0x8001EC0C + 0x8001F070
constexpr std::array<int16_t,3> kCrashTarget{{-4584, -1, 1431}};   // 0xEE180000, 0xFFFF0000, 0x05970000

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
    if (ev.scene != 0 || !profile.loaded) { out.unsupported_scene = true; return out; }
    scene0(ev, profile, in, out);
    return out;
}

} // namespace l3

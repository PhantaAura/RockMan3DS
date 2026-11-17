#include "legends3ds/source_st03_event8.h"

namespace l3 {
namespace {

constexpr uint32_t kIntroTimeline = 0x80115FB4u, kCrashTimeline = 0x8011600Cu;
constexpr uint32_t kIntroCamera = 0x80115FC4u, kCrashCamera = 0x80116034u;
constexpr uint32_t kRecordCallback = 0x80103A88u;   // 0x8001EC0C + 0x8001F070
constexpr std::array<int16_t,3> kCrashTarget{{-4584, -1, 1431}};   // 0xEE180000, 0xFFFF0000, 0x05970000
constexpr uint32_t kPoliceTimeline = 0x8011614Cu, kPoliceCamera = 0x80116164u;
constexpr std::array<int16_t,3> kPoliceTarget{{-2496, -1, 5065}};  // 0xF6400000, 0xFFFF0000, 0x13C90000
constexpr uint32_t kInspectorTimeline = 0x801161D0u, kInspectorCamera = 0x801161F8u;
constexpr std::array<int16_t,3> kInspectorWalk{{-2924, -1, 1164}};   // 0xF4940000, 0xFFFF0000, 0x048C0000
constexpr std::array<int16_t,3> kInspectorEnd{{-2601, -1, 1108}};    // 0xF5D70000, 0xFFFF0000, 0x04540000

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

// Scene 2 (0x8010263C, states via table 0x80100054): the Inspector talk.
// Slots of set 1: 1 Barrell, 3 the Inspector, 4 the officer, 5 the car.
void scene2(St03Event8& ev, const St03Event8Profile& p, const St03Event8Inputs& in, St03Event8Step& out) noexcept {
    bool open = in.message_open;
    const auto behaviour = [&](size_t slot, uint8_t s9) { out.objects[slot].state_09 = s9; out.objects[slot].clear_0a_0b = true; };
    const auto clear_flag = [&](uint16_t f) { if (out.flag_clear_count < out.flag_clears.size()) out.flag_clears[out.flag_clear_count++] = f; };
    const auto release = [&] { out.release_slots = uint16_t((1u << 3) | (1u << 4) | (1u << 1)); };
    switch (ev.state) {
    case 0: {   // 0x80102684: the focus camera on Barrell; the player hidden
        St03Event8Step::CameraFocus f;
        f.point = {in.barrell[0], int16_t(in.barrell[1] - 80), in.barrell[2]};
        f.distance = 300; f.yaw = uint16_t(in.heading_barrell_to_player + 32);
        out.camera_focus = f; out.player_hide = true; ++ev.state;
        break;
    }
    case 1:     // 0x80102714
        if (in.camera_busy) break;
        ++ev.state; message(out, open, 45);
        break;
    case 2: {   // 0x80102740
        if (open || !in.player_ready) break;
        out.camera_restore_80039e50 = true; ++ev.state;
        begin(ev, out, p.inspector, p.inspector_camera);
        message(out, open, 46);
        St03PlayerTarget t; t.position = kInspectorWalk; t.walk = true; t.heading = uint16_t(2092); out.player = t;
        out.player_anim_448 = 48;
        out.release_set = true; out.spawn_set = 1;
        clear_flag(0x606); clear_flag(0x607);
        for (size_t slot : {size_t(3), size_t(4), size_t(1)}) { out.objects[slot].variant_04 = 1; out.objects[slot].state_08 = 0; }
        break;
    }
    case 3:     // 0x8010287C
        if (open) break;
        ++ev.state; out.player_show = true; next_record_8001f158(ev);
        behaviour(3, 1); out.objects[1].state_09 = 2; out.objects[1].byte_0c_from = 3;
        break;
    case 4:     // 0x801028F0
        if (!in.flag_606) break;
        clear_flag(0x606); ++ev.state; message(out, open, 47);
        behaviour(3, 3); out.objects[1].state_09 = 0;
        break;
    case 5:     // 0x80102944
        if (open) break;
        ++ev.state; message(out, open, 49);
        behaviour(3, 0); behaviour(4, 2);
        break;
    case 6:     // 0x8010299C
        if (open) break;
        ++ev.state; message(out, open, 51);
        behaviour(3, 3); behaviour(4, 0);
        break;
    case 7:     // 0x801029F4
        if (open) break;
        ++ev.state; behaviour(3, 2); out.objects[3].byte_0c_from = 4; out.objects[3].byte_0d_from = 5;
        ev.countdown = 200;
        break;
    case 8:     // 0x80102A6C
        if (--ev.countdown == 140) { behaviour(1, 1); out.objects[1].byte_0c_from = 5; }
        if (ev.countdown == 80) { ++ev.state; behaviour(4, 1); out.objects[4].byte_0c_from = 5; }
        break;
    case 9:     // 0x80102AF8
        if (--ev.countdown >= 0) break;
        next_record_8001f158(ev); ++ev.state;
        { St03PlayerTarget t; t.position = kInspectorEnd; out.player = t; }
        out.player_heading_56 = 1740;
        release(); out.sound_8001a008 = 271; ev.countdown = 180;
        out.objects[5].heading_56 = 2048; out.objects[5].byte_0f_or = 0x40; out.objects[5].state_09 = 1;
        out.objects[5].clear_0a_0b = true;
        break;
    case 10:    // 0x80102BD8
        if (--ev.countdown >= 0) break;
        out.player_heading_442 = 2048; ++ev.state;
        break;
    case 11:    // 0x80102C08
        out.player_release = true; out.flag_8009395c = true;   // 0x800A395C = 1, then 0x80039820
        out.release_slots = uint16_t(out.release_slots | (1u << 5));
        out.flag_set = 0; ev.active = false; out.ended = true;
        break;
    case 12:    // 0x80102C40 (skipped before the talk)
        out.camera_restore_80039e50 = true; out.player_release = true;
        out.flag_set = 0; ev.active = false; out.ended = true; out.player_show = true;
        break;
    default: break;
    }
    // 0x80102C74: the record callback for states 3..10.
    if (ev.active && ev.state >= 3 && ev.state <= 10 && !at_end(ev) && ev.timeline->records[ev.record][1] == kRecordCallback) {
        if (ev.camera_program) step_source_camera_program_8001ec0c(ev.camera, *ev.camera_program, ev.counters);
        step_source_timeline_8001f070(ev.counters, ev.record, *ev.timeline);
    }
    if (ev.active && ev.state < 11 && in.start_pressed) {   // 0x80102CC8: the skip
        out.release_slots = uint16_t((1u << 3) | (1u << 4) | (1u << 1) | (1u << 5));
        out.skipped = true;
        ev.state = ev.state < 2 ? 12 : 11;
        St03PlayerTarget t; t.position = kInspectorEnd; t.walk = true; t.heading = uint16_t(2048); out.player = t;
        out.player_anim_448 = 0;
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
    const auto* it = package.timeline(kInspectorTimeline);
    const auto* ic = package.camera(kInspectorCamera);
    if (it && ic && !it->records.empty()) { out.inspector = *it; out.inspector_camera = *ic; out.inspector_loaded = true; }
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
    if (const auto* d = package.data_at(kSt03Doors); d && d->bytes.size() >= 128) {
        for (size_t i = 0; i < 8; ++i) out.doors[i] = d->bytes[i];
        for (size_t r = 0; r < out.door_states.size(); ++r)
            for (size_t i = 0; i < 8; ++i) out.door_states[r][i] = d->bytes[8 + r * 8 + i];
        out.doors_loaded = true;
    }
    if (const auto* cp = package.data_at(kSt03CarPaths); cp && cp->bytes.size() >= 88) {
        for (size_t i = 0; i < 88; ++i) out.car_paths[i] = cp->bytes[i];
        out.car_paths_loaded = true;
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
    if (!profile.loaded || (ev.scene == 1 && !profile.police_loaded) || (ev.scene == 2 && !profile.inspector_loaded) || ev.scene > 2) {
        out.unsupported_scene = true; return out;
    }
    if (ev.scene == 0) scene0(ev, profile, in, out);
    else if (ev.scene == 1) scene1(ev, profile, in, out);
    else scene2(ev, profile, in, out);
    return out;
}

} // namespace l3

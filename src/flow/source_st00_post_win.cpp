#include "legends3ds/source_st00_post_win.h"

namespace l3 {
namespace {
constexpr uint32_t kTimeline3 = 0x8011BE44u, kCamera3 = 0x8011BB3Cu;
constexpr uint32_t kSpawn3a = 0x8011BB14u, kSpawn3b = 0x8011BB28u;
constexpr uint32_t kTimeline2 = 0x8011BA4Cu, kCamera2 = 0x8011B4B4u, kSpawns2 = 0x8011B388u;
constexpr size_t kSpawnCount2 = 14;

int16_t rec16(const std::array<uint8_t,20>& r, size_t at) noexcept {
    return int16_t(uint16_t(r[at] | (uint16_t(r[at + 1]) << 8)));
}
// 0x8001EAE8 as far as the event needs it: the slot byte and the placement.
uint8_t spawn_record(SourcePostWinEvent& ev, const std::array<uint8_t,20>& r) noexcept {
    const uint8_t slot = r[0];
    if (slot >= ev.slots.size()) return slot;
    auto& s = ev.slots[slot];
    s = {};
    s.spawned = true; s.object_class = r[1]; s.family = r[3]; s.byte_04 = r[4];
    s.spawn = {{rec16(r, 12), rec16(r, 14), rec16(r, 16)}};
    s.spawn_heading = uint16_t(rec16(r, 18));
    return slot;
}
std::array<int16_t,3> slot_pos(const SourcePostWinEvent& ev, const SourcePostWinInputs& in, size_t slot) noexcept {
    if (slot < in.slot_position.size() && in.slot_position[slot]) return *in.slot_position[slot];
    return slot < ev.slots.size() ? ev.slots[slot].spawn : std::array<int16_t,3>{};
}
void state(SourcePostWinEvent& ev, size_t slot, uint8_t s9) noexcept {
    auto& s = ev.slots[slot]; s.state_09 = s9; s.sub_0a = 0; s.writes += 2;
}
SourcePostWinHandoff handoff(uint8_t stage, uint8_t area, uint16_t flag) noexcept {
    SourcePostWinHandoff h;
    // 3560..3565 = 0, 356A = 0, 356C = 2, 356D = 0, 356E stage, 356F area, 3570/3571 = 0xFF.
    h.raw[12] = 2; h.raw[14] = stage; h.raw[15] = area; h.raw[16] = h.raw[17] = 0xFF;
    h.stage = stage; h.area = area; h.cleared_flag = flag;
    return h;
}
} // namespace

bool bind_source_post_win_profile(const SourceScenePackage& p, uint8_t event, SourcePostWinProfile& out) {
    out = {};
    if (!p.loaded || p.stage != 0 || p.event != event || (event != 3 && event != 2)) return false;
    const auto* t = p.timeline(event == 3 ? kTimeline3 : kTimeline2);
    if (!t || t->records.empty()) return false;
    out.timeline = *t;
    if (const auto* c = p.camera(event == 3 ? kCamera3 : kCamera2)) out.camera = *c;
    if (event == 3) {
        if (!out.camera) return false;   // the cooker accepts event 3's program
        for (const uint32_t token : {kSpawn3a, kSpawn3b}) {
            bool found = false;
            for (const auto& s : p.spawns) if (s.token == token) { out.spawns.push_back(s.bytes); found = true; }
            if (!found) return false;
        }
    } else {
        const auto* d = p.data_at(kSpawns2);
        if (!d || d->bytes.size() < kSpawnCount2 * 20) return false;
        for (size_t i = 0; i < kSpawnCount2; ++i) {
            std::array<uint8_t,20> r{};
            for (size_t b = 0; b < 20; ++b) r[b] = d->bytes[i * 20 + b];
            out.spawns.push_back(r);
        }
    }
    out.event = event; out.loaded = true;
    return true;
}

SourcePostWinEvent start_source_post_win_event(uint8_t event) noexcept {
    SourcePostWinEvent ev;
    ev.event = event; ev.active = true;
    return ev;
}

void run_source_post_win_callback(SourcePostWinEvent& ev, uint32_t callback, const SourcePostWinInputs& in,
                                  SourcePostWinStep& out) noexcept {
    const uint32_t t = ev.counters.c10;
    auto& s0 = ev.slots[0]; auto& s1 = ev.slots[1];
    switch (callback) {
    // ---- event 3 (slot 0 = 40/05 scene Mega Man, slot 1 = 40/08) ----
    case 0x80101B6Cu: if (t == 15) state(ev, 1, 1); break;
    case 0x80101B9Cu: {
        const auto p = slot_pos(ev, in, 1);   // read before the t == 0 write, as the source does
        if (t == 0) state(ev, 1, 3);
        const auto smoke = [&](int16_t x, int16_t y, int16_t z) {
            if (out.smoke_count < out.smoke.size()) out.smoke[out.smoke_count++] = {{x, y, z}};
        };
        // 0x80116D60(pos, 1, 1, 0, -2, 0) into 0x8012243C..0x8012244C.
        if (t == 0xE6) { smoke(int16_t(p[0] - 0x40), int16_t(p[1] - 0x174), int16_t(p[2] + 0x58)); smoke(-0x15E, int16_t(0xFDDA), int16_t(0xFC7C)); }
        if (t == 0xF0) smoke(int16_t(p[0] + 0x7E), int16_t(p[1] - 0x180), int16_t(p[2] + 0x28));
        if (t == 0xF5) smoke(int16_t(p[0] + 0x38), int16_t(p[1] - 0xE8), int16_t(p[2] + 0x78));
        if (t == 0x104) smoke(int16_t(p[0] - 0x28), int16_t(p[1] - 0x128), int16_t(p[2] + 0xC4));
        break;
    }
    case 0x80101D88u: if (t == 0x23) { s1.byte_128 = 0x0C; ++s1.writes; } break;
    case 0x80101DACu: if (t == 0) state(ev, 0, 1); break;
    case 0x80101DE4u:
        if (t == 0) { state(ev, 1, 0); state(ev, 0, 0); out.smoke_release = true; }
        if (t == 0x3C) {
            SourcePostWinObject o; o.b00 = 1; o.b02 = 5; o.b0c = 5; o.b0e = 2; o.b0d = 0;
            o.pos = {{int16_t(0xFF00), int16_t(0xFD28), int16_t(0xFC90)}};
            out.object = o;
        }
        break;
    case 0x80101EF0u:
        if (t == 0) { state(ev, 1, 2); s1.byte_128 = 0x0D; ++s1.writes; out.sound_8002ee90 = std::array<uint16_t,2>{{0x40, 0x80}}; }
        break;
    case 0x80101F44u: if (t == 0) state(ev, 0, 0); break;
    // ---- event 2 (slots 0..4 = +0x24..+0x34) ----
    case 0x80101364u:
        if (t == 0) { s0.colour_15e = 0x2108; ++s0.writes; }
        if (t == 0x14A) { ev.slots[3].byte_0c = 1; ev.slots[2].byte_0c = 1; }
        if (t == 0x1AE) state(ev, 0, 1);
        if (t == 0x1B8) { s0.byte_0b = 1; ++s0.writes; }
        if (t == 0x1EF) { ev.slots[3].byte_0c = 2; ev.slots[2].byte_0c = 2; }
        if (t == 0x219) state(ev, 0, 2);
        break;
    case 0x8010141Cu:
        if (t == 0) { s1.flags_00 |= 2; state(ev, 0, 3); }
        if (t == 0x82) { ev.slots[3].byte_0c = 1; ev.slots[2].byte_0c = 1; }
        break;
    case 0x8010148Cu: if (t == 0) state(ev, 0, 4); break;
    case 0x801014BCu: if (t == 0) state(ev, 0, 5); break;
    case 0x801014ECu:
        if (t == 0) { s1.fixed_1c = 0x01F40000; s1.fixed_20 = int32_t(0xE3FF0000u); s1.fixed_24 = 0x01800000; s1.colour_15e = 0x4210; s1.writes += 4; }
        break;
    case 0x80101528u:
        if (t == 0) {
            SourcePostWinObject o; o.b00 = 1; o.b02 = 5; o.b0c = 5; o.b0e = 2; o.b0d = 0;
            o.pos = {{500, int16_t(0xE2B0), 0xEC}};
            out.object = o;
        }
        break;
    case 0x80101598u: if (t == 0) state(ev, 0, 6); break;
    case 0x801015C8u:
        if (t == 0) { s0.heading_56 = 0x800; state(ev, 0, 7); }
        if (t == 0x2D) state(ev, 1, 1);
        break;
    case 0x80101620u:
        if (t == 0) { state(ev, 0, 8); s0.fixed_24 = int32_t(0xFC200000u); state(ev, 1, 0); }
        break;
    case 0x80101680u: if (t == 0x1E) state(ev, 0, 9); break;
    case 0x801016B0u:
        if (t == 0) { state(ev, 1, 3); s1.fixed_24 = 0; }
        if (t == 0x3C) state(ev, 1, 0);
        break;
    case 0x801016ECu:
        if (t == 0) {
            out.fx_8011877c[0] = {{0x260, int16_t(58000), int16_t(0xFF10), 0x250, 0x50}};
            out.fx_8011877c[1] = {{0x1D0, int16_t(58000), int16_t(0xFF50), 0x250, 0x8E0}};
            out.fx_8011877c_count = 2;
        }
        break;
    case 0x80101764u: if (t == 0) { s0.fixed_24 = int32_t(0xFC380000u); state(ev, 0, 10); } break;
    case 0x80101794u:
        if (t == 0x3C) { state(ev, 0, 11); ev.slots[4].flags_00 |= 2; ev.slots[4].state_09 = 1; ++ev.slots[4].writes; }
        break;
    case 0x801017E8u: if (t == 0) { s1.fixed_24 = int32_t(0xFE400000u); ++s1.writes; } break;
    case 0x8010180Cu: if (t == 0) state(ev, 1, 2); break;
    case 0x80101844u: if (t == 0) { state(ev, 1, 0); ev.slots[4].sub_0a = 2; ++ev.slots[4].writes; } break;
    // Empty callbacks (jr ra): event 3 0x80101D80/0x80101DDC/0x80101F74;
    // event 2 0x80101618/0x80101660/0x80101668/0x80101670/0x8010178C/0x8010183C/0x80101880.
    default: break;
    }
}

namespace {
// 0x80102018 / 0x8010190C, then 0x8001EC0C and 0x8001F070.
void scene_tick(SourcePostWinEvent& ev, const SourcePostWinProfile& profile, const SourcePostWinInputs& in,
                SourcePostWinStep& out) noexcept {
    if (ev.counters.e8 != 0xFF && ev.record < profile.timeline.records.size()) {
        out.callback = profile.timeline.records[ev.record][1];
        run_source_post_win_callback(ev, out.callback, in, out);
    }
    if (profile.camera) step_source_camera_program_8001ec0c(ev.camera, *profile.camera, ev.counters);
    step_source_timeline_8001f070(ev.counters, ev.record, profile.timeline);
}
} // namespace

SourcePostWinStep step_source_post_win_event(SourcePostWinEvent& ev, const SourcePostWinProfile& profile,
                                             const SourcePostWinInputs& in) noexcept {
    SourcePostWinStep out;
    if (!ev.active || !profile.loaded || profile.event != ev.event) return out;
    ++ev.ticks;
    // 0x80017C30 starts the fade and raises 0x80098A72 at once, so a check
    // later in the same tick reads it busy.
    uint16_t busy = in.fade_busy;
    const auto fade = [&](uint8_t type) { out.fade_type = type; busy = 0xFFFF; };
    const auto finish = [&](uint8_t stage, uint8_t area, uint16_t flag) {
        out.handoff = handoff(stage, area, flag);
        ev.active = false;   // *event = 0
    };
    if (ev.event == 3) {
        switch (ev.phase) {
        case 0: if (in.player_ready) ev.phase = 1; break;   // 0x8005D7C0 (not ported) precedes the check
        case 1: out.stop_sequences = true; out.cd_request = 0x20000001u; ev.phase = 2; break;
        case 2: if (in.cd_done) { ev.countdown = 30; ev.phase = 3; } break;
        case 3: if (--ev.countdown == 0) ev.phase = 4; break;
        case 4: case 5: case 6:
            if (ev.phase == 4) {
                // 0x80101F7C.
                out.player_hidden = true; out.camera_reset_8001e6c0 = true;
                out.camera_program = kCamera3; ev.camera = {};
                for (const auto& r : profile.spawns) out.spawned_slots[out.spawn_count++] = spawn_record(ev, r);
                ev.record = 0; ev.constructed = true; out.constructed = true;
                ev.countdown = 0;   // +0x0B, +0x0A and +0x0C = 0
                ev.phase = 5;
            }
            if (ev.phase == 5 && busy == 0 && (ev.counters.e8 == 0xFF || in.start_skip)) { fade(10); ev.phase = 6; }
            scene_tick(ev, profile, in, out);
            // 0x8005D9EC (not ported) precedes the stores.
            if (ev.phase == 6 && busy == 0) finish(0, 4, 0x606);
            break;
        default: break;
        }
    } else {
        switch (ev.phase) {
        case 0: if (busy == 0) ev.phase = 1; break;
        case 1: if (in.player_ready) { out.player_hidden = true; ev.phase = 2; } break;
        case 2: out.stop_sequences = true; out.cd_request = 0x20000002u; ev.phase = 3; break;
        case 3: if (in.cd_done) { ev.countdown = 30; ev.phase = 4; } break;
        case 4: if (--ev.countdown == 0) { fade(2); ev.phase = 5; } break;
        case 5:
            // 0x80101888.
            out.camera_reset_8001e6c0 = true; out.camera_program = kCamera2; ev.camera = {};
            for (const auto& r : profile.spawns) {
                const uint8_t slot = spawn_record(ev, r);
                if (out.spawn_count < out.spawned_slots.size()) out.spawned_slots[out.spawn_count] = slot;
                ++out.spawn_count;
            }
            ev.record = 0; ev.constructed = true; out.constructed = true;
            ev.phase = 6;
            break;
        case 6: case 7:
            if (ev.phase == 6 && busy == 0 && (in.start_skip || ev.counters.e8 == 0xFF)) { fade(10); ev.phase = 7; }
            scene_tick(ev, profile, in, out);
            if (ev.phase == 7 && busy == 0) finish(2, 0, 0x605);
            break;
        default: break;
        }
    }
    return out;
}
} // namespace l3

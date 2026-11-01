#include "legends3ds/source_attract_event10.h"
#include "legends3ds/source_scene_face.h"

namespace l3 {
namespace {
uint16_t rd16(const uint8_t* p) noexcept { return uint16_t(p[0] | (p[1] << 8)); }
} // namespace

bool bind_source_attract_profile(const SourceScenePackage& pkg, SourceAttractProfile& out) {
    if (!pkg.loaded || pkg.event != 10 || pkg.stage != 7) return false;
    const auto* t0 = pkg.timeline(0x80105E1Cu); const auto* t1 = pkg.timeline(0x801060D4u);
    const auto* c0 = pkg.camera(0x80105DD0u); const auto* c1 = pkg.camera(0x80105E2Cu);
    const auto* rows = pkg.data_at(0x80106124u);
    if (!t0 || !t1 || !c0 || !c1 || !rows || rows->bytes.size() != 64) return false;
    SourceAttractProfile p;
    p.timelines = {{*t0, *t1}}; p.cameras = {{*c0, *c1}};
    for (size_t i = 0; i < 8; ++i) for (size_t k = 0; k < 8; ++k) p.bg_rows[i][k] = rows->bytes[i * 8 + k];
    p.area_requests = pkg.area_requests;
    // Spawn records (--extra 80105B8C:580) and the Servbot behaviour table
    // (--extra 8010663C:100); both optional for the logic, required for the cast.
    if (const auto* rec = pkg.data_at(0x80105B8Cu))
        for (size_t k = 0; k + 20 <= rec->bytes.size(); k += 20) {
            std::array<uint8_t,20> r{}; for (size_t i = 0; i < 20; ++i) r[i] = rec->bytes[k + i]; p.records.push_back(r);
        }
    if (const auto* face = pkg.data_at(0x80106914u)) p.teisel_face = face->bytes;   // --extra 80106914:72
    if (const auto* beh = pkg.data_at(0x8010663Cu))
        for (size_t k = 0; k + 4 <= beh->bytes.size(); k += 4)
            p.servbot_behaviours.push_back(uint32_t(beh->bytes[k]) | (uint32_t(beh->bytes[k + 1]) << 8) |
                                           (uint32_t(beh->bytes[k + 2]) << 16) | (uint32_t(beh->bytes[k + 3]) << 24));
    p.loaded = true;
    out = std::move(p);
    return true;
}

namespace {
// The live record's callback (0x8010036C sub 1/2 and 0x80100598 call
// event+0xA4[+4] while the record byte is not 0xFF). Background/area parts.
uint32_t run_callback(SourceAttractEvent10State& st, const SourceAttractProfile& prof, SourceBgLayers& L,
                      SourceAttractEvent10Step& out) noexcept {
    const auto& t = prof.timelines[st.timeline];
    if (st.record >= t.records.size() || uint8_t(t.records[st.record][0]) == 0xFF) return 0;
    const uint32_t cb = t.records[st.record][1];
    const bool first = st.event.c10 == 0;
    const auto flags = [&](size_t layer, uint8_t value) { if (L[layer].present) L[layer].flags = value; };
    switch (cb) {
    case 0x801007E0u:   // frame counter; at 222 the cloud layers 2/4/5/6 hide
        if (st.callback_counter_80108b0c == 222) { flags(2, 0); flags(4, 0); flags(5, 0); flags(6, 0); }
        ++st.callback_counter_80108b0c;
        break;
    case 0x80100834u:   // top-down shot: side sea off, top sea and cloud strips on
        if (first) { flags(0, 0); flags(1, 3); flags(3, 3); flags(7, 3); }
        break;
    case 0x8010086Cu:   // strips off; actor spawns at +0x10 == 130 (not ported)
        if (first) { flags(3, 0); flags(7, 0); }
        if (st.event.c10 == 130) ++out.actor_callbacks;
        break;
    // First tick: release the previous slots (0x80031824 on event+0x24..),
    // spawn consecutive records (0x8001EAE8). Record indices into 0x80105B8C.
    case 0x80100944u: if (first) { out.release_slots = 1; out.spawn_first = 1; out.spawn_count = 3; } break;
    case 0x801009B8u:   // also releases the 64-entry pool 0x800A3B40; two objects at 130 (not ported)
        if (first) { out.release_slots = 3; out.spawn_first = 4; out.spawn_count = 3; }
        if (st.event.c10 == 130) ++out.actor_callbacks;
        break;
    case 0x80100B14u: if (first) { out.release_slots = 3; out.spawn_first = 7; out.spawn_count = 5; } break;
    case 0x80100B98u: if (first) { out.release_slots = 5; out.spawn_first = 12; out.spawn_count = 3; } break;
    case 0x80100C4Cu: if (first) { out.release_slots = 3; out.spawn_first = 15; out.spawn_count = 14; } break;
    default: break;
    }
    // The importer's callback -> area requests (0x80100CD4 on the first tick).
    if (first) for (const auto& a : prof.area_requests) if (a.callback == cb) out.area_request = int8_t(a.area);
    return cb;
}
} // namespace

SourceAttractEvent10Step step_source_attract_event10_80100330(SourceAttractEvent10State& st,
    const SourceAttractProfile& prof, SourceBgLayers& L, bool cd_done, int16_t busy, bool start) noexcept {
    SourceAttractEvent10Step out;
    if (!prof.loaded || st.done) return out;
    bool tail = true;   // 0x8010054C / 0x8010068C: Start bit, camera, timeline
    switch (st.phase) {
    case 0:
        if (st.sub == 0) {   // 0x801003B8
            st.camera = SourceCameraBlock{};   // 0x8001E6C0(0, 0, 0)
            st.camera_program = 0; st.camera.pc = 0;
            st.timeline = 0; st.record = 0;
            st.flags_0a = 128; st.count_0b = 0;
            for (size_t i = 0; i < 8; ++i) {   // 0x80106124 rows -> +0, +0x20, +0x22
                const auto& row = prof.bg_rows[i];
                L[i].flags = row[0];
                L[i].vx = int16_t(rd16(&row[4])); L[i].vy = int16_t(rd16(&row[6]));
            }
            L[7].y = 240; L[7].y16 = 0x00F00000;   // 0x800BBC26 / 0x800BBC20
            st.callback_counter_80108b0c = 0;
            out.cd_command = true;
            ++st.sub;
        } else if (st.sub == 1) {
            out.callback = run_callback(st, prof, L, out);
            if (cd_done) ++st.sub;
        } else {
            out.callback = run_callback(st, prof, L, out);
            if (st.count_0b == 10) out.fade_type = 2;
            if (st.count_0b == 30) {
                st.event.c10 = 0; st.event.c18 = 0; st.event.c1c = 0; st.event.e8 = 0; st.event.e9 = 0;   // 0x8001F1DC
                st.camera_program = 1; st.camera.pc = 0;
                st.timeline = 1; st.record = 0;
                st.phase = 1; st.sub = 0;
            }
            ++st.count_0b;
        }
        break;
    case 1:
        if (st.sub == 0) {   // 0x801005C8
            out.callback = run_callback(st, prof, L, out);
            if (busy == 0) { ++st.sub; st.flags_0a &= 127; }
        } else if (st.event.e8 == 0xFF) {   // 0x80100610
            out.fade_type = 20; st.phase = 2; st.sub = 0; st.flags_0a |= 128;
        } else {
            out.callback = run_callback(st, prof, L, out);
            if ((st.flags_0a & 1) && !(st.flags_0a & 128)) { out.fade_type = 10; st.phase = 2; st.sub = 0; }
        }
        break;
    case 2:   // 0x801006D8: the fade ends, the CD/sounds stop, the title thread
        tail = false;
        if (st.sub == 0) {
            if (busy == 0) ++st.sub;
            // 0x8010074C: a skipped scene keeps its camera moving.
            if ((st.flags_0a & 1) && !(st.flags_0a & 128)) step_source_camera_program_8001ec0c(st.camera, prof.cameras[st.camera_program], st.event);
        } else st.done = true;
        break;
    default: break;
    }
    if (tail) {
        if (start) st.flags_0a |= 1;
        step_source_camera_program_8001ec0c(st.camera, prof.cameras[st.camera_program], st.event);
        step_source_timeline_8001f070(st.event, st.record, prof.timelines[st.timeline]);
    }
    return out;
}

bool spawn_source_attract_servbot(const std::array<uint8_t,20>& r, const SourceAttractProfile& prof,
                                  SourceAttractServbot& out) noexcept {
    if (r[3] != 0x20 || r[1] != 0x00 || r[2] != 4) return false;
    SourceAttractServbot s;
    s.slot = r[0]; s.id = r[8]; s.request = r[9]; s.big = r[10]; s.byte_0f = r[11];
    for (size_t i = 0; i < 3; ++i) { s.raw[i] = int16_t(rd16(&r[12 + 2 * i])); s.fixed[i] = int32_t(uint32_t(int32_t(s.raw[i])) << 16); }
    s.heading = rd16(&r[18]);
    if (s.id >= prof.servbot_behaviours.size()) return false;
    s.behaviour = prof.servbot_behaviours[s.id];
    out = s;
    return true;
}

SourceAttractAnimation update_source_attract_servbot_8010205c(SourceAttractServbot& s, uint32_t t, const ReferenceTables& refs) noexcept {
    SourceAttractAnimation a;
    if (s.state == 0) s.state = 1;   // 0x8010212C (flags, model, scale by +0xE, floor query) then the behaviour
    const auto move = [&](int16_t forward) {   // 0x8002FEA4(work, forward, 0, 0)
        const auto d = reference_player_motion_delta(refs, s.heading, forward, 0, 0);
        const std::array<int32_t,3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
        for (size_t i = 0; i < 3; ++i) {
            s.fixed[i] = int32_t(uint32_t(s.fixed[i]) + uint32_t(delta[i]));
            s.raw[i] = reference_fixed_to_short_80030150(s.fixed[i]);
        }
    };
    const auto animate = [&](uint8_t request) {   // +9 == 0: 0x800322A8(request); else 0x80032488
        if (!s.started) { a.reset = true; a.request = request; s.started = 1; } else a.step = true;
    };
    const int32_t c = int32_t(t);
    switch (s.behaviour) {
    case 0x801021DCu: animate(s.request); break;
    case 0x80102238u: animate(s.request); a.shadow = true; break;
    case 0x801022B8u:
        if (c < 20) { s.raw[2] = int16_t(s.raw[2] - 3); s.fixed[2] = int32_t(uint32_t(s.fixed[2]) + 0xFFFC6A7Fu); }
        else if (c < 40) {
            s.raw[0] = int16_t(s.raw[0] + 1); s.raw[2] = int16_t(s.raw[2] - 1);
            s.fixed[0] = int32_t(uint32_t(s.fixed[0]) + 0x00019581u); s.fixed[2] = int32_t(uint32_t(s.fixed[2]) - 0x00019581u);
        }
        animate(s.request); a.shadow = true; break;
    case 0x8010239Cu: move(128); animate(s.request); a.shadow = true; break;
    case 0x8010241Cu:
        if (c < 20) {
            s.raw[2] = int16_t(s.raw[2] - 3); s.fixed[2] = int32_t(uint32_t(s.fixed[2]) + 0xFFFC6A7Fu);
            s.raw[0] = int16_t(s.raw[0] - 3); s.fixed[0] = int32_t(uint32_t(s.fixed[0]) + 0xFFFC6A7Fu);
        } else if (c < 40) { s.raw[2] = int16_t(s.raw[2] - 1); s.fixed[2] = int32_t(uint32_t(s.fixed[2]) + 0xFFFE6A7Fu); }
        animate(s.request); a.shadow = true; break;
    case 0x80102500u: if (c >= 76) move(256); animate(s.request); a.shadow = true; break;
    case 0x80102590u:
        a.shadow = true;
        if (!s.started) { a.reset = true; a.request = s.request; s.started = 1; break; }
        if (c >= 81 && c < 139) move(128);
        if (c == 125) { a.reset = true; a.request = 19; } else a.step = true;
        break;
    case 0x80102678u: move(64); animate(s.request); a.shadow = true; break;
    default: animate(s.request); break;   // unported behaviour: animate in place
    }
    return a;
}

bool spawn_source_attract_static(const std::array<uint8_t,20>& r, SourceAttractStatic& out) noexcept {
    if (r[3] != 0x20 || r[2] != 10 || (r[1] != 0x28 && r[1] != 0x2B)) return false;
    SourceAttractStatic s;
    s.object_class = r[1]; s.slot = r[0];
    s.request = r[1] == 0x28 ? 19 : 0;   // 0x80102DE4 / 0x80102F08
    for (size_t i = 0; i < 3; ++i) s.raw[i] = int16_t(rd16(&r[12 + 2 * i]));
    s.heading = rd16(&r[18]);
    out = s;
    return true;
}

bool spawn_source_attract_teisel(const std::array<uint8_t,20>& r, SourceAttractTeisel& out) noexcept {
    if (r[3] != 0x40 || r[1] != 0x0B || r[2] != 10) return false;
    SourceAttractTeisel t;
    t.slot = r[0];
    for (size_t i = 0; i < 3; ++i) t.raw[i] = int16_t(rd16(&r[12 + 2 * i]));
    t.heading = rd16(&r[18]);
    out = t;
    return true;
}

namespace {
constexpr uint32_t kTeiselFaceBase = 0x80106914u;
int byte_at(const SourceAttractProfile& p, uint32_t token) noexcept {
    if (token < kTeiselFaceBase || token - kTeiselFaceBase >= p.teisel_face.size()) return -1000;
    return int8_t(p.teisel_face[token - kTeiselFaceBase]);
}
uint32_t word_at(const SourceAttractProfile& p, uint32_t token) noexcept {
    if (token < kTeiselFaceBase || token + 4 - kTeiselFaceBase > p.teisel_face.size()) return 0;
    const uint8_t* b = &p.teisel_face[token - kTeiselFaceBase];
    return uint32_t(b[0]) | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24);
}
void face_set_80103a24(SourceAttractTeisel& t, const SourceAttractProfile& p, uint32_t expression) noexcept {
    t.face[0] = {word_at(p, 0x80106944u + 4 * expression), 0, -1};
    t.face[1] = {word_at(p, 0x80106950u + 4 * expression), 0, -1};
}
} // namespace

SourceAttractTeiselStep update_source_attract_teisel_8010384c(SourceAttractTeisel& t, const SourceAttractProfile& p,
                                                               const SourceEventCounters& ev) noexcept {
    SourceAttractTeiselStep out;
    if (t.state == 0) {   // 0x8010386C..0x801038F0, then straight to the face step
        out.reset = true; face_set_80103a24(t, p, 0); t.state = 1;
    } else {
        if (ev.e8 == 4) {
            if (ev.e9 == 1) { if (ev.c10 == 10) face_set_80103a24(t, p, 2); else if (ev.c10 == 90) face_set_80103a24(t, p, 0); }
            else if (ev.e9 == 2) { if (ev.c10 == 1) face_set_80103a24(t, p, 2); else if (ev.c10 == 50) face_set_80103a24(t, p, 1); }
        }
        out.step = true;
    }
    static constexpr uint8_t kParts[2][2] = {{16, 17}, {18, 19}};
    for (size_t ch = 0; ch < 2; ++ch) {   // 0x80103A6C
        auto& f = t.face[ch];
        if (!f.script) continue;
        const int next = byte_at(p, f.script + uint32_t(f.index + 1));
        if (next == 127) f.index = 0;
        else if (next != 126 && next > -1000) f.index = int8_t(f.index + 1);
        const int frame = byte_at(p, f.script + uint32_t(f.index));
        if (frame <= -1000) continue;
        const int16_t value = int16_t(uint8_t(frame));
        if (value != f.previous && out.copy_count < out.copies.size())
            out.copies[out.copy_count++] = {kParts[ch][0], kParts[ch][1], source_face_offset_8002d404(uint8_t(frame), 42, 64, 6)};
        f.previous = value;
    }
    return out;
}
} // namespace l3

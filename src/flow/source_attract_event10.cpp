#include "legends3ds/source_attract_event10.h"

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
    case 0x80100944u: case 0x801009B8u: case 0x80100B14u: case 0x80100B98u:
    case 0x80100C4Cu: case 0x80100CCCu: if (first) ++out.actor_callbacks; break;
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
} // namespace l3

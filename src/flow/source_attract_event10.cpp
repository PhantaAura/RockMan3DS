#include "legends3ds/source_attract_event10.h"

namespace l3 {
namespace {
uint16_t rd16(const uint8_t* p) noexcept { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t* p) noexcept { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
uint32_t crc32(const uint8_t* p, size_t n) noexcept {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
int32_t add(int32_t a, int32_t b) noexcept { return int32_t(uint32_t(a) + uint32_t(b)); }
} // namespace

bool load_source_attract_profile(const std::vector<uint8_t>& b, SourceAttractProfile& out) {
    if (b.size() != kSourceAttractProfileBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'A' || b[3] != 'T' ||
        rd16(&b[4]) != 1 || rd16(&b[6]) != 16 || rd32(&b[8]) != b.size() || rd32(&b[12]) != crc32(b.data() + 16, b.size() - 16))
        return false;
    SourceAttractProfile p;
    const uint8_t* r = b.data() + 16 + 32;
    for (auto& t : p.timelines) {
        t.token = rd32(r); const uint32_t n = rd32(r + 4);
        if (!n || n > 16) return false;
        for (uint32_t k = 0; k < n; ++k) t.records.push_back({{rd32(r + 8 + 8 * k), rd32(r + 12 + 8 * k)}});
        r += 8 + 8 * 16;
    }
    for (auto& c : p.cameras) {
        c.token = rd32(r); const uint32_t n = rd32(r + 4);
        if (!n || n > 256) return false;
        for (uint32_t k = 0; k < n; ++k) c.words.push_back(rd32(r + 8 + 4 * k));
        r += 8 + 4 * 256;
    }
    for (auto& row : p.bg_rows) { for (size_t k = 0; k < 8; ++k) row[k] = r[k]; r += 8; }
    p.loaded = true;
    out = std::move(p);
    return true;
}

void step_source_camera_program_8001ec0c(SourceCameraBlock& c, const SourceAttractCameraProgram& program,
                                         const SourceEventCounters& ev) noexcept {
    for (size_t budget = 0; budget < program.words.size(); ++budget) {
        if (c.pc >= program.words.size()) { c.unsupported = true; break; }
        const uint32_t command = program.words[c.pc];
        const uint8_t op = uint8_t(command >> 24);
        if (command == 0xFFFFFFFFu || op == 255) break;
        if (op <= 2) {
            if (op == 0) { if (ev.c14 < (command & 0x00FFFFFFu)) break; }
            else if (op == 1) { if (ev.e8 != uint8_t(command >> 16) || ev.c18 != (command & 0xFFFFu)) break; }
            else { if (ev.e9 != uint8_t(command >> 16) || ev.c1c != (command & 0xFFFFu)) break; }
            ++c.pc;
        } else if (op == 3 || op == 4) { c.mode = uint8_t(op - 3); ++c.pc;
        } else if (op == 5 || op == 6) { c.tracked = uint8_t(op - 5); ++c.pc;
        } else if (op >= 16 && op <= 22) {
            if (c.pc + 3 >= program.words.size()) { c.unsupported = true; break; }
            std::array<int32_t,3> v{};
            for (size_t i = 0; i < 3; ++i) v[i] = int32_t(program.words[c.pc + 1 + i]);
            if (op == 16) c.tracked = 0;
            if (op == 20) { c.tracked = 1; c.tracked_slot = uint8_t(command >> 16); }
            switch (op) {   // offsets 8, 56, 24, 72, 168, 104, 120
            case 16: c.target = v; break;          case 17: c.angles = v; break;
            case 18: c.target_velocity = v; break; case 19: c.angle_velocity = v; break;
            case 20: c.word_a8 = v; break;         case 21: c.third = v; break;
            default: c.third_velocity = v; break;
            }
            c.pc += 4;
        } else if (op >= 32 && op <= 34) { c.shift = uint8_t(op - 32); ++c.pc;
        } else { c.unsupported = true; break; }
    }
    for (size_t i = 0; i < 3; ++i) {
        c.target[i] = add(c.target[i], c.target_velocity[i]);
        c.angles[i] = add(c.angles[i], c.angle_velocity[i]);
        c.third[i] = add(c.third[i], c.third_velocity[i]);
    }
}

SourceCameraView source_camera_view(const SourceCameraBlock& c) noexcept {
    SourceCameraView v;
    for (size_t i = 0; i < 3; ++i) v.target[i] = add(c.target[i], c.offset_98[i]) >> 16;
    v.yaw = int16_t(uint32_t(c.angles[0]) >> 16); v.pitch = int16_t(uint32_t(c.angles[1]) >> 16);
    v.distance = int16_t(uint32_t(c.angles[2]) >> 16); v.shift = c.shift;
    return v;
}

void step_source_timeline_8001f070(SourceEventCounters& ev, size_t& record, const SourceAttractTimeline& t) noexcept {
    if (record >= t.records.size()) return;
    ++ev.c10; ++ev.c14; ++ev.c18; ++ev.c1c;
    const uint16_t raw = uint16_t(t.records[record][0] >> 16);
    const uint32_t deadline = raw <= 32767u ? raw : (uint32_t(raw) | 0xFFFF0000u);
    if (raw == 0xFFFFu || deadline != ev.c10 || record + 1 >= t.records.size()) return;
    const uint32_t header = t.records[record + 1][0];
    const uint8_t b8 = uint8_t(header), b9 = uint8_t(header >> 8);
    if (ev.e8 != b8) { ev.c18 = 0; ev.c1c = 0; }
    if (ev.e9 != b9) ev.c1c = 0;
    ev.e8 = b8; ev.e9 = b9; ev.c10 = 0; ++record;
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
    case 0x80100944u: if (first) { out.area_request = 1; ++out.actor_callbacks; } break;
    case 0x801009B8u: if (first) { out.area_request = 2; ++out.actor_callbacks; } break;
    case 0x80100B14u: if (first) { out.area_request = 3; ++out.actor_callbacks; } break;
    case 0x80100B98u: if (first) { out.area_request = 4; ++out.actor_callbacks; } break;
    case 0x80100C4Cu: case 0x80100CCCu: if (first) ++out.actor_callbacks; break;
    default: break;
    }
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

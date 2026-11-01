#include "legends3ds/source_scene_runtime.h"

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
// Bounded little-endian reader over the package payload.
struct Reader {
    const std::vector<uint8_t>& b; size_t at; bool ok = true;
    uint32_t u32() { if (at + 4 > b.size()) { ok = false; return 0; } const uint32_t v = rd32(&b[at]); at += 4; return v; }
    uint16_t u16() { if (at + 2 > b.size()) { ok = false; return 0; } const uint16_t v = rd16(&b[at]); at += 2; return v; }
    bool bytes(uint8_t* dst, size_t n) { if (at + n > b.size()) { ok = false; return false; } for (size_t i = 0; i < n; ++i) dst[i] = b[at + i]; at += n; return true; }
};
} // namespace

const SourceSceneTimeline* SourceScenePackage::timeline(uint32_t token) const noexcept {
    for (const auto& t : timelines) if (t.token == token) return &t;
    return nullptr;
}
const SourceSceneCameraProgram* SourceScenePackage::camera(uint32_t token) const noexcept {
    for (const auto& c : cameras) if (c.token == token) return &c;
    return nullptr;
}
const SourceSceneData* SourceScenePackage::data_at(uint32_t token) const noexcept {
    for (const auto& d : data) if (d.token == token) return &d;
    return nullptr;
}

bool load_source_scene_package(const std::vector<uint8_t>& b, SourceScenePackage& out) {
    if (b.size() < 16 + 64 + 8 || b.size() > kSourceScenePackageMaxBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'C' || b[3] != 'S' ||
        rd16(&b[4]) != 1 || rd16(&b[6]) != 16 || rd32(&b[8]) != b.size() || rd32(&b[12]) != crc32(b.data() + 16, b.size() - 16))
        return false;
    SourceScenePackage p;
    Reader r{b, 16 + 64};   // after the EXE and overlay SHA-256
    p.event = r.u16(); p.stage = r.u16(); p.handler = r.u32();
    const uint32_t timelines = r.u32();
    for (uint32_t i = 0; r.ok && i < timelines && i < 64; ++i) {
        SourceSceneTimeline t; t.token = r.u32(); const uint32_t n = r.u32();
        if (n > 64) return false;
        for (uint32_t k = 0; r.ok && k < n; ++k) { const uint32_t h = r.u32(); t.records.push_back({{h, r.u32()}}); }
        p.timelines.push_back(std::move(t));
    }
    const uint32_t cameras = r.u32();
    for (uint32_t i = 0; r.ok && i < cameras && i < 64; ++i) {
        SourceSceneCameraProgram c; c.token = r.u32(); const uint32_t n = r.u32();
        if (n > 1024) return false;
        for (uint32_t k = 0; r.ok && k < n; ++k) c.words.push_back(r.u32());
        p.cameras.push_back(std::move(c));
    }
    const uint32_t spawns = r.u32();
    for (uint32_t i = 0; r.ok && i < spawns && i < 256; ++i) { SourceSceneSpawnRecord s; s.token = r.u32(); r.bytes(s.bytes.data(), 20); p.spawns.push_back(s); }
    const uint32_t requests = r.u32();
    for (uint32_t i = 0; r.ok && i < requests && i < 256; ++i) p.cd_requests.push_back(r.u32());
    const uint32_t areas = r.u32();
    for (uint32_t i = 0; r.ok && i < areas && i < 256; ++i) { SourceSceneAreaRequest a; a.callback = r.u32(); a.area = int32_t(r.u32()); p.area_requests.push_back(a); }
    p.bg_table = r.u32();
    const uint32_t blocks = r.u32();
    for (uint32_t i = 0; r.ok && i < blocks && i < 64; ++i) {
        SourceSceneData d; d.token = r.u32(); const uint32_t n = r.u32();
        if (n > 4096) return false;
        d.bytes.resize(n); r.bytes(d.bytes.data(), n); p.data.push_back(std::move(d));
    }
    if (!r.ok || r.at != b.size() || timelines > 64 || cameras > 64 || spawns > 256 || requests > 256 || areas > 256 || blocks > 64) return false;
    p.loaded = true;
    out = std::move(p);
    return true;
}

void step_source_camera_program_8001ec0c(SourceCameraBlock& c, const SourceSceneCameraProgram& program,
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

void step_source_timeline_8001f070(SourceEventCounters& ev, size_t& record, const SourceSceneTimeline& t) noexcept {
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

} // namespace l3

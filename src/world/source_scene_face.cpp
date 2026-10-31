#include "legends3ds/source_scene_face.h"

namespace l3 {
namespace {
uint16_t rd16(const uint8_t* p) noexcept { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t* p) noexcept { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
}

bool parse_source_scene_face_profile(const uint8_t* p, size_t size, SourceSceneFaceProfile& out) noexcept {
    out = {};
    const size_t need = 2 * (1 + 6 * kSourceFaceCueCapacity) + 2 * kSourceFaceScripts * (1 + 4 * kSourceFaceScriptCapacity);
    if (size != need) return false;
    for (auto& table : out.cues) {
        table.count = p[0]; ++p;
        if (table.count > kSourceFaceCueCapacity) return false;
        for (auto& c : table.cues) { c = {rd16(p), rd16(p + 2), rd16(p + 4)}; p += 6; }
    }
    for (auto& channel : out.scripts) for (auto& s : channel) {
        s.count = p[0]; ++p;
        if (s.count > kSourceFaceScriptCapacity) return false;
        for (auto& e : s.entries) { e = {{p[0], p[1], p[2], p[3]}}; p += 4; }
    }
    out.loaded = true;
    return true;
}

uint32_t source_face_offset_8002d404(uint32_t frame, uint32_t du, uint32_t dv, uint32_t per_row) noexcept {
    if (!per_row) return 0;   // the original divides by zero here
    const uint32_t row = frame / per_row, column = frame - row * per_row;
    const uint32_t u = column * du, v = row * dv;
    return (v << 24) | (u << 16) | (v << 8) | u;
}

size_t step_source_scene_face_8010c820(SourceSceneFaceState& st, const SourceSceneFaceProfile& prof,
    uint16_t event_08, uint32_t event_1c, std::array<SourceFaceUvCopy,4>& out) noexcept {
    size_t n = 0;
    if (!prof.loaded) return 0;
    const uint8_t parts[2][2] = {{16, 17}, {18, 19}};
    const auto emit = [&](size_t ch) {
        const auto& s = prof.scripts[ch][st.script[ch]];
        if (st.entry[ch] >= s.count || n >= out.size()) return;
        out[n++] = {parts[ch][0], parts[ch][1], source_face_offset_8002d404(s.entries[st.entry[ch]][0], 42, 51, 6)};
    };
    // 0x8010C820: cue at the table cursor fires when it names this timeline
    // record and frame; it restarts the channel's script and advances by one.
    for (size_t ch = 0; ch < 2; ++ch) {
        const auto& table = prof.cues[ch];
        if (st.cue_position[ch] >= table.count) continue;
        const auto& cue = table.cues[st.cue_position[ch]];
        if (cue.timeline != event_08 || uint32_t(cue.frame) != event_1c) continue;
        if (cue.script >= kSourceFaceScripts || !prof.scripts[ch][cue.script].count) continue;
        st.script[ch] = uint8_t(cue.script); st.entry[ch] = 0; st.counter[ch] = 0; st.running[ch] = true;
        emit(ch);
        ++st.cue_position[ch];
    }
    // 0x8010C6B8: duration <= 0 jumps by that many entries (loop) and clears
    // the counter; the counter then counts to the current entry's duration.
    for (size_t ch = 0; ch < 2; ++ch) {
        if (!st.running[ch]) continue;
        const auto& s = prof.scripts[ch][st.script[ch]];
        if (st.entry[ch] >= s.count) { st.running[ch] = false; continue; }
        const int16_t d = int16_t(rd16(&s.entries[st.entry[ch]][2]));
        if (d <= 0) {
            const int target = int(st.entry[ch]) + d;
            if (target < 0 || target >= s.count) { st.running[ch] = false; continue; }
            st.entry[ch] = uint8_t(target); st.counter[ch] = 0;
        }
        st.counter[ch] = uint16_t(st.counter[ch] + 1);
        const int16_t now = int16_t(rd16(&s.entries[st.entry[ch]][2]));
        if (int16_t(st.counter[ch]) == now) {
            if (st.entry[ch] + 1 >= s.count) { st.running[ch] = false; continue; }
            ++st.entry[ch]; st.counter[ch] = 0;
            emit(ch);
        }
    }
    return n;
}

bool apply_source_face_uv_8002d320(PbdPart& dst, const uint8_t* ebd, size_t size, uint32_t base,
    uint32_t descriptor, uint8_t source_part, uint32_t offset) noexcept {
    const auto at = [&](uint32_t token, size_t n) -> const uint8_t* {
        if (token < base || token - base > size || n > size - (token - base)) return nullptr;
        return ebd + (token - base);
    };
    const uint8_t* d = at(descriptor + 0x70, 4);
    if (!d) return false;
    const uint8_t* part = at(rd32(d) + 20u + 20u * source_part, 20);
    if (!part) return false;
    const size_t tris = part[0], quads = part[1];
    const uint8_t* tl = tris ? at(rd32(part + 4), tris * 12) : nullptr;
    const uint8_t* ql = quads ? at(rd32(part + 8), quads * 12) : nullptr;
    if ((tris && !tl) || (quads && !ql)) return false;
    if (dst.triangles.size() > tris || dst.quads.size() > quads) return false;
    // Each destination record gets the source record's two UV words + offset.
    const auto word = [&](const uint8_t* s, size_t k) {
        uint32_t w = rd32(s + 4 * k) + offset;
        return std::array<uint8_t,4>{{uint8_t(w), uint8_t(w >> 8), uint8_t(w >> 16), uint8_t(w >> 24)}};
    };
    for (size_t i = 0; i < dst.triangles.size(); ++i) {
        const auto a = word(tl + 12 * i, 0), b = word(tl + 12 * i, 1);
        for (size_t k = 0; k < 4; ++k) dst.triangles[i].uv[k] = a[k];
        dst.triangles[i].uv[4] = b[0]; dst.triangles[i].uv[5] = b[1];
    }
    for (size_t i = 0; i < dst.quads.size(); ++i) {
        const auto a = word(ql + 12 * i, 0), b = word(ql + 12 * i, 1);
        for (size_t k = 0; k < 4; ++k) { dst.quads[i].uv[k] = a[k]; dst.quads[i].uv[4 + k] = b[k]; }
    }
    return true;
}
} // namespace l3

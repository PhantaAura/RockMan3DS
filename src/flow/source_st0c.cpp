#include "legends3ds/source_st0c.h"

namespace l3 {

std::optional<std::vector<StageDataBlock>> load_stage_data(const std::vector<uint8_t>& b) noexcept {
    if (b.size() < 8 || b[0] != 'L' || b[1] != '3' || b[2] != 'S' || b[3] != 'D') return std::nullopt;
    if ((b[4] | (b[5] << 8)) != 1) return std::nullopt;
    const size_t n = size_t(b[6] | (b[7] << 8));
    std::vector<StageDataBlock> out;
    size_t o = 8;
    for (size_t i = 0; i < n; ++i) {
        if (o + 8 > b.size()) return std::nullopt;
        StageDataBlock d;
        d.token = uint32_t(b[o]) | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) | (uint32_t(b[o + 3]) << 24);
        const size_t len = size_t(b[o + 4]) | (size_t(b[o + 5]) << 8) | (size_t(b[o + 6]) << 16) | (size_t(b[o + 7]) << 24);
        o += 8;
        if (len > b.size() - o) return std::nullopt;
        d.bytes.assign(b.begin() + long(o), b.begin() + long(o + len));
        o += len;
        out.push_back(std::move(d));
    }
    if (o != b.size()) return std::nullopt;
    return out;
}

bool bind_st0c_profile(const std::vector<StageDataBlock>& blocks, St03Event8Profile& out) {
    out = {};
    const auto find = [&](uint32_t t, size_t n) -> const std::vector<uint8_t>* {
        for (const auto& d : blocks) if (d.token == t && d.bytes.size() >= n) return &d.bytes;
        return nullptr;
    };
    const auto* rec = find(kSt0CGuardRecord, 20);
    const auto* ws = find(kSt0CWalkSpeeds, 112);
    const auto* it = find(kSt0CIdleTimers, 64);
    const auto* wt = find(kSt0CWalkTables, 112);
    if (!rec || !ws || !it || !wt) return false;
    const auto& r = *rec;
    const auto s16 = [&](size_t o) { return int16_t(uint16_t(r[o] | (r[o + 1] << 8))); };
    St03ObjectRecord g;
    g.slot = r[0]; g.object_class = r[1]; g.subtype = r[2]; g.family = r[3]; g.byte_04 = r[4];
    g.word_08 = {r[8], r[9], r[10], r[11]};
    g.position = {s16(12), s16(14), s16(16)};
    g.heading = uint16_t(r[18] | (r[19] << 8));
    out.object_sets[0] = {g};
    for (size_t i = 0; i < 64; ++i) out.idle_timers[i] = (*it)[i];
    out.idle_timers_loaded = true;
    for (size_t i = 0; i < 112; ++i) { out.walk_speeds[i] = (*ws)[i]; out.walk_tables[i] = (*wt)[i]; }
    out.walk_loaded = true;
    return true;
}

} // namespace l3
